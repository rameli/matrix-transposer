#include <iostream>
#include <cstdint>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sstream>
#include <string>
#include <cstring>
#include <atomic>
#include <fstream>
#include <cstdlib>

#include "MemoryUtils.h"
#include "FutexSignaller.h"

using std::string;

FutexSignaller::FutexSignaller(uint32_t ownerPid, Role role, const std::string& nameSuffix) : 
    m_OwnerPid(ownerPid),
    m_Role(role)
{
    SharedMemory::Ownership ownership = (role == Role::Waiter) ? SharedMemory::Ownership::Owner : SharedMemory::Ownership::Borrower;
    SharedMemory::BufferInitMode bufferInitMode = role == Role::Waiter ? SharedMemory::BufferInitMode::Zero : SharedMemory::BufferInitMode::NoInit;
    string shmObjectName = CreateShmObjectName(m_OwnerPid, nameSuffix);
    size_t bufferSizeInBytes = MemoryUtils::GetCacheLineSize();
    if (bufferSizeInBytes == 0)
    {
        bufferSizeInBytes = 64; // At least one full cache line of size 64 bytes
    }

    mp_SharedMemory = std::make_unique<SharedMemory>(bufferSizeInBytes, shmObjectName, ownership, bufferInitMode);
    void* ptr = mp_SharedMemory->GetRawPointer();

    if (role == Role::Waiter)
    {
        m_RawPointer = new (ptr) std::atomic<uint32_t>(0);
    }
    else
    {
        m_RawPointer = static_cast<std::atomic<uint32_t>*>(ptr);
    }
}

std::string FutexSignaller::CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "futex_uid{" << ownerPid << "}" << nameSuffix;
    return oss.str();
}

FutexSignaller::~FutexSignaller()
{
}

void FutexSignaller::Wait()
{
    // Loop is required to handle spurious wakeups in FUTEX_WAIT
    while (1)
    {
        uint32_t expected = 1; // Reset expected value in each iteration as compare_exchange_strong modifies it if the condition fails
        if (m_RawPointer->compare_exchange_strong(expected, 0))
        {
            // Already signalled. No waiting required. Set the futex to non-signalled state and return.
            break;
        }

        long res = syscall(SYS_futex, m_RawPointer, FUTEX_WAIT, 0, nullptr, nullptr, 0);
        if (res == -1 && errno != EAGAIN)
        {
            std::cout << "FUTEX_WAIT error(" << errno << "): " << strerror(errno) << std::endl;
        }
    }
}

bool FutexSignaller::IsWaiting()
{
    return (m_RawPointer->load(std::memory_order_relaxed) == 0);
}

void FutexSignaller::Wake()
{
    uint32_t expected = 0;
    if (m_RawPointer->compare_exchange_strong(expected, 1))
    {
        // Wake up one waiting thread if any.
        int res = syscall(SYS_futex, m_RawPointer, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
        if (res == -1)
        {
            std::cout << "FUTEX_WAKE error(" << errno << "): " << strerror(errno) << std::endl;
        }
    }
}