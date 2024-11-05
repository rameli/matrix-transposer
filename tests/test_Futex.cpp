#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include "ipc/Futex.h"

static std::string CreateFutexShmFilename(uint32_t uniqueId)
{
    std::ostringstream oss;
    oss << "transpose_client_uid{" << uniqueId << "}_futex";
    return oss.str();
}

static bool ShmObjectExists(uint32_t uniqueId)
{
    std::string expectedName = CreateFutexShmFilename(uniqueId);

    std::filesystem::path SHM_DIR = "/dev/shm/";
    std::filesystem::path filePath = SHM_DIR / expectedName;
    return std::filesystem::exists(filePath);
}


TEST(FutexTestSuite, CorrectInit)
{
    std::unique_ptr<Futex> pFutex;
    uint32_t uniqueId = getpid();
    ASSERT_NO_THROW(pFutex = std::make_unique<Futex>(uniqueId));
    EXPECT_EQ(ShmObjectExists(uniqueId), true);

}

TEST(FutexTestSuite, CorrectInitAndDestroy)
{
    std::unique_ptr<Futex> pFutex;
    uint32_t uniqueId = getpid();
    
    // Create the futex
    ASSERT_NO_THROW(pFutex = std::make_unique<Futex>(uniqueId));
    EXPECT_EQ(ShmObjectExists(uniqueId), true);

    // Destroy the futex
    ASSERT_NO_THROW(pFutex.reset());
    EXPECT_EQ(ShmObjectExists(uniqueId), false);
}

TEST(FutexTestSuite, TwoThreads)
{
    bool futexSignalled = false;
    uint32_t uniqueId = getpid();

    auto futexWait = [uniqueId, &futexSignalled]() {
        std::unique_ptr<Futex> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<Futex>(uniqueId));

        EXPECT_EQ(futexSignalled, false);
        pFutex->wait();
        EXPECT_EQ(futexSignalled, true);
    };

    auto futexWake = [uniqueId, &futexSignalled]() {
        std::unique_ptr<Futex> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<Futex>(uniqueId));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        futexSignalled = true;
        pFutex->wake();
    };

    std::thread tWait(futexWait);
    std::thread tWake(futexWake);

    tWait.join();
    tWake.join();
}

TEST(FutexTestSuite, TwoProcesses)
{
    uint32_t uniqueId;
    bool *pFutexSignalledFlag = nullptr;

    uniqueId = getpid(); // PID of the parent process

    pFutexSignalledFlag = static_cast<bool*>(mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    ASSERT_NE(pFutexSignalledFlag, MAP_FAILED) << "Failed to mmap shared memory for futex signalled flag";

    // Initialize the flag
    *pFutexSignalledFlag = false;

    // Fork to create child and parent processes
    pid_t pid = fork();

    if (pid == 0) {
        // Child process - Signaler
        std::unique_ptr<Futex> pFutex;
        pFutex = std::make_unique<Futex>(uniqueId);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        *pFutexSignalledFlag = true;
        pFutex->wake();
    } else {
        // Parent process - Waiter
        std::unique_ptr<Futex> pFutex;
        ASSERT_NO_THROW(pFutex = std::make_unique<Futex>(uniqueId));

        EXPECT_EQ((*pFutexSignalledFlag), false);
        pFutex->wait();
        EXPECT_EQ((*pFutexSignalledFlag), true);
    }
}