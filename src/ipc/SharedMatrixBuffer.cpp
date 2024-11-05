#include <cstdint>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "ipc/SharedMatrixBuffer.h"


SharedMatrixBuffer::SharedMatrixBuffer(uint32_t uniqueId, uint32_t m, uint32_t n, uint32_t k, Endpoint endpoint) :
    m_FileDescriptor(-1),
    m_RawPointer(nullptr),
    m_NumRows(1UL << m),
    m_NumColumns(1UL << n),
    m_BufferIndex(k),
    m_UniqueId(uniqueId),
    m_Endpoint(endpoint)
{
    // Generate the shared memory name based on the process ID and index
    m_ShmObjectName = CreateShmObjectName(m_UniqueId, m_BufferIndex);

    // Calculate size: 2^m * 2^n * sizeof(uint64_t)
    m_BufferBytes = m_NumRows * m_NumColumns * sizeof(uint64_t);

    // Only the client should initialize the shared memory object for the futex.
    // If the object already exists, unlink it and try again.
    if (m_Endpoint == Endpoint::CLIENT)
    {
        // Client opens the mutex shared memory object with the O_EXCL flag
        m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);

        if (m_FileDescriptor < 0)
        {
            if (errno == EEXIST) {
                shm_unlink(m_ShmObjectName.c_str());
                m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
            }
            if (m_FileDescriptor < 0) {
                if (m_FileDescriptor == -1)
                {
                    throw std::runtime_error("Failed to create shared memory object for ID: " + std::to_string(m_UniqueId) +
                        ", m: " + std::to_string(m) +
                        ", n: " + std::to_string(n) +
                        ", k: " + std::to_string(k) + 
                        "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
                }
            }
        }
    }
    else if (m_Endpoint == Endpoint::SERVER)
    {
        // Server opens the mutex shared memory object without the O_EXCL flag
        m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR, 0666);

        if (m_FileDescriptor < 0)
        {
            throw std::runtime_error("Failed to create shared memory object for ID: " + std::to_string(m_UniqueId) +
                ", m: " + std::to_string(m) +
                ", n: " + std::to_string(n) +
                ", k: " + std::to_string(k) + 
                "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
        }
    }

    // Set the size of the shared memory object
    if (ftruncate(m_FileDescriptor, m_BufferBytes) == -1)
    {
        // Destructor won't be called if an exception is thrown in the constructor
        close(m_FileDescriptor);
        throw std::runtime_error("Failed to set shared memory size");
    }

    // Map the shared memory object
    m_RawPointer = mmap(0, m_BufferBytes, PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);
    if (m_RawPointer == MAP_FAILED)
    {
        // Destructor won't be called if an exception is thrown in the constructor
        close(m_FileDescriptor);
        throw std::runtime_error("Failed to map shared memory");
    }

    // According to https://man7.org/linux/man-pages/man3/shm_open.3.html
    // "After a call to mmap(2) the file descriptor may be closed without affecting the memory mapping."
    close(m_FileDescriptor);
}

SharedMatrixBuffer::~SharedMatrixBuffer()
{
    if (m_RawPointer != MAP_FAILED && m_RawPointer != nullptr)
    {
        munmap(m_RawPointer, m_BufferBytes);
    }

    // Only the client should unlink the shared memory object
    if (m_Endpoint == Endpoint::CLIENT)
    {
        shm_unlink(m_ShmObjectName.c_str());
    }
}

uint64_t* SharedMatrixBuffer::GetRawPointer() const
{
    return static_cast<uint64_t*>(m_RawPointer);
}

uint32_t SharedMatrixBuffer::RowCount() const
{
    return m_NumRows;
}

uint32_t SharedMatrixBuffer::ColumnCount() const
{
    return m_NumColumns;
}

std::string SharedMatrixBuffer::GetName() const
{
    return m_ShmObjectName;
}

uint32_t SharedMatrixBuffer::GetElementCount() const
{
    return m_NumRows * m_NumColumns;
}

size_t SharedMatrixBuffer::GetBufferSizeInBytes() const
{
    return m_BufferBytes;
}

std::string SharedMatrixBuffer::CreateShmObjectName(uint32_t uniqueId, uint32_t k)
{
    std::ostringstream oss;
    oss << "transpose_client_uid{" << uniqueId << "}_k{" << k << "}";
    return oss.str();
}
