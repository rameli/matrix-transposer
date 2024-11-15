#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

#include "spsc-queue/SpscQueue.h"
#include "futex/FutexSignaller.h"
#include "matrix-buf/SharedMatrixBuffer.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "shared-mem/SharedMemory.h"
#include "BufferDimensions.h"
#include "ClientStats.h"

using ClientId = uint32_t;

struct ClientContext
{
    ClientId id;
    BufferDimensions matrixSize;
    ClientStats stats;
    UnixSockIpcContext ipcContext;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffers;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffersTr;
    std::unique_ptr<SpscQueue> pRequestQueue;
    std::unique_ptr<FutexSignaller> pTransposeReadyFutex;
};

