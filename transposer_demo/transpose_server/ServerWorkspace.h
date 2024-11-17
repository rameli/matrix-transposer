#pragma once

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <vector>

#include "ClientContext.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "ClientServerMessage.h"

using ClientBank = std::vector<ClientContext>;

struct ServerWorkspace
{
    bool running;
    uint32_t numWorkerThreads;
    uint32_t serverPid;
    ClientBank clientBank;
    std::shared_mutex clientBankMutex;
    std::unique_ptr<UnixSockIpcServer<ClientServerMessage>> pIpcServer;
};