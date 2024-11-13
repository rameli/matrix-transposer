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


TEST(FutexTestSuite, CorrectInit)
{
    std::unique_ptr<FutexSignaller> pFutex;
    uint32_t uniqueId = getpid();
    ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));
    EXPECT_EQ(ShmObjectExists(uniqueId), true);
}

TEST(FutexTestSuite, CorrectInitAndDestroy)
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for the wait thread to start
    std::thread tWake(futexWake);

    tWait.join();
    tWake.join();
}

TEST(FutexTestSuite, TwoProcesses)
{
    uint32_t uniqueId;
    bool *pFutexSignalledFlag = nullptr;

    uniqueId = getpid(); // PID of the parent process (waits for signal from child)

    pFutexSignalledFlag = static_cast<bool*>(mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    ASSERT_NE(pFutexSignalledFlag, MAP_FAILED) << "Failed to mmap shared memory for futex signalled flag";

    // Initialize the flag
    *pFutexSignalledFlag = false;

    // Fork to create child and parent processes
    pid_t pid = fork();

    ASSERT_GE(pid, 0) << "Failed to fork process";

    if (pid == 0) {
        // Child process - Signaler
        std::unique_ptr<FutexSignaller> pFutex;
        pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waker, "");

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        *pFutexSignalledFlag = true;
        pFutex->Wake();
    } else {
        // Parent process - Waiter
        std::unique_ptr<FutexSignaller> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, ""));

        EXPECT_EQ((*pFutexSignalledFlag), false);
        pFutex->Wait();
        EXPECT_EQ((*pFutexSignalledFlag), true);
    }
}