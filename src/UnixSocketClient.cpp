#include "UnixSocketClient.h"

UnixSocketClient::UnixSocketClient(const std::string& socketPath) : socketPath(socketPath), clientSocket(-1) {
    // Create the socket
    clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
}

UnixSocketClient::~UnixSocketClient() {
    if (clientSocket >= 0) {
        close(clientSocket);
    }
}

void UnixSocketClient::connectToServer() {
    sockaddr_un servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sun_family = AF_UNIX;
    strncpy(servAddr.sun_path, socketPath.c_str(), sizeof(servAddr.sun_path) - 1);

    if (connect(clientSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
}

void UnixSocketClient::sendMessage(const std::string& message) {
    send(clientSocket, message.c_str(), message.size(), 0);
}

std::string UnixSocketClient::receiveMessage() {
    char buffer[256];
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the received message
        return std::string(buffer);
    }
    return "";
}
