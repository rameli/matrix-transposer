#pragma once

#include <iostream>
#include <atomic>
#include <cstdint>
#include <sys/epoll.h>
#include <functional>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

#include "UnixSockIpcBase.h"

template <typename T>
using ClientMessageHandler = std::function<void(const T&)>;

template <typename T>
class UnixSockIpcClient final : public UnixSockIpcBase<T>
{
public:
    ~UnixSockIpcClient()
    {
        this->m_Running = false;
        this->m_CommunicatorThread.join();

        close(this->m_epollFd);
        close(this->m_BaseSocketFd);
    }    

    UnixSockIpcClient(const std::string& address, ClientMessageHandler<T> handler) : UnixSockIpcBase<T>(address, false)
    {
        m_Handler = handler;

        if(connect(this->m_BaseSocketFd, (const sockaddr*)(&(this->m_AddrStruct)), sizeof(this->m_AddrStruct)) < 0)
        {
            close(this->m_BaseSocketFd);
            throw std::runtime_error("Failed to connect to server (errno: " + std::to_string(errno) + "): " + std::string(strerror(errno)));
        }

        this->m_epollFd = epoll_create1(0);
        if (this->m_epollFd < 0) {
            close(this->m_BaseSocketFd);
            throw std::runtime_error("Failed to create epoll instance (errno: " + std::to_string(errno) + "): " + std::string(strerror(errno)));
        }

        this->ePollEvent.data.fd = this->m_BaseSocketFd;
        this->ePollEvent.events = EPOLLIN;

        if (epoll_ctl(this->m_epollFd, EPOLL_CTL_ADD, this->m_BaseSocketFd, &(this->ePollEvent)) < 0) {
            close(this->m_BaseSocketFd);
            throw std::runtime_error("Failed to add server socket to epoll instance (errno: " + std::to_string(errno) + "): " + std::string(strerror(errno)));
        }

        this->m_Running = true;
        this->m_CommunicatorThread = std::thread(&UnixSockIpcClient::ClientThread, this);
    }

    void Send(const T& message)
    {
        if (!this->m_Running)
        {
            throw std::runtime_error("Communicator is not initialized or is not running");
        }
        
        int r = send(this->m_BaseSocketFd, &message, sizeof(T), 0);
        // std::cout << "Client set(): " << r << std::endl;
    }

private:
    void ClientThread()
    {
        constexpr int MAX_EVENTS = 1;
        constexpr size_t BUFFER_SIZE = sizeof(T);

        epoll_event event;
        int numEvents;
        char buffer[sizeof(T)];
        size_t totalRead;
        ssize_t bytesRead;

        while (this->m_Running)
        {
            numEvents = epoll_wait(this->m_epollFd, &event, MAX_EVENTS, 10);
            if (numEvents == 0)
            {
                continue;
            }
            if (numEvents < 0)
            {
                this->m_Running = false;
                return;
            }

            if (event.events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
            {
                epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, event.data.fd, nullptr);
                this->m_ConnectionCount--;
                this->m_Running = false;
                return;
            }
            else if (event.events & EPOLLIN)
            {
                totalRead = 0;
                // Receive the complete the client setup message
                while ((totalRead < BUFFER_SIZE) && this->m_Running)
                {
                    bytesRead = recv(event.data.fd, buffer + totalRead, BUFFER_SIZE - totalRead, MSG_DONTWAIT);
                    if (bytesRead < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            continue;
                        }
                        else
                        {
                            epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, event.data.fd, nullptr);
                            this->m_ConnectionCount--;
                            this->m_Running = false;
                            return;
                        }
                    }

                    totalRead += bytesRead;
                }

                // Handle message from server
                m_Handler(*reinterpret_cast<T*>(buffer));
            }
        }
    }

    ClientMessageHandler<T> m_Handler;
};