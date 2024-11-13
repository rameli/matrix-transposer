#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <memory>
#include <sstream>

#include "SharedMemory.h"

class SharedMatrixBuffer
{
public:
    enum class Endpoint
    {
        Server,
        Client
    };

    SharedMatrixBuffer(uint32_t ownerPid, Endpoint endpoint, uint32_t m, uint32_t n, uint32_t k, std::string suffix);
    
    ~SharedMatrixBuffer();

    uint64_t* GetRawPointer() const;
    std::string GetName() const;

    uint32_t RowCount() const;
    uint32_t ColumnCount() const;
    uint32_t GetElementCount() const;
    size_t GetBufferSizeInBytes() const;

private:
    static std::string CreateShmObjectName(uint32_t ownerPid, uint32_t k, std::string suffix);

    uint32_t m_OwnerPid;
    Endpoint m_Endpoint;
    uint32_t m_NumRows;
    uint32_t m_NumColumns;
    uint32_t m_BufferIndex;
    std::unique_ptr<SharedMemory> mp_SharedMemory;
};
