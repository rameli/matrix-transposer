#pragma once

#include <string>

namespace MatrixTransposer::Constants
{
    const std::string SERVER_SOCKET_ADDRESS = "/tmp/transpose_server.sock";
    const std::string MATRIX_BUF_NAME_SUFFIX = "";
    const std::string TR_MATRIX_BUF_NAME_SUFFIX = "_tr";
    const std::string REQ_QUEUE_NAME_SUFFIX = "";
    constexpr uint32_t REQ_QUEUE_CAPACITY = 16;
    constexpr uint32_t MAX_CLIENTS = 32;

    constexpr uint32_t TRANSPOSE_TILE_SIZE = 32;

}