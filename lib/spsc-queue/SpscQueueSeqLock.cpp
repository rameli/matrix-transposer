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
#include "SpscQueueSeqLock.h"

using std::string;

SpscQueueSeqLock::SpscQueueSeqLock(uint32_t ownerPid, Role role, size_t capacity, const std::string& nameSuffix) : 
    m_OwnerPid(ownerPid),
    m_Role(role),
    m_Capacity(capacity),
    m_CapacityMinusOne(capacity - 1)
{
    if (false == std::atomic<size_t>::is_always_lock_free)
    {
        throw std::runtime_error("Atomic size_t is not always lock-free. Cannot create queue.");
    }

    if (m_Capacity < 2)
    {
        throw std::invalid_argument("Capacity must be at least 2");
    }

    if ((capacity & (capacity - 1)) != 0)
    {
        throw std::invalid_argument("Capacity must be a power of 2");
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

SpscQueueSeqLock::~SpscQueueSeqLock()
{
}

bool SpscQueueSeqLock::Enqueue(uint32_t item)
{
    size_t head = mp_QueueData->head;
    size_t nextHead = (head + 1) & m_CapacityMinusOne;

    if (nextHead == mp_QueueData->tail) {
        return false;
    }

    mp_QueueData->seq.fetch_add(1, std::memory_order_acq_rel);

    mp_QueueData->buffer[head] = item;
    mp_QueueData->head = nextHead;

    mp_QueueData->seq.fetch_add(1, std::memory_order_acq_rel);

    return true;
}

bool SpscQueueSeqLock::Dequeue(uint32_t& item)
{
    size_t head = mp_QueueData->head;
    size_t tail = mp_QueueData->tail;

    if (tail == head)
    {
        return false;
    }

    size_t seqStart = mp_QueueData->seq.load(std::memory_order_acquire);
    if (seqStart % 2 != 0)
    {
        return false;
    }

    item = mp_QueueData->buffer[tail];
    size_t nextTail = (tail + 1) & m_CapacityMinusOne;

    size_t seqEnd = mp_QueueData->seq.load(std::memory_order_acquire);

    if (seqStart == seqEnd)
    {
        mp_QueueData->tail = nextTail;
        return true;
    }

    return false;
}

size_t SpscQueueSeqLock::GetRealCapacity() const
{
    return m_CapacityMinusOne;
}

std::string SpscQueueSeqLock::CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "spsc_queue_buffer_uid{" << ownerPid << "}" << nameSuffix;
    return oss.str();
}

size_t SpscQueueSeqLock::CalculateBufferSize()
{
    size_t cacheLineSize = MemoryUtils::GetCacheLineSize();
    if (cacheLineSize == 0)
    {
        cacheLineSize = 64; // At least one full cache line of size 64 bytes
    }

    return (((sizeof(QueueData) + m_Capacity * sizeof(uint32_t))/cacheLineSize) + 1)* cacheLineSize;
}