#pragma once

#include <string>

namespace MatrixTransposer::Constants
{
    const std::string SERVER_SOCKET_ADDRESS = "/tmp/transpose_server.sock";
    const std::string SHM_NAME_MATRIX_SUFFIX = "";
    const std::string SHM_NAME_TR_MATRIX_SUFFIX = "_tr";
    const std::string SHM_NAME_REQ_BUF_SUFFIX = "_reqBuf";

    constexpr uint32_t MAX_CLIENTS = 128;
}