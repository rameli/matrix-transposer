#pragma once

#include <cstdint>
#include <memory>

#include "ClientContext.h"
#include "unix-socks/UnixSockIpcServer.h"
#include "ClientServerMessage.h"

using ClientBank = std::unordered_map<ClientId, ClientContext>;

struct ServerWorkspace
{
    uint32_t serverPid;
    ClientBank clientBank;
    std::unique_ptr<UnixSockIpcServer<ClientServerMessage>> pIpcServer;
};