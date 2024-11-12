#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "unix-socks/UnixSockIpcClient.h"
#include "futex/Futex.h"
#include "shared-mem/SharedMatrixBuffer.h"
#include "ClientServerMessage.h"
#include "BufferDimensions.h"
#include "ClientStats.h"

struct ClientWorkspace
{
    uint32_t clientPid;
    BufferDimensions buffers;
    ClientStats stats;
    std::unique_ptr<UnixSockIpcClient<ClientServerMessage>> pIpcClient;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffers;
    std::vector<std::unique_ptr<SharedMatrixBuffer>> matrixBuffersTr;
    std::unique_ptr<Futex> pTransposeReadyFutex;
};