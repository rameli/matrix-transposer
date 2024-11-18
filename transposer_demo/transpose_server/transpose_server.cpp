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

void workerThreadRoutine(uint32_t id)
{
    while (gWorkspace.running)
    {
        uint32_t bufferIndex;
        if (gWorkspace.workerQueues[id]->Dequeue(bufferIndex))
        {
            
        }
    }
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

            std::clog << "Adding client PID: " << clientId << std::endl;
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

static void DisplayServerStats()
{
    while (gWorkspace.running)
    {
        std::ostringstream tableHeader;
        std::ostringstream tableFooter;

        tableHeader << "****************************************************" << std::endl;
        tableHeader << "Server running with PID: " << gWorkspace.serverPid << std::endl;
        tableHeader << "Worker Threads: " << gWorkspace.numWorkerThreads << "/" << std::thread::hardware_concurrency() << std::endl;
        tableHeader << "Clients: " << gWorkspace.clientBank.size() << "/" << MAX_CLIENTS << std::endl;

        tableFooter << "Press ENTER to stop the server...";

        Table table(tableHeader.str(), {"Client PID", "M", "N", "K", "Req. Count", "Avg. Processing (ns)"}, MAX_CLIENTS, tableFooter.str());

        system("clear");
        table.clear();

        {
            for (const auto& clientContext : gWorkspace.clientBank)
            {
                if (!clientContext.subscribed)
                {
                    continue;
                }

                table.addRow({std::to_string(clientContext.id),
                            std::to_string(clientContext.matrixSize.m),
                            std::to_string(clientContext.matrixSize.n),
                            std::to_string(clientContext.matrixSize.k),
                            std::to_string(clientContext.stats.GetTotalRequests()),
                            std::to_string(clientContext.stats.GetAverageElapsedTimeUs())
                            });
            }
        }

        std::string tableOutput = table.render();
        std::cout << tableOutput;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Server shutting down..." << std::endl;
    std::cout << "****************************************************" << std::endl;
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

    gWorkspace.serverPid = getpid();
    gWorkspace.clientBank.resize(MAX_CLIENTS);
    gWorkspace.workerThreads.reserve(gWorkspace.numWorkerThreads);
    gWorkspace.workerQueues.reserve(gWorkspace.numWorkerThreads);
    gWorkspace.running = true;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        gWorkspace.clientBank[i].subscribed = false;
    }

    try
    {
        gWorkspace.pIpcServer = std::make_unique<UnixSockIpcServer<ClientServerMessage>>(SERVER_SOCKET_ADDRESS, MessageHandler);

        // for (uint32_t i = 0; i < gWorkspace.numWorkerThreads; i++)
        // {
        //     gWorkspace.workerQueues.push_back(std::make_unique<SpscQueueSeqLock>(i, SpscQueueSeqLock::Role::Producer, WORKER_THREAD_QUEUE_CAPACITY, WORKER_THREAD_QUEUE_NAME_SUFFIX));
        //     gWorkspace.workerThreads.push_back(std::thread(workerThreadRoutine, i));
        // }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    // std::thread serverStatsThread(DisplayServerStats);
    std::thread workloadDispatcherThread(WorkloadDispatcher);

    // std::jthread ipcServerThread([&](){
    //     for (int i = 0; i < gWorkspace.numWorkerThreads; i++)
    //     {
    //         gWorkspace.workerQueues[i]->Enqueue(i);
    //     }
    // });

    std::cin.get();
    gWorkspace.running = false;

    // for (auto& th : gWorkspace.workerThreads)
    // {
    //     th.join();
    // }
    
    // serverStatsThread.join();
    workloadDispatcherThread.join();

    return 0;
}
