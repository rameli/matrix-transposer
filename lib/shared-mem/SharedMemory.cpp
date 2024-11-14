#include <cstdint>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "SharedMemory.h"

using std::ostringstream;

SharedMemory::SharedMemory(size_t sizeInBytes, const std::string name, Ownership ownership, BufferInitMode initMode) :
    m_FileDescriptor(-1),
    m_RawPointer(nullptr),
    m_SizeInBytes(sizeInBytes),
    m_ShmObjectName(name),
    m_Ownership(ownership)
{
    if (m_Ownership == Ownership::Owner)
    {
        m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);

        if (m_FileDescriptor < 0)
        {
            ostringstream oss;
            oss << "Failed to create shared memory object for " << name << ". errno(" << errno << "): " << strerror(errno);
            std::cerr << oss.str() << std::endl;
            throw std::runtime_error(oss.str());
        }

        if (ftruncate(m_FileDescriptor, m_SizeInBytes) < 0)
        {
            close(m_FileDescriptor);
            shm_unlink(m_ShmObjectName.c_str());
            
            ostringstream oss;
            oss << "Failed to set shared memory size (" << m_SizeInBytes << " bytes) for " << name << ". errno(" << errno << "): " << strerror(errno);
            std::cerr << oss.str() << std::endl;
            throw std::runtime_error(oss.str());
        }
    }
    else if (m_Ownership == Ownership::Borrower)
    {
        m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR, 0666);

        if (m_FileDescriptor < 0)
        {
            ostringstream oss;
            oss << "Failed to create shared memory object for " << name << ". errno(" << errno << "): " << strerror(errno);
            std::cerr << oss.str() << std::endl;
            throw std::runtime_error(oss.str());
        }
    }

    m_RawPointer = mmap(0, m_SizeInBytes, PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);
    if (m_RawPointer == MAP_FAILED || m_RawPointer == nullptr)
    {
        close(m_FileDescriptor);
        shm_unlink(m_ShmObjectName.c_str());
        
        ostringstream oss;
        oss << "Failed to map shared memory for " << name << ". errno (" << errno << "): " << strerror(errno);
        std::cerr << oss.str() << std::endl;
        throw std::runtime_error(oss.str());
    }

    // According to https://man7.org/linux/man-pages/man3/shm_open.3.html
    // "After a call to mmap(2) the file descriptor may be closed without affecting the memory mapping."
    close(m_FileDescriptor);

    if (initMode == BufferInitMode::Zero && m_Ownership == Ownership::Owner)
    {
        FillWithZero();
    }
}

SharedMemory::~SharedMemory()
{
    if (m_RawPointer != MAP_FAILED && m_RawPointer != nullptr)
    {
        munmap(m_RawPointer, m_SizeInBytes);
    }

    if (m_Ownership == Ownership::Owner)
    {
        shm_unlink(m_ShmObjectName.c_str());
    }

    m_RawPointer = nullptr;
    m_SizeInBytes = 0;
    m_FileDescriptor = -1;
    m_ShmObjectName.clear();
}

void* SharedMemory::GetRawPointer() const
{
    return m_RawPointer;
}

const std::string& SharedMemory::GetName() const
{
    return m_ShmObjectName;
}

size_t SharedMemory::GetBufferSizeInBytes() const
{
    return m_SizeInBytes;
}

void SharedMemory::FillWithZero()
{
    
    memset(m_RawPointer, 0, GetBufferSizeInBytes());
}