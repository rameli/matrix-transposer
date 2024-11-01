#include "UnixSocketServer.h"
#include <thread>
#include <chrono>
#include <string>

int main() {
    const std::string socketPath = "/tmp/mysocket";

    UnixSocketServer server(socketPath);
    server.start();

    std::string a;
    std::getline(std::cin, a);

    server.stop();
    return 0;
}
