#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class UnixSocketServer
{
public:
    UnixSocketServer(const std::string& socketPath);
    ~UnixSocketServer();

    void stop();

private:
    void acceptConnections();
    void handleClient(int clientSocket);

    int m_ServerSocket;
    std::string m_SocketPath;
    
    std::vector<std::thread> m_ClientThreadsVec;

    std::thread m_AcceptThread;
    std::atomic<bool> m_Running;
};