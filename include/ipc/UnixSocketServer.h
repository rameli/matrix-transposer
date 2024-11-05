#pragma once

#include <thread>
#include <atomic>
#include <vector>

class UnixSocketServer
{
public:
    UnixSocketServer();
    ~UnixSocketServer();

    void stop();

private:
    void AcceptConnections();
    void HandleClient(int clientSocket);

    int m_ServerSocket;
    std::string m_SocketPath;
    
    std::thread m_AcceptThread;
    std::atomic<bool> m_Running;
};