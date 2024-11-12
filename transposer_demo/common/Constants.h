#pragma once

#include <string>

namespace MatrixTransposer::Constants
{
    const std::string SERVER_SOCKET_ADDRESS = "/tmp/transpose_server.sock";
    constexpr uint32_t MAX_CLIENTS = 32;
}