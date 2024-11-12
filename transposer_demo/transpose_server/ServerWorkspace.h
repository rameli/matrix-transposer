#pragma once

#include <cstdint>
#include <memory>
#include <mutex>

#include "ClientContext.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "ClientServerMessage.h"

using ClientBank = std::unordered_map<ClientId, ClientContext>;

struct ServerWorkspace
{
    bool running;
    uint32_t serverPid;
    ClientBank clientBank;
    std::mutex clientBankMutex;
    std::unique_ptr<UnixSockIpcServer<ClientServerMessage>> pIpcServer;
};