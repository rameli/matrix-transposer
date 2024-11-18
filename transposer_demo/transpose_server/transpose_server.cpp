#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "ClientContext.h"
#include "ClientServerMessage.h"
#include "Constants.h"
#include "futex/FutexSignaller.h"
#include "mat-transpose/mat-transpose.h"
#include "matrix-buf/SharedMatrixBuffer.h"
#include "presentation/Table.h"
#include "ServerWorkspace.h"
#include "unix-socks/UnixSockIpcServer.h"

using std::unique_ptr;
using std::unordered_map;
using std::vector;

using MatrixTransposer::Constants::SERVER_SOCKET_ADDRESS;
using MatrixTransposer::Constants::MATRIX_BUF_NAME_SUFFIX;
using MatrixTransposer::Constants::TR_MATRIX_BUF_NAME_SUFFIX;
using MatrixTransposer::Constants::REQ_QUEUE_NAME_SUFFIX;
using MatrixTransposer::Constants::REQ_QUEUE_CAPACITY;
using MatrixTransposer::Constants::MAX_CLIENTS;
using MatrixTransposer::Constants::TRANSPOSE_TILE_SIZE;
using MatrixTransposer::Constants::WORKER_THREAD_QUEUE_CAPACITY;
using MatrixTransposer::Constants::WORKER_THREAD_QUEUE_NAME_SUFFIX;

ServerWorkspace gWorkspace;

void setBit(uint64_t &number, int position, bool value)
{
    if (value)
        number |= (1ULL << position);
    else
        number &= ~(1ULL << position);
}

bool getBit(const uint64_t &number, int position)
{
    return (number & (1ULL << position)) != 0;
}

static bool ClientExists(uint32_t clientId, uint32_t& bankIndex)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (gWorkspace.clientBank[i].subscribed)
        {
            if (gWorkspace.clientBank[i].id == clientId)
            {
                bankIndex = i;
                return true;
            }
        }
    }

    return false;
}

static bool AddClient(uint32_t clientId, uint32_t m, uint32_t n, uint32_t k, const UnixSockIpcContext& context)
{
    int32_t indexToAdd;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!getBit(gWorkspace.validClientsBitSet, i))
        {
            indexToAdd = i;
            break;
        }
    }

    gWorkspace.clientBank[indexToAdd] = ClientContext();

    ClientContext& newClientContext = gWorkspace.clientBank[indexToAdd];

    try
    {
        newClientContext.id = clientId;
        newClientContext.matrixSize.m = m;
        newClientContext.matrixSize.n = n;
        newClientContext.matrixSize.k = k;
        newClientContext.matrixSize.numRows = 1 << m;
        newClientContext.matrixSize.numColumns = 1 << n;
        newClientContext.ipcContext = context;
        newClientContext.matrixBuffers.reserve(k);
        newClientContext.matrixBuffersTr.reserve(k);

        newClientContext.pTransposeReadyFutex = std::make_unique<FutexSignaller>(clientId, FutexSignaller::Role::Waker, "");
        newClientContext.pRequestQueue = std::make_unique<SpscQueueSeqLock>(clientId, SpscQueueSeqLock::Role::Consumer, REQ_QUEUE_CAPACITY, REQ_QUEUE_NAME_SUFFIX);

        for (uint32_t bufferIndex = 0; bufferIndex < k; bufferIndex++)
        {
            newClientContext.matrixBuffers.push_back(std::make_unique<SharedMatrixBuffer>(clientId, SharedMatrixBuffer::Endpoint::Server, m, n, bufferIndex, SharedMatrixBuffer::BufferInitMode::NoInit, MATRIX_BUF_NAME_SUFFIX));
            newClientContext.matrixBuffersTr.push_back(std::make_unique<SharedMatrixBuffer>(clientId, SharedMatrixBuffer::Endpoint::Server, m, n, bufferIndex, SharedMatrixBuffer::BufferInitMode::NoInit, TR_MATRIX_BUF_NAME_SUFFIX));
        }

        newClientContext.subscribed = true;
    }
    catch(const std::exception& e)
    {
        {
        }
        std::cout << "Failed to set up client context for client PID: " << clientId << std::endl;
        std::cerr << e.what() << '\n';
        return false;
    }



    // Waiting for thread dispatcher to pick up the new client
    while (gWorkspace.clientBankUpdateAvailable.load(std::memory_order_acquire));

    setBit(gWorkspace.validClientsBitSet, indexToAdd, 1);
    gWorkspace.clientBankUpdateAvailable.store(true, std::memory_order_release);

    return true;
}

static void RemoveClient(uint32_t clientId)
{
    int indexToRemove = 0;

    for (int i = 0; i < gWorkspace.clientBank.size(); i++)
    {
        if (gWorkspace.clientBank[i].id == clientId)
        {
            indexToRemove = i;
            break;
        }
    }

    // Waiting for thread dispatcher to pick up the new client
    while (gWorkspace.clientBankUpdateAvailable.load(std::memory_order_acquire));
    setBit(gWorkspace.validClientsBitSet, indexToRemove, 0);
    gWorkspace.clientBankUpdateAvailable.store(true, std::memory_order_release);
    while (gWorkspace.clientBankUpdateAvailable.load(std::memory_order_acquire));
    gWorkspace.clientBank[indexToRemove].subscribed = false;
}

