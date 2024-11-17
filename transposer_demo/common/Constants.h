#pragma once

#include <string>

namespace MatrixTransposer::Constants
{
    const std::string SERVER_SOCKET_ADDRESS = "/tmp/transpose_server.sock";
    const std::string MATRIX_BUF_NAME_SUFFIX = "";
    const std::string TR_MATRIX_BUF_NAME_SUFFIX = "_tr";
    const std::string TR_GOLDEN_MATRIX_BUF_NAME_SUFFIX = "_tr_gold";
    const std::string REQ_QUEUE_NAME_SUFFIX = "_req";
    constexpr uint32_t REQ_QUEUE_CAPACITY = 16;
    constexpr uint32_t MAX_CLIENTS = 64;
    const std::string WORKER_THREAD_QUEUE_NAME_SUFFIX = "_wrk_q";
    const uint32_t WORKER_THREAD_QUEUE_CAPACITY = 1024*1024;

    constexpr uint32_t TRANSPOSE_TILE_SIZE = 32;

}