#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <sys/epoll.h>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <cerrno>
#include <cstring>

template <typename T>
class UnixSockIpcBase
{
public:
    virtual ~UnixSockIpcBase() = default;
    UnixSockIpcBase(const std::string& address, bool instanceIsServer)
    {
        m_BaseSocketAddress = address;
        m_ConnectionCount = 0;
        m_Running = false;

        if (address.size() >= sizeof(m_AddrStruct.sun_path) - 1)
        {
            throw std::runtime_error("Address is too long. Must be less than " + std::to_string(sizeof(m_AddrStruct.sun_path) - 1) + " characters");
        }

        m_BaseSocketFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_BaseSocketFd < 0)
        {
            throw std::runtime_error("Failed to create socket (errno: " + std::to_string(errno) + "): " + std::string(strerror(errno)));
        }

        if (instanceIsServer)
        {
            remove(address.c_str());
        }

        memset(&m_AddrStruct, 0, sizeof(m_AddrStruct));
        m_AddrStruct.sun_family = AF_UNIX;
        strncpy(m_AddrStruct.sun_path, address.c_str(), sizeof(m_AddrStruct.sun_path) - 1);
    }

    uint32_t GetConnectionCount() const noexcept
    {
        return m_ConnectionCount.load();
    }

    bool IsRunning() const noexcept
    {
        return m_Running;
    }

protected:
    int m_BaseSocketFd;
    struct sockaddr_un m_AddrStruct;
    epoll_event ePollEvent;

    std::string m_BaseSocketAddress;

    std::atomic<uint32_t> m_ConnectionCount;
    bool m_Running;

    std::thread m_CommunicatorThread;
    int m_epollFd;
};