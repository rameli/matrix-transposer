#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/futex.h>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "MemoryUtils.h"
#include "SpscQueue.h"

using std::string;

SpscQueue::SpscQueue(uint32_t ownerPid, Role role, size_t capacity, const std::string& nameSuffix) : 
    m_OwnerPid(ownerPid),
    m_Role(role),
    m_Capacity(capacity+1)
{
    if (m_Capacity < 2)
    {
        throw std::invalid_argument("Capacity must be at least 2");
    }

    if (false == std::atomic<size_t>::is_always_lock_free)
    {
        throw std::runtime_error("Atomic size_t is not always lock-free. Cannot create queue.");
    }

    SharedMemory::Ownership bufferOwnership = (role == Role::Producer) ? SharedMemory::Ownership::Owner : SharedMemory::Ownership::Borrower;
    SharedMemory::BufferInitMode bufferInitMode = (role == Role::Producer) ? SharedMemory::BufferInitMode::Zero : SharedMemory::BufferInitMode::NoInit;    
    string bufferShmObjectName = CreateShmObjectName(m_OwnerPid, nameSuffix);
    size_t bufferSizeInBytes = CalculateBufferSize();
    mp_SharedMemory = std::make_unique<SharedMemory>(bufferSizeInBytes, bufferShmObjectName, bufferOwnership, bufferInitMode);
    mp_QueueData = reinterpret_cast<QueueData*>(mp_SharedMemory->GetRawPointer());

    if (role == Role::Producer)
    {
        new (&mp_QueueData->head) std::atomic<size_t>(0);
        new (&mp_QueueData->tail) std::atomic<size_t>(0);
    }
}

SpscQueue::~SpscQueue()
{
}

bool SpscQueue::Enqueue(uint32_t item)
{
    size_t head = mp_QueueData->head.load(std::memory_order_relaxed);
    size_t nextHead = (head + 1) % m_Capacity;

    if (nextHead == mp_QueueData->tail.load(std::memory_order_acquire)) {
        return false;
    }

    mp_QueueData->buffer[head] = item;
    mp_QueueData->head.store(nextHead, std::memory_order_release);
    return true;
}

bool SpscQueue::Dequeue(uint32_t& item)
{
    size_t tail = mp_QueueData->tail.load(std::memory_order_relaxed);

    if (tail == mp_QueueData->head.load(std::memory_order_acquire)) {
        return false;
    }

    item = mp_QueueData->buffer[tail];
    mp_QueueData->tail.store((tail + 1) % m_Capacity, std::memory_order_release);
    return true;
}

uint32_t SpscQueue::GetCapacity() const
{
    return m_Capacity;
}

std::string SpscQueue::CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "spsc_queue_buffer_uid{" << ownerPid << "}" << nameSuffix;
    return oss.str();
}

size_t SpscQueue::CalculateBufferSize()
{
    size_t cacheLineSize = MemoryUtils::GetCacheLineSize();
    if (cacheLineSize == 0)
    {
        cacheLineSize = 64; // At least one full cache line of size 64 bytes
    }

    return (((sizeof(QueueData) + m_Capacity * sizeof(uint32_t))/cacheLineSize) + 1)* cacheLineSize;
}