#include "UnixSocketServer.h"

UnixSocketServer::UnixSocketServer(const std::string& socketPath) : socketPath(socketPath), running(false) {
    // Create the socket
    serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Remove the socket file if it already exists
    unlink(socketPath.c_str());

    // Set up the socket address structure
    sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socketPath.c_str(), sizeof(address.sun_path) - 1);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

UnixSocketServer::~UnixSocketServer() {
    stop();
}

void UnixSocketServer::start() {
    running = true;
    std::cout << "Server is listening on " << socketPath << "..." << std::endl;

    // Accept connections in a separate thread
    std::thread acceptThread(&UnixSocketServer::acceptConnections, this);
    acceptThread.detach();
}

void UnixSocketServer::stop() {
    running = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for the accept thread to exit
    close(serverSocket);
    unlink(socketPath.c_str());
}

void UnixSocketServer::acceptConnections() {
    while (running) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            std::cout << "New client connected." << std::endl;
            clientThreads.emplace_back(&UnixSocketServer::handleClient, this, clientSocket);
        }
    }
}

void UnixSocketServer::handleClient(int clientSocket) {
    receiveMessages(clientSocket);
    close(clientSocket);
}

void UnixSocketServer::receiveMessages(int clientSocket) {
    char buffer[256];
    while (true) {
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the received message
            std::cout << "Received: " << buffer << std::endl;

            // Optionally echo the message back to all clients
            sendMessageToAll(buffer);
        } else {
            break; // Exit if no more data is read
        }
    }
}

void UnixSocketServer::sendMessageToAll(const std::string& message) {
    // Here you can send the message to all connected clients.
    // For simplicity, we're not maintaining a list of clients in this implementation.
    // You would typically need to store client sockets in a vector.
}

