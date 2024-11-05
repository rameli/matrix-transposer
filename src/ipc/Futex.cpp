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

// Constructor: Opens or creates a shared memory object
Futex::Futex(uint32_t uniqueId) : 
    m_UniqueId(uniqueId)
{
    m_ShmObjectName = CreateShmObjectName(m_UniqueId);

    // Open or create shared memory
    m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_FileDescriptor == -1)
    {
        throw std::runtime_error("Failed to create shared memory object for ID: " + std::to_string(m_UniqueId) +
            "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    // Set size of shared memory object
    if (ftruncate(m_FileDescriptor, CACHE_LINE_SIZE) == -1)
    {
        unlink();
        throw std::runtime_error("Failed to set size of shared memory for ID: " + std::to_string(m_UniqueId) +
            "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    // Map shared memory into process address space
    void* ptr = mmap(nullptr, CACHE_LINE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);

    if (ptr == MAP_FAILED)
    {
        unlink(); // Clean up before throwing
        throw std::runtime_error("Failed to mmap shared memory for futex for ID: " + std::to_string(m_UniqueId) +
            "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    m_RawPointer = new (ptr) std::atomic<uint32_t>(0);
}

std::string Futex::CreateShmObjectName(uint32_t uniqueId)
{
    std::ostringstream oss;
    oss << "transpose_client_uid{" << uniqueId << "}_futex";
    return oss.str();
}

// Destructor: Unlink shared memory object
Futex::~Futex() {
    if (m_RawPointer != MAP_FAILED && m_RawPointer != nullptr)
    {
        munmap((void *)m_RawPointer, CACHE_LINE_SIZE);
    }
    unlink();
}

// Wait for the futex signal
void Futex::wait()
{
    uint32_t one = 1;
    while (1)
    {
        // if (std::atomic_compare_exchange_strong(m_RawPointer, &one, 0))
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

// Wake the futex
void Futex::wake()
{
    uint32_t zero = 0;
    if (m_RawPointer->compare_exchange_strong(zero, 1))
    {
        int res = syscall(SYS_futex, m_RawPointer, FUTEX_WAKE, 1, nullptr, nullptr, 0);
        if (res == -1)
        {
            throw std::runtime_error("Failed to wake futex");
        }
    }
}

void Futex::unlink()
{
    if (m_FileDescriptor != -1)
    {
        shm_unlink(m_ShmObjectName.c_str());
        close(m_FileDescriptor);
    }
}