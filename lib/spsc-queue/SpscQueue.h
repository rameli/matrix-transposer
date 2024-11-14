#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <memory>

#include "SharedMemory.h"
#include "FutexSignaller.h"

class SpscQueue
{
public:
    enum class Role
    {
        Producer,
        Consumer
    };

    SpscQueue(uint32_t ownerPid, Role role, size_t capacity, const std::string& nameSuffix);
    ~SpscQueue();

    const std::string& GetName() const;
    uint32_t GetCapacity() const;
    bool Enqueue(uint32_t item);
    bool Deque(uint32_t& item);

private:
    struct QueueData
    {
        alignas(64) std::atomic<size_t> head;
        alignas(64) std::atomic<size_t> tail;
        alignas(64) uint32_t buffer[];
    };

    size_t CalculateBufferSize();
    static std::string CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix);
    static std::string CreateFutexObjectName(uint32_t ownerPid, const std::string& nameSuffix);

    uint32_t m_OwnerPid;
    Role m_Role;
    size_t m_Capacity;

    QueueData* mp_QueueData;

    std::unique_ptr<FutexSignaller> mp_Futex;
    std::unique_ptr<SharedMemory> mp_SharedMemory;
};
