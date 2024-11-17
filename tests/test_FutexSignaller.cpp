#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include "futex/FutexSignaller.h"

static std::string CreateFutexShmFilename(uint32_t uniqueId, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "futex_uid{" << uniqueId << "}" << nameSuffix;
    return oss.str();
}

static bool ShmObjectExists(uint32_t uniqueId)
{
    std::string expectedName = CreateFutexShmFilename(uniqueId, "");

    std::filesystem::path SHM_DIR = "/dev/shm/";
    std::filesystem::path filePath = SHM_DIR / expectedName;
    return std::filesystem::exists(filePath);
}

TEST(FutexTestSuite, SingleInit)
{
    std::unique_ptr<FutexSignaller> pFutex;
    uint32_t uniqueId = getpid();
    ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));
    EXPECT_EQ(ShmObjectExists(uniqueId), true);
}

TEST(FutexTestSuite, InitAndDestroy)
{
    std::unique_ptr<FutexSignaller> pFutex;
    uint32_t uniqueId = getpid();
    
    // Create the futex
    ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));
    EXPECT_EQ(ShmObjectExists(uniqueId), true);

    // Destroy the futex
    ASSERT_NO_THROW(pFutex.reset());
    EXPECT_EQ(ShmObjectExists(uniqueId), false);
}

TEST(FutexTestSuite, MultipleInitAndDestroy)
{
    std::unique_ptr<FutexSignaller> pFutex;
    uint32_t uniqueId = getpid();
    
    for (int i = 0; i < 100; i++)
    {
        // Create the futex
        ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));
        EXPECT_EQ(ShmObjectExists(uniqueId), true);

        // Destroy the futex
        ASSERT_NO_THROW(pFutex.reset());
        EXPECT_EQ(ShmObjectExists(uniqueId), false);
    }
}

TEST(FutexTestSuite, TwoThreads)
{
    bool futexSignalled = false;
    uint32_t uniqueId = getpid();

    auto futexWait = [uniqueId, &futexSignalled]()
    {
        std::unique_ptr<FutexSignaller> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));

        EXPECT_EQ(futexSignalled, false);
        pFutex->Wait();
        EXPECT_EQ(futexSignalled, true);
    };

    auto futexWake = [uniqueId, &futexSignalled]()
    {
        std::unique_ptr<FutexSignaller> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waker, ""));

        futexSignalled = true;
        pFutex->Wake();
    };

    std::thread tWait(futexWait);
    // Wait for the waiter to create the futex
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::thread tWake(futexWake);

    tWait.join();
    tWake.join();
}

TEST(FutexTestSuite, TwoProcesses)
{
    uint32_t uniqueId;
    std::atomic<bool> *pFutexSignalledFlag = nullptr;

    uniqueId = getpid(); // PID of the parent process (waits for signal from child)

    auto ptr = mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(pFutexSignalledFlag, MAP_FAILED) << "Failed to mmap shared memory for futex signalled flag";

    pFutexSignalledFlag = new (ptr) std::atomic<bool>(false);

    // Initialize the flag
    pFutexSignalledFlag->store(false, std::memory_order_relaxed);

    // Fork to create child and parent processes
    pid_t pid = fork();

    ASSERT_GE(pid, 0) << "Failed to fork process";

    if (pid == 0)
    {
        // Child process - Signaler
        std::unique_ptr<FutexSignaller> pFutex;
        pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waker, "");

        // Wait for the waiter to create the futex
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        pFutexSignalledFlag->store(true, std::memory_order_relaxed);
        pFutex->Wake();

        exit(0);
    }
    else
    {
        // Parent process - Waiter
        std::unique_ptr<FutexSignaller> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));

        EXPECT_EQ(pFutexSignalledFlag->load(std::memory_order_relaxed), false);
        pFutex->Wait();
        EXPECT_EQ(pFutexSignalledFlag->load(std::memory_order_relaxed), true);

        pFutexSignalledFlag->store(false, std::memory_order_relaxed);
        munmap(ptr, sizeof(bool));
    }
}