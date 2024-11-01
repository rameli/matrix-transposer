#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

class UnixSocketClient {
public:
    UnixSocketClient(const std::string& socketPath);
    ~UnixSocketClient();

    void connectToServer();
    void sendMessage(const std::string& message);
    std::string receiveMessage();

private:
    int clientSocket;
    std::string socketPath;
};
