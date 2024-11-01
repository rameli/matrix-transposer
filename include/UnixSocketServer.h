#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <thread>

class UnixSocketServer {
public:
    UnixSocketServer(const std::string& socketPath);
    ~UnixSocketServer();

    void start();
    void sendMessageToAll(const std::string& message);
    void stop();

private:
    void acceptConnections();
    void handleClient(int clientSocket);
    void receiveMessages(int clientSocket);

    int serverSocket;
    std::string socketPath;
    std::vector<std::thread> clientThreads;
    bool running;
};
