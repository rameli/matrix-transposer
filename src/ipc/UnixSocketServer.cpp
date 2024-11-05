#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <mutex>

#include "ipc/ClientSetupMessage.h"
#include "ipc/UnixSocketServer.h"

UnixSocketServer::UnixSocketServer() :
    m_Running {false},
    m_SocketPath {"/tmp/matrix_transposer_server.sock"}
{
    m_ServerSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_ServerSocket < 0)
    {
        throw std::runtime_error("Failed to create server socket.");
    }

    unlink(m_SocketPath.c_str());

    sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, m_SocketPath.c_str(), sizeof(address.sun_path) - 1);

    if (bind(m_ServerSocket, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        close(m_ServerSocket);
        throw std::runtime_error("Failed to bind server socket.");
    }

    if (listen(m_ServerSocket, 20) < 0)
    {
        close(m_ServerSocket);
        throw std::runtime_error("Failed to listen on server socket.");
    }

    m_Running = true;
    m_AcceptThread = std::thread(&UnixSocketServer::AcceptConnections, this);
}

UnixSocketServer::~UnixSocketServer()
{
    if (m_Running)
    {
        stop();
    }
}

void UnixSocketServer::stop()
{
    m_Running = false;
    
    shutdown(m_ServerSocket, SHUT_RDWR);
    close(m_ServerSocket);
    m_AcceptThread.join();

    unlink(m_SocketPath.c_str());
}

void UnixSocketServer::AcceptConnections() {
   
    while (m_Running)
    {
        int clientSocket = accept(m_ServerSocket, nullptr, nullptr);

        if (clientSocket >= 0)
        {
            std::thread clientThread(&UnixSocketServer::HandleClient, this, clientSocket);
            clientThread.detach();
        }
    }
}

void UnixSocketServer::HandleClient(int clientSocket) {
    constexpr size_t BUFFER_SIZE = sizeof(ClientSetupMessage);

    char buffer[BUFFER_SIZE];
    ssize_t totalRead = 0;
    ssize_t bytesRead;

    // Receive the complete the client setup message
    while (totalRead < BUFFER_SIZE) {
        bytesRead = read(clientSocket, buffer + totalRead, BUFFER_SIZE - totalRead);
        if (bytesRead <= 0) {
            close(clientSocket);
            return;
        }
        totalRead += bytesRead;
    }

    ClientSetupMessage* setupMessage = reinterpret_cast<ClientSetupMessage*>(buffer);
    std::cout << "Received client setup message: "<< std::endl;
    std::cout << "clientID: " << setupMessage->clientID << std::endl;
    std::cout << "m: " << setupMessage->m << std::endl;
    std::cout << "n: " << setupMessage->n << std::endl;
    std::cout << "k: " << setupMessage->k << std::endl;

    while (m_Running)
    {
        bytesRead = read(clientSocket, buffer, 1);
        if (bytesRead <= 0)
        {
            // Handle error or EOF
            break;
        }
    }

    close(clientSocket);
}