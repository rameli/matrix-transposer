#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "SharedMatrixBuffer.h"

using std::string;

SharedMatrixBuffer::SharedMatrixBuffer(uint32_t ownerPid, Endpoint endpoint, uint32_t m, uint32_t n, uint32_t k, BufferInitMode initMode, const std::string& nameSuffix) :
    m_NumRows(1UL << m),
    m_NumColumns(1UL << n),
    m_BufferIndex(k),
    m_OwnerPid(ownerPid),
    m_Endpoint(endpoint)
{
    SharedMemory::Ownership ownership = (m_Endpoint == Endpoint::Client) ? SharedMemory::Ownership::Owner : SharedMemory::Ownership::Borrower;
    SharedMemory::BufferInitMode bufferInitMode = initMode == BufferInitMode::Zero ? SharedMemory::BufferInitMode::Zero : SharedMemory::BufferInitMode::NoInit;
    string shmObjectName = CreateShmObjectName(m_OwnerPid, m_BufferIndex, nameSuffix);

    // Calculate size: 2^m * 2^n * sizeof(uint64_t)
    size_t bufferSizeInBytes = m_NumRows * m_NumColumns * sizeof(uint64_t);


    mp_SharedMemory = std::make_unique<SharedMemory>(bufferSizeInBytes, shmObjectName, ownership, bufferInitMode);

    if (initMode == BufferInitMode::Random)
    {
        FillWithRandom();
    }
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

const std::string& SharedMatrixBuffer::GetName() const
{
    static const string EMPTY_STRING = "";
    if (mp_SharedMemory == nullptr)
    {
        return EMPTY_STRING;
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

std::string SharedMatrixBuffer::CreateShmObjectName(uint32_t ownerPid, uint32_t k, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "mat_buff_uid{" << ownerPid << "}_k{" << k << "}" << nameSuffix;

    return oss.str();
}

void SharedMatrixBuffer::FillWithRandom()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t* region = GetRawPointer();

    for (size_t i = 0; i < GetElementCount(); ++i) {
        region[i] = dist(gen);
    }
}

