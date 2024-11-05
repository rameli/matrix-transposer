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

#include "ipc/Futex.h"

static constexpr size_t CACHE_LINE_SIZE = 64;

Futex::Futex(uint32_t uniqueId, Endpoint endpoint) : 
    m_UniqueId(uniqueId),
    m_Endpoint(endpoint)
{
    m_ShmObjectName = CreateShmObjectName(m_UniqueId);

    m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_FileDescriptor == -1)
    {
        throw std::runtime_error("Failed to create shared memory object for ID: " + std::to_string(m_UniqueId) +
            "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    // Set size of shared memory object
    if (ftruncate(m_FileDescriptor, CACHE_LINE_SIZE) == -1)
    {
        close(m_FileDescriptor);
        throw std::runtime_error("Failed to set size of shared memory for ID: " + std::to_string(m_UniqueId) + "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    // Map shared memory into process address space
    void* ptr = mmap(nullptr, CACHE_LINE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);

    if (ptr == MAP_FAILED)
    {
        close(m_FileDescriptor);
        throw std::runtime_error("Failed to mmap shared memory for futex for ID: " + std::to_string(m_UniqueId) + "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    // Since the pointer points to a shared memory object, we do not need to delete it.
    m_RawPointer = new (ptr) std::atomic<uint32_t>(0);

    // According to https://man7.org/linux/man-pages/man3/shm_open.3.html
    // "After a call to mmap(2) the file descriptor may be closed without affecting the memory mapping."
    close(m_FileDescriptor);
}

std::string Futex::CreateShmObjectName(uint32_t uniqueId)
{
    std::ostringstream oss;
    oss << "transpose_client_uid{" << uniqueId << "}_futex";
    return oss.str();
}

Futex::~Futex() {
    if (m_RawPointer != MAP_FAILED && m_RawPointer != nullptr)
    {
        munmap((void *)m_RawPointer, CACHE_LINE_SIZE);
    }

    // Only the client should unlink the shared memory object
    if (m_Endpoint == Endpoint::CLIENT)
    {
        shm_unlink(m_ShmObjectName.c_str());
    }
}

void Futex::Wait()
{
    uint32_t one = 1;
    while (1)
    {
        // We need to rely on low-level CPU primitives to ensure atomicity across multiple processors/caches
        if (m_RawPointer->compare_exchange_strong(one, 0))
        {
            break;
        }

        long res = syscall(SYS_futex, m_RawPointer, FUTEX_WAIT, 0, nullptr, nullptr, 0);
        if (res == -1 && errno != EAGAIN)
        {
            throw std::runtime_error("Failed to wait on futex");
        }
    }
}

void Futex::Wake()
{
    uint32_t zero = 0;

    // We need to rely on low-level CPU primitives to ensure atomicity across multiple processors/caches
    if (m_RawPointer->compare_exchange_strong(zero, 1))
    {
        int res = syscall(SYS_futex, m_RawPointer, FUTEX_WAKE, 1, nullptr, nullptr, 0);
        if (res == -1)
        {
            throw std::runtime_error("Failed to wake futex");
        }
    }
}
