#include <cstdint>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "SharedMatrixBuffer.h"


SharedMatrixBuffer::SharedMatrixBuffer(uint32_t uniqueId, size_t m, size_t n, size_t k) :
    m_FileDescriptor(-1),
    m_RawPointer(nullptr),
    m_NumRows(1UL << m),
    m_NumColumns(1UL << n),
    m_BufferIndex(k),
    m_UniqueId(uniqueId)
{
    // Generate the shared memory name based on the process ID and index
    m_ShmObjectName = CreateShmObjectName(m_UniqueId, m_BufferIndex);

    // Calculate size: 2^m * 2^n * sizeof(uint64_t)
    m_BufferBytes = m_NumRows * m_NumColumns * sizeof(uint64_t);

    // Create shared memory object
    m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_FileDescriptor == -1)
    {
        throw std::runtime_error("Failed to create shared memory object for ID: " + std::to_string(m_UniqueId) +
            ", m: " + std::to_string(m) +
            ", n: " + std::to_string(n) +
            ", k: " + std::to_string(k) + 
            "errno(" + std::to_string(errno) + "): " + std::string(strerror(errno)));
    }

    // Set the size of the shared memory object
    if (ftruncate(m_FileDescriptor, m_BufferBytes) == -1)
    {
        unlink(); // Clean up before throwing
        throw std::runtime_error("Failed to set shared memory size");
    }

    // Map the shared memory object
    m_RawPointer = mmap(0, m_BufferBytes, PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);
    if (m_RawPointer == MAP_FAILED)
    {
        unlink(); // Clean up before throwing
        throw std::runtime_error("Failed to map shared memory");
    }
}

SharedMatrixBuffer::~SharedMatrixBuffer()
{
    if (m_RawPointer != MAP_FAILED && m_RawPointer != nullptr)
    {
        munmap(m_RawPointer, m_BufferBytes); // Unmap shared memory
    }
    unlink(); // Unlink the shared memory object
}

void* SharedMatrixBuffer::GetRawPointer() const
{
    return m_RawPointer; // Return the pointer to the shared memory
}

size_t SharedMatrixBuffer::RowCount() const
{
    return m_NumRows; // Return number of rows
}

size_t SharedMatrixBuffer::ColumnCount() const
{
    return m_NumColumns; // Return number of columns
}

std::string SharedMatrixBuffer::GetName() const
{
    return m_ShmObjectName; // Return the shared memory name
}

size_t SharedMatrixBuffer::GetElementCount() const
{
    return m_NumRows * m_NumColumns;
}

size_t SharedMatrixBuffer::GetBufferSizeInBytes() const
{
    return m_BufferBytes;
}

std::string SharedMatrixBuffer::CreateShmObjectName(uint32_t uniqueId, size_t k)
{
    std::ostringstream oss;
    oss << "transpose_client_uid{" << uniqueId << "}_k{" << k << "}";
    return oss.str();
}


void SharedMatrixBuffer::unlink()
{
    if (m_FileDescriptor != -1)
    {
        shm_unlink(m_ShmObjectName.c_str());
        close(m_FileDescriptor);
    }
}
