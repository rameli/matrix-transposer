#include "UnixSocketServer.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    {
    UnixSocketServer server;
    std::cout << "Server is running..." << std::endl;

    std::string a;
    std::getline(std::cin, a);
    std::cout << "Server is stopping..." << std::endl;
    }

    // server.stop();
    std::cout << "Server stopped." << std::endl;
    return 0;
}
