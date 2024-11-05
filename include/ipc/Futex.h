#pragma once

#include <cstdint>
#include <string>

class Futex {
public:
    Futex(uint32_t uniqueId);
    ~Futex();

    void wait();
    void wake();

private:
    std::string CreateShmObjectName(uint32_t uniqueId);
    void InitializeSharedMemory();
    void unlink();

    uint32_t m_UniqueId;
    std::string m_ShmObjectName;
    int m_FileDescriptor;
    std::atomic<uint32_t> *m_RawPointer;
};
