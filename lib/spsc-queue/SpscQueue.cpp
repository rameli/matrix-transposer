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
    m_Capacity(capacity)
{
    if (m_Capacity < 2)
    {
        throw std::invalid_argument("Capacity must be at least 2");
    }

    SharedMemory::Ownership bufferOwnership = (role == Role::Producer) ? SharedMemory::Ownership::Owner : SharedMemory::Ownership::Borrower;
    SharedMemory::BufferInitMode bufferInitMode = (role == Role::Producer) ? SharedMemory::BufferInitMode::Zero : SharedMemory::BufferInitMode::NoInit;    
    string bufferShmObjectName = CreateShmObjectName(m_OwnerPid, nameSuffix);
    size_t bufferSizeInBytes = CalculateBufferSize();
    mp_SharedMemory = std::make_unique<SharedMemory>(bufferSizeInBytes, bufferShmObjectName, bufferOwnership, bufferInitMode);
    m_RawPointer = reinterpret_cast<uint32_t*>(mp_SharedMemory->GetRawPointer());

    FutexSignaller::Role futexRole = (role == Role::Producer) ? FutexSignaller::Role::Waiter : FutexSignaller::Role::Waker;
    string futexShmObjectName = CreateFutexObjectName(m_OwnerPid, nameSuffix);
    mp_Futex = std::make_unique<FutexSignaller>(m_OwnerPid, futexRole, futexShmObjectName);
}

SpscQueue::~SpscQueue()
{
}

uint32_t SpscQueue::GetCapacity() const
{
    return m_Capacity;
}

uint32_t* SpscQueue::GetRawPointer() const
{
    return mp_QueueData->buffer;
}

std::string SpscQueue::CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "spsc_queue_buffer_uid{" << ownerPid << "}" << nameSuffix;
    return oss.str();
}

std::string SpscQueue::CreateFutexObjectName(uint32_t ownerPid, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "spsc_queue_futex_uid{" << ownerPid << "}" << nameSuffix;
    return oss.str();
}


size_t SpscQueue::CalculateBufferSize()
{
    size_t cacheLineSize = MemoryUtils::getCacheLineSize();
    if (cacheLineSize == 0)
    {
        cacheLineSize = 64;
    }    

    return (sizeof(QueueData) + m_Capacity * sizeof(uint32_t))/cacheLineSize * cacheLineSize;
}