#include <iostream>
#include <memory>
#include <unistd.h>

#include "UnixSocketClient.h"

int main()
{
    std::unique_ptr<UnixSocketClient> pClient;
    uint32_t clientID = getpid();
    size_t m = 3;
    size_t n = 33;
    size_t k = 333;

    try
    {
        pClient = std::make_unique<UnixSocketClient>(clientID, m, n, k);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
