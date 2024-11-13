#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "SharedMatrixBuffer.h"

using std::ostringstream;
using std::string;

SharedMatrixBuffer::SharedMatrixBuffer(uint32_t ownerPid, Endpoint endpoint, uint32_t m, uint32_t n, uint32_t k, std::string suffix) :
    m_NumRows(1UL << m),
    m_NumColumns(1UL << n),
    m_BufferIndex(k),
    m_OwnerPid(ownerPid),
    m_Endpoint(endpoint)
{
    string shmObjectName = CreateShmObjectName(m_OwnerPid, m_BufferIndex, suffix);
    SharedMemory::Ownership ownership = (m_Endpoint == Endpoint::Client) ? SharedMemory::Ownership::Owner : SharedMemory::Ownership::Borrower;

    // Calculate size: 2^m * 2^n * sizeof(uint64_t)
    size_t bufferSizeInBytes = m_NumRows * m_NumColumns * sizeof(uint64_t);
    mp_SharedMemory = std::make_unique<SharedMemory>(bufferSizeInBytes, shmObjectName, ownership);
}

SharedMatrixBuffer::~SharedMatrixBuffer()
{
}

uint64_t* SharedMatrixBuffer::GetRawPointer() const
{
    if (mp_SharedMemory == nullptr)
    {
        return nullptr;
    }

    return static_cast<uint64_t*>(mp_SharedMemory->GetRawPointer());
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
    if (mp_SharedMemory == nullptr)
    {
        return "";
    }
    
    return mp_SharedMemory->GetName();
}

uint32_t SharedMatrixBuffer::GetElementCount() const
{
    return m_NumRows * m_NumColumns;
}

size_t SharedMatrixBuffer::GetBufferSizeInBytes() const
{
    if (mp_SharedMemory == nullptr)
    {
        return 0;
    }

    return mp_SharedMemory->GetBufferSizeInBytes();
}

std::string SharedMatrixBuffer::CreateShmObjectName(uint32_t uniqueId, uint32_t k, std::string suffix)
{
    std::ostringstream oss;
    oss << "mat_buff_uid{" << uniqueId << "}_k{" << k << "}" << suffix;

    return oss.str();
}
