#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <thread>

#include "futex/Futex.h"
#include "shared-mem/SharedMatrixBuffer.h"
#include "unix-socks/UnixSockIpcClient.h"
#include "ClientServerMessage.h"
#include "Constants.h"
#include "ClientWorkspace.h"

using std::vector;
using std::unique_ptr;
using MatrixTransposer::Constants::SERVER_SOCKET_ADDRESS;

ClientWorkspace gWorkspace;

bool responseReceived = false;

bool ProcessArguments(int argc, char* argv[], uint32_t &m, uint32_t &n, uint32_t &k)
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

void MessageHandler(const ClientServerMessage& message)
{
    std::cout << ClientServerMessage::ToString(message) << std::endl;
    responseReceived = true;
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

        for (int bufferIndex = 0; bufferIndex < gWorkspace.buffers.k; bufferIndex++)
        {
            gWorkspace.matrixBuffers.push_back(std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, gWorkspace.buffers.m, gWorkspace.buffers.n, bufferIndex, SharedMatrixBuffer::Endpoint::CLIENT, false));
            gWorkspace.matrixBuffersTr.push_back(std::make_unique<SharedMatrixBuffer>(gWorkspace.clientPid, gWorkspace.buffers.m, gWorkspace.buffers.n, bufferIndex, SharedMatrixBuffer::Endpoint::CLIENT, true));
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

    ClientServerMessage unsubscribeMessage;
    ClientServerMessage::GenerateUnsubscribeMessage(unsubscribeMessage, gWorkspace.clientPid);
    gWorkspace.pIpcClient->Send(unsubscribeMessage);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 0;
}
