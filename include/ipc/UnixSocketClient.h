#pragma once

#include <string>
#include <cstdint>

class UnixSocketClient {
public:
    UnixSocketClient(uint32_t clientID, size_t m, size_t n, size_t k);
    ~UnixSocketClient();

private:
    int m_ClientSocket;
    std::string m_SocketPath;
};
