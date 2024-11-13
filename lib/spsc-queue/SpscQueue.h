#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include "SharedMemory.h"
#include "futex/FutexSignaller.h"

class SpscQueue {
public:
    enum class Role
    {
        Producer,
        Consumer
    };

    SpscQueue(uint32_t ownerPid, Role endponit, uint32_t capacity, const std::string& nameSuffix);
    ~SpscQueue();

    const std::string& GetName() const;
    bool Enqueue(bool blocking);
    bool Deque(bool blocking);

private:
    static std::string CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix);

    uint32_t m_OwnerPid;
    uint32_t *m_RawPointer;
    Role m_Role;

    std::unique_ptr<FutexSignaller> mp_Futex;
    std::unique_ptr<SharedMemory> mp_SharedMemory;
};
