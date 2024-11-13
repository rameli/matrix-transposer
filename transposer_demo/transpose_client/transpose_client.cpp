#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <thread>
#include <random>

#include "futex/Futex.h"
#include "shared-mem/SharedMatrixBuffer.h"
#include "unix-socks/UnixSockIpcClient.h"
#include "ClientServerMessage.h"
#include "Constants.h"
#include "ClientWorkspace.h"

using std::vector;
using std::unique_ptr;

using MatrixTransposer::Constants::SERVER_SOCKET_ADDRESS;
using MatrixTransposer::Constants::SHM_NAME_MATRIX_SUFFIX;
using MatrixTransposer::Constants::SHM_NAME_TR_MATRIX_SUFFIX;
using MatrixTransposer::Constants::SHM_NAME_REQ_BUF_SUFFIX;

ClientWorkspace gWorkspace;

bool responseReceived = false;

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
    // std::cout << ClientServerMessage::ToString(message) << std::endl;
    responseReceived = true;
}

static void FillWithRandom(uint64_t* region, size_t numWords)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    for (size_t i = 0; i < numWords; ++i) {
        region[i] = dist(gen);
    }
}

int main(int argc, char* argv[])
{

    
    if (!ProcessArguments(argc, argv, gWorkspace.buffers.m, gWorkspace.buffers.n, gWorkspace.buffers.k))
    {
        return 1;
    }

    gWorkspace.clientPid = getpid();

    try
    {
        gWorkspace.pIpcClient = std::make_unique<UnixSockIpcClient<ClientServerMessage>>(SERVER_SOCKET_ADDRESS, MessageHandler);
        gWorkspace.pTransposeReadyFutex = std::make_unique<Futex>(gWorkspace.clientPid, Futex::Endpoint::CLIENT);
        gWorkspace.pRequestBuffer = std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, gWorkspace.buffers.m, gWorkspace.buffers.n, 0, SharedMatrixBuffer::Endpoint::CLIENT, SHM_NAME_REQ_BUF_SUFFIX);
        for (int bufferIndex = 0; bufferIndex < gWorkspace.buffers.k; bufferIndex++)
        {
            gWorkspace.matrixBuffers.push_back(std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, gWorkspace.buffers.m, gWorkspace.buffers.n, bufferIndex, SharedMatrixBuffer::Endpoint::CLIENT, SHM_NAME_MATRIX_SUFFIX));
            gWorkspace.matrixBuffersTr.push_back(std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, gWorkspace.buffers.m, gWorkspace.buffers.n, bufferIndex, SharedMatrixBuffer::Endpoint::CLIENT, SHM_NAME_TR_MATRIX_SUFFIX));
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

    while (!responseReceived)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // std::cin.get();
    //     for (int bufferIndex = 0; bufferIndex < gWorkspace.buffers.k; bufferIndex++)
    //     {
    //         FillWithRandom(gWorkspace.matrixBuffers[bufferIndex]->GetRawPointer(), gWorkspace.matrixBuffers[bufferIndex]->GetElementCount());
    //     }
    // std::cin.get();

    ClientServerMessage unsubscribeMessage;
    ClientServerMessage::GenerateUnsubscribeMessage(unsubscribeMessage, gWorkspace.clientPid);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    gWorkspace.pIpcClient->Send(unsubscribeMessage);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));


    return 0;
}
