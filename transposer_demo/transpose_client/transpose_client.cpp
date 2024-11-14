#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <thread>
#include <random>

#include "futex/FutexSignaller.h"
#include "matrix-buf/SharedMatrixBuffer.h"
#include "unix-socks/UnixSockIpcClient.h"
#include "ClientServerMessage.h"
#include "Constants.h"
#include "ClientWorkspace.h"

using std::vector;
using std::unique_ptr;

using MatrixTransposer::Constants::SERVER_SOCKET_ADDRESS;
using MatrixTransposer::Constants::MATRIX_BUF_NAME_SUFFIX;
using MatrixTransposer::Constants::TR_MATRIX_BUF_NAME_SUFFIX;
using MatrixTransposer::Constants::REQ_QUEUE_NAME_SUFFIX;
using MatrixTransposer::Constants::REQ_QUEUE_CAPACITY;

ClientWorkspace gWorkspace;


static bool ProcessArguments(int argc, char* argv[], uint32_t &m, uint32_t &n, uint32_t &k)
 {
    if (argc != 4 && argc != 1)
    {
        std::cerr << "Usage: " << argv[0] << " <m> <n> <k>" << std::endl;
        return false;
    }

    if (argc == 1)
    {
        m = 4;
        n = 4;
        k = 8;
        return true;
    }

    m = std::atoi(argv[1]);
    n = std::atoi(argv[2]);
    k = std::atoi(argv[3]);
    return true;
}

static void MessageHandler(const ClientServerMessage& message)
{
    std::cout << ClientServerMessage::ToString(message) << std::endl;
    gWorkspace.subscribeResponseReceived = true;
}

int main(int argc, char* argv[])
{
    if (!ProcessArguments(argc, argv, gWorkspace.buffers.m, gWorkspace.buffers.n, gWorkspace.buffers.k))
    {
        return 1;
    }

    try
    {
        gWorkspace.clientPid = getpid();
        gWorkspace.subscribeResponseReceived = false;
        gWorkspace.pIpcClient = std::make_unique<UnixSockIpcClient<ClientServerMessage>>(SERVER_SOCKET_ADDRESS, MessageHandler);
        gWorkspace.pTransposeReadyFutex = std::make_unique<FutexSignaller>(gWorkspace.clientPid, FutexSignaller::Role::Waiter, "");
        gWorkspace.pRequestQueue = std::make_unique<SpscQueue>(gWorkspace.clientPid, SpscQueue::Role::Producer, REQ_QUEUE_CAPACITY, REQ_QUEUE_NAME_SUFFIX);

        gWorkspace.matrixBuffers.reserve(gWorkspace.buffers.k);
        gWorkspace.matrixBuffersTr.reserve(gWorkspace.buffers.k);
        for (int bufferIndex = 0; bufferIndex < gWorkspace.buffers.k; bufferIndex++)
        {
            gWorkspace.matrixBuffers.push_back(std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, SharedMatrixBuffer::Endpoint::Client, gWorkspace.buffers.m, gWorkspace.buffers.n, bufferIndex, SharedMatrixBuffer::BufferInitMode::Random, MATRIX_BUF_NAME_SUFFIX));
            gWorkspace.matrixBuffersTr.push_back(std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, SharedMatrixBuffer::Endpoint::Client, gWorkspace.buffers.m, gWorkspace.buffers.n, bufferIndex, SharedMatrixBuffer::BufferInitMode::Zero, TR_MATRIX_BUF_NAME_SUFFIX));
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    
    ClientServerMessage subscribeMessage;
    ClientServerMessage::GenerateSubscribeMessage(subscribeMessage, gWorkspace.clientPid, gWorkspace.buffers.m, gWorkspace.buffers.n, gWorkspace.buffers.k);
    gWorkspace.pIpcClient->Send(subscribeMessage);

    while (!gWorkspace.subscribeResponseReceived)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (int i = 0; i < 10; i++)
    {
        std::cout << "Req " << i;
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        gWorkspace.pRequestQueue->Enqueue(gWorkspace.clientPid);
        gWorkspace.pTransposeReadyFutex->Wait();
        std::cout << " ... completed: " << std::endl;
    }

    ClientServerMessage unsubscribeMessage;
    ClientServerMessage::GenerateUnsubscribeMessage(unsubscribeMessage, gWorkspace.clientPid);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));


    std::cin.get();
    gWorkspace.pIpcClient->Send(unsubscribeMessage);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));


    return 0;
}
