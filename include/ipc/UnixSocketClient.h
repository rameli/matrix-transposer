#pragma once

#include <string>
#include <cstdint>

class UnixSocketClient {
public:
    UnixSocketClient(uint32_t clientID, uint32_t m, uint32_t n, uint32_t k);
    ~UnixSocketClient();

private:
    int m_ClientSocket;
    std::string m_SocketPath;
};
