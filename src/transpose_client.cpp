#include "UnixSocketClient.h"
#include <thread>
#include <chrono>

void clientFunction(const std::string& socketPath, const std::string& clientName) {
    UnixSocketClient client(socketPath);
    client.connectToServer();

    for (int i = 0; i < 5; ++i) {
        client.sendMessage(clientName + " says hello " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    const std::string socketPath = "/tmp/mysocket";

    // Start multiple clients
    std::thread client1(clientFunction, socketPath, "Client 1");
    std::thread client2(clientFunction, socketPath, "Client 2");

    client1.join();
    client2.join();

    return 0;
}
