#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <sstream>

class SharedMemory
{
public:
    enum class Ownership
    {
        Owner,
        Borrower
    };

    SharedMemory(size_t sizeInBytes, const std::string name, Ownership ownership);
    ~SharedMemory();

    void* GetRawPointer() const;
    const std::string& GetName() const;
    size_t GetBufferSizeInBytes() const;

private:
    std::string m_ShmObjectName;
    size_t m_SizeInBytes;
    int m_FileDescriptor;
    void* m_RawPointer;
    Ownership m_Ownership;
};