static void MessageHandler(const UnixSockIpcContext& context, const ClientServerMessage& message)
{
    uint32_t clientId;

    switch (message.type)
    {
    case ClientServerMessage::MessageType::Subscribe:
    {
        uint32_t m, n, k;
        if (!ClientServerMessage::ProcessSubscribeMessage(message, clientId, m, n, k))
        {
            std::cout << "Failed to process subscribe message from client PID: " << message.senderId << std::endl;
            return;
        }

        {
            uint32_t bankIndex;
            if (ClientExists(clientId, bankIndex))
            {
                std::clog << "Client PID: " << clientId << " already exists" << std::endl;
                return;
            }

            std::clog << "New client: " << clientId << std::endl;
            if (!AddClient(clientId, m, n, k, context))
            {
                std::clog << "Failed to add client PID: " << clientId << std::endl;
                return;
            }
        }

        ClientServerMessage responseMessage;
        ClientServerMessage::GenerateSubscribeResponseMessage(responseMessage, gWorkspace.serverPid, clientId, MAX_CLIENTS, gWorkspace.clientBank.size());
        gWorkspace.pIpcServer->Send(context, responseMessage);

        break;
    }
    case ClientServerMessage::MessageType::Unsubscribe:
    {
        if (!ClientServerMessage::ProcessUnsubscribeMessage(message, clientId))
        {
            std::cout << "Failed to process unsubscribe message from client PID: " << message.senderId << std::endl;
            return;
        }

        {
            uint32_t bankIndex;
            if (!ClientExists(clientId, bankIndex))
            {
                std::cout << "Cannot remove client PID: " << clientId << ". Does not exist" << std::endl;
                return;
            }

            ClientContext& clientContext = gWorkspace.clientBank[bankIndex];
            std::clog << "client: " << clientContext.id 
                    << ", m: "<< clientContext.matrixSize.m
                    << ", n: " << clientContext.matrixSize.n 
                    << ", k: " << clientContext.matrixSize.k
                    << ", totalReqs: " << clientContext.stats.GetTotalRequests()
                    << ", avgTime: " << clientContext.stats.GetAverageElapsedTimeUs() << " (ns)" << std::endl;

            RemoveClient(clientId);
        }

        break;
    }
    default:
        std::cout << "Received unknown message type from client PID: " << message.senderId << std::endl;
        break;
    }

}

static void WorkloadDispatcher()
{
    uint64_t localValidClientsBitSet = 0;

    while (gWorkspace.running)
    {
        if (gWorkspace.clientBankUpdateAvailable.load(std::memory_order_acquire))
        {
            localValidClientsBitSet = gWorkspace.validClientsBitSet;
            gWorkspace.clientBankUpdateAvailable.store(false, std::memory_order_release);
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (!getBit(localValidClientsBitSet, i))
            {
                continue;
            }
            auto& clientContext = gWorkspace.clientBank[i];

            uint32_t bufferIndex;
            if (clientContext.pRequestQueue->Dequeue(bufferIndex))
            {
                uint64_t* pOriginalMat = clientContext.matrixBuffers[bufferIndex]->GetRawPointer();
                uint64_t* pTransposeRes = clientContext.matrixBuffersTr[bufferIndex]->GetRawPointer();
                uint32_t rowCount = clientContext.matrixSize.numRows;
                uint32_t columnCount = clientContext.matrixSize.numColumns;

                clientContext.stats.StartTimer();
                TransposeTiledMultiThreaded(pOriginalMat, pTransposeRes, rowCount, columnCount, TRANSPOSE_TILE_SIZE, gWorkspace.numWorkerThreads);

                clientContext.pTransposeReadyFutex->Wake();
                clientContext.stats.StopTimer();
            }
        }
    }
}

int main(int argc, char* argv[])
{
    gWorkspace.numWorkerThreads = std::thread::hardware_concurrency();

    if (argc > 1)
    {
        gWorkspace.numWorkerThreads = std::atoi(argv[1]);
    }

    if (gWorkspace.numWorkerThreads & (gWorkspace.numWorkerThreads - 1))
    {
        std::cerr << "Number of worker threads must be a power of 2" << std::endl;
        return 1;
    }

    gWorkspace.serverPid = getpid();
    gWorkspace.clientBank.resize(MAX_CLIENTS);
    gWorkspace.running = true;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        gWorkspace.clientBank[i].subscribed = false;
    }

    try
    {
        gWorkspace.pIpcServer = std::make_unique<UnixSockIpcServer<ClientServerMessage>>(SERVER_SOCKET_ADDRESS, MessageHandler);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    std::thread workloadDispatcherThread(WorkloadDispatcher);


    std::clog << "Server PID: " << gWorkspace.serverPid << std::endl;
    std::clog << "Running " << gWorkspace.numWorkerThreads << "/" << std::thread::hardware_concurrency() << " worker threads" << std::endl;
    std::clog << "Press Enter to stop the server" << std::endl;

    std::cin.get();
    gWorkspace.running = false;

    workloadDispatcherThread.join();

    return 0;
}
