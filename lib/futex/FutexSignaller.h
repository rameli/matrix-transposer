#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include "SharedMemory.h"

class FutexSignaller {
public:
    enum class Role
    {
        Waker,
        Waiter
    };

    FutexSignaller(uint32_t ownerPid, Role role, const std::string& nameSuffix);
    ~FutexSignaller();

    const std::string& GetName() const;
    void Wait();
    bool IsWaiting();

    void Wake();

private:
    static std::string CreateShmObjectName(uint32_t ownerPid, const std::string& nameSuffix);

    uint32_t m_OwnerPid;
    std::atomic<uint32_t> *m_RawPointer;
    Role m_Role;

    std::unique_ptr<SharedMemory> mp_SharedMemory;

};
