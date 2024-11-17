#pragma once

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <vector>
#include <thread>

#include "ClientContext.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "ClientServerMessage.h"
#include "spsc-queue/SpscQueueSeqLock.h"


using ClientBank = std::vector<ClientContext>;

struct ServerWorkspace
{
    bool running;
    uint32_t numWorkerThreads;
    uint32_t serverPid;
    ClientBank clientBank;
    std::shared_mutex clientBankMutex;
    std::unique_ptr<UnixSockIpcServer<ClientServerMessage>> pIpcServer;
    std::atomic<bool> clientBankUpdateAvailable { false };
    uint64_t validClientsBitSet { 0 };
    std::vector<std::thread> workerThreads;
    std::vector<std::unique_ptr<SpscQueueSeqLock>> workerQueues;
};