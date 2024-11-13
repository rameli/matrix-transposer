#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

#include "futex/FutexSignaller.h"
#include "matrix-buf/SharedMatrixBuffer.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "BufferDimensions.h"
#include "ClientStats.h"

using ClientId = uint32_t;

struct ClientContext
{
    ClientId id;
    BufferDimensions buffers;
    ClientStats stats;
    UnixSockIpcContext ipcContext;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffers;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffersTr;
    std::unique_ptr<SharedMatrixBuffer> pRequestBuffer;
    std::unique_ptr<FutexSignaller> pTransposeReadyFutex;
};

