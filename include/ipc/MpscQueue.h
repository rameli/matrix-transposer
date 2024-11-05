#pragma once

#include "MPSCQueueRingBuffer.h"

class MPSCQueueClass
{
public:
    MPSCQueueClass();
    ~MPSCQueueClass();

    void Enqueue(const ClientRequest& request);
    bool Dequeue(ClientRequest& request);

private:
    void CreateSharedMemory();
    void DetachSharedMemory();
    void InitializeQueue();

    int m_FileDescriptor;
    std::string m_ShmObjectName;

    MPSCQueue* m_Queue;
};


