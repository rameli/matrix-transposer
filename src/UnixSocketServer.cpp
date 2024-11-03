#include "UnixSocketServer.h"

UnixSocketServer::UnixSocketServer(const std::string& socketPath) :
    m_SocketPath(socketPath),
    m_Running(false)
{
    m_ServerSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_ServerSocket < 0)
    {
        throw std::runtime_error("Failed to create server socket.");
    }

    unlink(socketPath.c_str());

    sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socketPath.c_str(), sizeof(address.sun_path) - 1);

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
    std::cout << "Server is listening on " << socketPath << "..." << std::endl;

    m_ClientThreadsVec.reserve(32);
    m_AcceptThread = std::thread(&UnixSocketServer::acceptConnections, this);
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
    
    for (auto& clientThread : m_ClientThreadsVec)
    {
        clientThread.join();
    }
    
    shutdown(m_ServerSocket, SHUT_RDWR);
    close(m_ServerSocket);
    m_AcceptThread.join();

    unlink(m_SocketPath.c_str());
}

void UnixSocketServer::acceptConnections() {
   
    while (m_Running)
    {
        std::cout << "accept()" << std::endl;
        int clientSocket = accept(m_ServerSocket, nullptr, nullptr);
        std::cout << "accept() returned" << std::endl;

        if (clientSocket >= 0)
        {
            m_ClientThreadsVec.emplace_back(&UnixSocketServer::handleClient, this, clientSocket);
        }
    }
}

void UnixSocketServer::handleClient(int clientSocket) {
    char buffer[256];
    std::cout << "Handle client: " << clientSocket << std::endl;

    while (m_Running)
    {
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            std::cout << "Received: " << buffer << std::endl;
        } else {
            break;
        }
    }
    std::cout << "Handle client exiting: " << clientSocket << std::endl;

    close(clientSocket);
}