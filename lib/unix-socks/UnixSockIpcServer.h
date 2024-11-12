#pragma once

#include <iostream>

#include "UnixSockIpcBase.h"


struct UnixSockIpcContext final
{
    int socket;
};

template <typename T>
using ServerMessageHandler = std::function<void(UnixSockIpcContext, const T&)>;

template <typename T>
class UnixSockIpcServer final : public UnixSockIpcBase<T>
{
public:
    ~UnixSockIpcServer()
    {
        this->m_Running = false;
        this->m_CommunicatorThread.join();

        close(this->m_epollFd);
        close(this->m_BaseSocketFd);
        remove(this->m_BaseSocketAddress.c_str());
    }    

    UnixSockIpcServer(const std::string& address, ServerMessageHandler<T> handler) : UnixSockIpcBase<T>(address, true)
    {
        m_Handler = handler;

        // Socket creation and filling `sockaddr_un` is done in the UnixSockIpcBase constructor

        if(bind(this->m_BaseSocketFd, (const sockaddr*)(&(this->m_AddrStruct)), sizeof(this->m_AddrStruct)) < 0)
        {
            close(this->m_BaseSocketFd);
            throw std::runtime_error("Failed to bind socket (errno: " + std::to_string(errno) + "): " + std::string(strerror(errno)));
        }

        if (listen(this->m_BaseSocketFd, 16) < 0)
        {
            close(this->m_BaseSocketFd);
            throw std::runtime_error("Failed to listen on socket (errno: " + std::to_string(errno) + "): " + std::string(strerror(errno)));
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
        this->m_CommunicatorThread = std::thread(&UnixSockIpcServer::ServerThread, this);
    }

    void Send(const UnixSockIpcContext& context, const T& message)
    {
        if (!this->m_Running)
        {
            return;
        }
        
        send(context.socket, &message, sizeof(T), 0);
    }

private:
    void ServerThread()
    {
        constexpr int MAX_EVENTS = 4;
        constexpr size_t BUFFER_SIZE = sizeof(T);

        epoll_event events[MAX_EVENTS];
        int numEvents;
        char buffer[sizeof(T)];
        size_t totalRead;
        ssize_t bytesRead;

        while (this->m_Running)
        {
            numEvents = epoll_wait(this->m_epollFd, events, MAX_EVENTS, 10);
            if (numEvents < 0)
            {
                break;
            }

            for (int i = 0; i < numEvents; i++)
            {
                if (events[i].data.fd == this->m_BaseSocketFd)
                {
                    // Register the new client socket with epoll
                    int clientSocket = accept(this->m_BaseSocketFd, nullptr, nullptr);
                    if (clientSocket < 0)
                    {
                        continue;
                    }

                    epoll_event clientEvent;
                    clientEvent.data.fd = clientSocket;
                    clientEvent.events = EPOLLIN;
                    epoll_ctl(this->m_epollFd, EPOLL_CTL_ADD, clientSocket, &clientEvent);
                    this->m_ConnectionCount++;
                }
                else if (events[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
                {
                    epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                    close(events[i].data.fd);
                    this->m_ConnectionCount--;
                }
                else if (events[i].events & EPOLLIN)
                {
                    totalRead = 0;
                    // Receive the complete the client setup message
                    while ((totalRead < BUFFER_SIZE) && this->m_Running)
                    {
                        bytesRead = recv(events[i].data.fd, buffer + totalRead, BUFFER_SIZE - totalRead, MSG_DONTWAIT);
                        if (bytesRead < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                continue;
                            }
                            else
                            {
                                epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                                close(events[i].data.fd);
                                this->m_ConnectionCount--;

                                break;
                            }
                        }

                        totalRead += bytesRead;
                    }

                    // Handle message from client
                    m_Handler({events[i].data.fd}, *reinterpret_cast<T*>(buffer));
                }
            }
        }
    
        while ((numEvents = epoll_wait(this->m_epollFd, events, MAX_EVENTS, 1)) > 0)
        {
            for (int i = 0; i < numEvents; i++)
            {
                if (events[i].data.fd != this->m_BaseSocketFd)
                {
                    epoll_ctl(this->m_epollFd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                    close(events[i].data.fd);
                }
            }
        }
    }

    ServerMessageHandler<T> m_Handler;
};