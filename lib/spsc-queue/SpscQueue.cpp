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

#include "FutexSignaller.h"

using std::string;

static constexpr uint32_t DEFAULT_CACHE_LINE_SIZE = 64;

FutexSignaller::FutexSignaller(uint32_t ownerPid, Role role, const std::string& nameSuffix) : 
    m_OwnerPid(ownerPid),
    m_Role(role)
{
    SharedMemory::Ownership ownership = (role == Role::Waiter) ? SharedMemory::Ownership::Owner : SharedMemory::Ownership::Borrower;
    SharedMemory::BufferInitMode bufferInitMode = role == Role::Waiter ? SharedMemory::BufferInitMode::Zero : SharedMemory::BufferInitMode::NoInit;
    string shmObjectName = CreateShmObjectName(m_OwnerPid, nameSuffix);
    size_t bufferSizeInBytes = GetCacheLineSize(DEFAULT_CACHE_LINE_SIZE);

    mp_SharedMemory = std::make_unique<SharedMemory>(bufferSizeInBytes, shmObjectName, ownership, bufferInitMode);
    void* ptr = mp_SharedMemory->GetRawPointer();

    m_RawPointer = new (ptr) std::atomic<uint32_t>(0);
}

std::string FutexSignaller::CreateShmObjectName(uint32_t uniqueId, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "futex_uid{" << uniqueId << "}" << nameSuffix;
    return oss.str();
}

FutexSignaller::~FutexSignaller()
{
}

void FutexSignaller::Wait()
{
    while (1)
    {
        uint32_t expected = 1; // Reset expected value in each iteration as compare_exchange_strong modifies it if the condition fails
        if (m_RawPointer->compare_exchange_strong(expected, 0))
        {
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
    return (m_RawPointer->load(std::memory_order_acquire) == 0);
}

void FutexSignaller::Wake()
{
    uint32_t expected = 0;

    // Attempt to release the lock by setting m_RawPointer to 0 (unlocked).
    if (m_RawPointer->compare_exchange_strong(expected, 1))
    {
        // Wake up one waiting thread if any.
        int res = syscall(SYS_futex, m_RawPointer, FUTEX_WAKE, 1, nullptr, nullptr, 0);
        if (res == -1)
        {
            std::cout << "FUTEX_WAKE error(" << errno << "): " << strerror(errno) << std::endl;
        }
    }
}

uint32_t FutexSignaller::GetCacheLineSize(uint32_t defaultSize)
{
    const std::string CACHE_LINE_SIZE_FILE_PATH = "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size";
    
    // Attempt to open the file
    std::ifstream file(CACHE_LINE_SIZE_FILE_PATH);
    if (!file.is_open()) {
        std::cerr << "Unable to open cache line size file: " << CACHE_LINE_SIZE_FILE_PATH << std::endl;
        return defaultSize;
    }
    
    // Read the cache line size from the file
    uint32_t cacheLineSize;
    file >> cacheLineSize;
    file.close();

    if (cacheLineSize <= 0) {
        std::cerr << "Failed to retrieve a valid cache line size." << std::endl;
        return defaultSize;
    }

    return cacheLineSize;
}