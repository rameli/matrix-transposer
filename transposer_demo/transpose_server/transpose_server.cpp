#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <unordered_map>

#include "futex/Futex.h"
#include "shared-mem/SharedMatrixBuffer.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "ClientServerMessage.h"
#include "Constants.h"
#include "ClientContext.h"
#include "ServerWorkspace.h"

using std::vector;
using std::unique_ptr;
using std::unordered_map;
using MatrixTransposer::Constants::SERVER_SOCKET_ADDRESS;
using MatrixTransposer::Constants::MAX_CLIENTS;

ServerWorkspace gWorkspace;

static bool ClientExists(const ServerWorkspace& workspace, uint32_t clientId)
{
    return workspace.clientBank.contains(clientId);
}

static bool AddClient(uint32_t clientId, uint32_t m, uint32_t n, uint32_t k, const UnixSockIpcContext& context)
{
    gWorkspace.clientBank[clientId] = ClientContext();
    ClientContext& newClientContext = gWorkspace.clientBank[clientId];

    try
    {
        newClientContext.id = clientId;
        newClientContext.buffers.m = m;
        newClientContext.buffers.n = n;
        newClientContext.buffers.k = k;
        newClientContext.ipcContext = context;
        newClientContext.matrixBuffers.reserve(k);
        newClientContext.matrixBuffersTr.reserve(k);

        newClientContext.pTransposeReadyFutex = std::make_unique<Futex>(clientId, Futex::Endpoint::SERVER);

        for (uint32_t bufferIndex = 0; bufferIndex < k; bufferIndex++)
        {
            newClientContext.matrixBuffers.push_back(std::make_unique<SharedMatrixBuffer>(clientId, m, n, bufferIndex, SharedMatrixBuffer::Endpoint::SERVER, false));
            newClientContext.matrixBuffersTr.push_back(std::make_unique<SharedMatrixBuffer>(clientId, m, n, bufferIndex, SharedMatrixBuffer::Endpoint::SERVER, true));
        }
    }
    catch(const std::exception& e)
    {
        gWorkspace.clientBank.erase(clientId);
        std::cout << "Failed to set up client context for client PID: " << clientId << std::endl;
        std::cerr << e.what() << '\n';
        return false;
    }

    return true;
}

static bool RemoveClient(uint32_t clientId)
{
    gWorkspace.clientBank.erase(clientId);
    return true;
}

void MessageHandler(const UnixSockIpcContext& context, const ClientServerMessage& message)
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

        if (ClientExists(gWorkspace, clientId))
        {
            std::cout << "Client PID: " << clientId << " already exists" << std::endl;
            return;
        }

        if (!AddClient(clientId, m, n, k, context))
        {
            std::cout << "Failed to add client PID: " << clientId << std::endl;
            return;
        }

        ClientServerMessage responseMessage;
        ClientServerMessage::GenerateSubscribeResponseMessage(responseMessage, gWorkspace.serverPid, clientId, MAX_CLIENTS, gWorkspace.clientBank.size());
        gWorkspace.pIpcServer->Send(context, responseMessage);

        std::cout << ClientServerMessage::ToString(message) << std::endl;
        break;
    }
    case ClientServerMessage::MessageType::Unsubscribe:
    {
        if (!ClientServerMessage::ProcessUnsubscribeMessage(message, clientId))
        {
            std::cout << "Failed to process unsubscribe message from client PID: " << message.senderId << std::endl;
            return;
        }

        if (!ClientExists(gWorkspace, clientId))
        {
            std::cout << "Cannot remove client PID: " << clientId << ". Does not exist" << std::endl;
            return;
        }

        if (!RemoveClient(clientId))
        {
            std::cout << "Failed to remove client PID: " << clientId << std::endl;
            return;
        }

        std::cout << ClientServerMessage::ToString(message) << std::endl;
        break;
    }
    default:
        std::cout << "Received unknown message type from client PID: " << message.senderId << std::endl;
        break;
    }
}

void DisplayServerStats(ServerWorkspace& workspace)
{
    std::cout << "****************************************************" << std::endl;
    std::cout << "Server running with PID: " << workspace.serverPid << std::endl;
    std::cout << "Press ENTER to stop the server..." << std::endl;

    std::cin.get();
    std::cout << "Server shutting down..." << std::endl;
    std::cout << "****************************************************" << std::endl;
}


int main(int argc, char* argv[])
{
    gWorkspace.serverPid = getpid();
    gWorkspace.clientBank.reserve(MAX_CLIENTS);

    try
    {
        gWorkspace.pIpcServer = std::make_unique<UnixSockIpcServer<ClientServerMessage>>(SERVER_SOCKET_ADDRESS, MessageHandler);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    DisplayServerStats(gWorkspace);

    return 0;
}
