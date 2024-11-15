#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "shared-mem/SharedMemory.h"
#include "unix-socks/UnixSockIpcClient.h"
#include "futex/FutexSignaller.h"
#include "matrix-buf/SharedMatrixBuffer.h"
#include "spsc-queue/SpscQueue.h"
#include "ClientServerMessage.h"
#include "BufferDimensions.h"
#include "ClientStats.h"

struct ClientWorkspace
{
    uint32_t clientPid;
    BufferDimensions buffers;
    ClientStats stats;
    bool subscribeResponseReceived;
    std::unique_ptr<UnixSockIpcClient<ClientServerMessage>> pIpcClient;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffers;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffersTr;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffersTrReference;
    std::unique_ptr<SpscQueue> pRequestQueue;
    std::unique_ptr<FutexSignaller> pTransposeReadyFutex;
};