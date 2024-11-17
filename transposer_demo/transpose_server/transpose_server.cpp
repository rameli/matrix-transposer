#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
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

using std::shared_lock;
using std::shared_mutex;
using std::unique_lock;
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

ServerWorkspace gWorkspace;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^
std::atomic<bool> gClientBankUpdate { false };
uint64_t gValidClientIds { 0 };
// --------------------------

static bool ClientExists(uint32_t clientId, uint32_t& bankIndex)
{
    for (int i = 0; i < gWorkspace.clientBank.size(); i++)
    {
        if (gWorkspace.clientBank[i].id == clientId)
        {
            bankIndex = i;
            return true;
        }
    }

    return false;
}

static bool AddClient(uint32_t clientId, uint32_t m, uint32_t n, uint32_t k, const UnixSockIpcContext& context)
{
    if (gWorkspace.clientBank.size() >= MAX_CLIENTS)
    {
        return false;
    }

    gWorkspace.clientBank.push_back(ClientContext());

    ClientContext& newClientContext = gWorkspace.clientBank.back();

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
        newClientContext.pRequestQueue = std::make_unique<SpscQueueRingBuffer>(clientId, SpscQueueRingBuffer::Role::Consumer, REQ_QUEUE_CAPACITY, REQ_QUEUE_NAME_SUFFIX);

        for (uint32_t bufferIndex = 0; bufferIndex < k; bufferIndex++)
        {
            newClientContext.matrixBuffers.push_back(std::make_unique<SharedMatrixBuffer>(clientId, SharedMatrixBuffer::Endpoint::Server, m, n, bufferIndex, SharedMatrixBuffer::BufferInitMode::NoInit, MATRIX_BUF_NAME_SUFFIX));
            newClientContext.matrixBuffersTr.push_back(std::make_unique<SharedMatrixBuffer>(clientId, SharedMatrixBuffer::Endpoint::Server, m, n, bufferIndex, SharedMatrixBuffer::BufferInitMode::NoInit, TR_MATRIX_BUF_NAME_SUFFIX));
        }
    }
    catch(const std::exception& e)
    {
        {
            unique_lock<shared_mutex> lock(gWorkspace.clientBankMutex);
            gWorkspace.clientBank.pop_back();
        }
        std::cout << "Failed to set up client context for client PID: " << clientId << std::endl;
        std::cerr << e.what() << '\n';
        return false;
    }

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

    gWorkspace.clientBank.erase(gWorkspace.clientBank.begin() + indexToRemove);
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
            unique_lock<shared_mutex> lock(gWorkspace.clientBankMutex);

            if (ClientExists(clientId, bankIndex))
            {
                std::cout << "Client PID: " << clientId << " already exists" << std::endl;
                return;
            }

            if (!AddClient(clientId, m, n, k, context))
            {
                std::cout << "Failed to add client PID: " << clientId << std::endl;
                return;
            }
        }

        ClientServerMessage responseMessage;
        ClientServerMessage::GenerateSubscribeResponseMessage(responseMessage, gWorkspace.serverPid, clientId, MAX_CLIENTS, gWorkspace.clientBank.size());
        gWorkspace.pIpcServer->Send(context, responseMessage);

        // std::cout << ClientServerMessage::ToString(message) << std::endl;
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

            unique_lock<shared_mutex> lock(gWorkspace.clientBankMutex);

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
                    << ", avgTime: " << clientContext.stats.GetAverageElapsedTimeUs() << " (us)" << std::endl;

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
        tableHeader << "Clients: " << gWorkspace.clientBank.size() << "/" << MAX_CLIENTS << std::endl;

        tableFooter << "Press ENTER to stop the server...";

        Table table(tableHeader.str(), {"Client PID", "M", "N", "K", "Req. Count", "Avg. Processing (us)"}, MAX_CLIENTS, tableFooter.str());

        system("clear");
        table.clear();

        {
            shared_lock<shared_mutex> lock(gWorkspace.clientBankMutex);
            for (const auto& clientContext : gWorkspace.clientBank)
            {
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
    while (gWorkspace.running)
    {
        shared_lock<shared_mutex> lock(gWorkspace.clientBankMutex);
        for (auto& clientContext : gWorkspace.clientBank)
        {
            uint32_t bufferIndex;
            if (clientContext.pRequestQueue->Dequeue(bufferIndex))
            {
                uint64_t* originalMat = clientContext.matrixBuffers[bufferIndex]->GetRawPointer();
                uint64_t* transposeRes = clientContext.matrixBuffersTr[bufferIndex]->GetRawPointer();
                uint32_t rowCount = clientContext.matrixSize.numRows;
                uint32_t columnCount = clientContext.matrixSize.numColumns;
                // std::cerr << "Transposing matrix for client PID: " << clientContext.id << ". Buffer index: " << bufferIndex << std::endl;

                clientContext.stats.StartTimer();
                TransposeTiledMultiThreaded(originalMat, transposeRes, rowCount, columnCount, TRANSPOSE_TILE_SIZE, gWorkspace.numWorkerThreads);
                clientContext.stats.StopTimer();

                clientContext.pTransposeReadyFutex->Wake();
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
    gWorkspace.clientBank.reserve(MAX_CLIENTS);
    gWorkspace.running = true;

    try
    {
        gWorkspace.pIpcServer = std::make_unique<UnixSockIpcServer<ClientServerMessage>>(SERVER_SOCKET_ADDRESS, MessageHandler);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    std::jthread serverStatsThread(DisplayServerStats);
    std::jthread workloadDispatcherThread(WorkloadDispatcher);

    std::cin.get();
    gWorkspace.running = false;

    return 0;
}
