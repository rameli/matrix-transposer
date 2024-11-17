#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <cstdint>

#include "spsc-queue/SpscQueueSeqLock.h"

TEST(SpscQueueSeqLockTestSuite, SingleProcess)
{
    constexpr size_t CAPACITY = 1024*1024;
    size_t realCapacity;

    SpscQueueSeqLock queue(getpid(), SpscQueueSeqLock::Role::Producer, CAPACITY, "");
    realCapacity = queue.GetRealCapacity();

    for (int i = 0; i < realCapacity; i++)
    {
        ASSERT_TRUE(queue.Enqueue(i));
    }

    ASSERT_FALSE(queue.Enqueue(0));

    for (int i = 0; i < realCapacity; i++)
    {
        uint32_t item;
        ASSERT_TRUE(queue.Dequeue(item));
        ASSERT_EQ(item, i);
    }

    uint32_t item;
    ASSERT_FALSE(queue.Dequeue(item));
}

TEST(SpscQueueSeqLockTestSuite, TwoProcesses)
{
    constexpr size_t CAPACITY = 1024*1024;
    size_t realCapacity;

    uint32_t producerPid = getpid();
    SpscQueueSeqLock queue(producerPid, SpscQueueSeqLock::Role::Producer, CAPACITY, "");
    realCapacity = queue.GetRealCapacity();

    pid_t pid = fork();
    if (pid == -1)
    {
        FAIL() << "Failed to fork process";
    }
    else if (pid == 0)
    {
        SpscQueueSeqLock queue(producerPid, SpscQueueSeqLock::Role::Consumer, CAPACITY, "");

        int received = 0;
        uint32_t item;
        while (received < realCapacity)
        {
            if (queue.Dequeue(item))
            {
                EXPECT_EQ(item, received);
                received++;
            }
        }
        exit(0);
    }
    else
    {
        for (int i = 0; i < realCapacity; i++)
        {
            while (!queue.Enqueue(i))
            {
            }
        }

        int status;
        waitpid(pid, &status, 0);
        ASSERT_TRUE(WIFEXITED(status));
        ASSERT_EQ(WEXITSTATUS(status), 0);
    }
}

TEST(SpscQueueSeqLockTestSuite, TestQueueFull)
{
    constexpr size_t CAPACITY = 1024*1024;
    size_t realCapacity;

    SpscQueueSeqLock queue(getpid(), SpscQueueSeqLock::Role::Producer, CAPACITY, "");
    realCapacity = queue.GetRealCapacity();

    for (int i = 0; i < realCapacity; i++)
    {
        ASSERT_TRUE(queue.Enqueue(i));
    }

    ASSERT_FALSE(queue.Enqueue(0));
}

TEST(SpscQueueSeqLockTestSuite, TestQueueEmpty)
{
    constexpr size_t CAPACITY = 1024*1024;
    SpscQueueSeqLock queue(getpid(), SpscQueueSeqLock::Role::Producer, CAPACITY, "");

    uint32_t item;
    ASSERT_FALSE(queue.Dequeue(item));
}

TEST(SpscQueueSeqLockTestSuite, TestMultipleItems)
{
    uint32_t producerPid = getpid();
    constexpr size_t CAPACITY = 1024*1024;
    constexpr size_t TOTAL_ITEMS = 1234;

    SpscQueueSeqLock producerQueue(producerPid, SpscQueueSeqLock::Role::Producer, CAPACITY, "");

    pid_t pid = fork();
    if (pid == -1)
    {
        FAIL() << "Failed to fork process";
    }
    else if (pid == 0)
    {
        SpscQueueSeqLock consumerQueue(producerPid, SpscQueueSeqLock::Role::Consumer, CAPACITY, "");

        int received = 0;
        uint32_t item;
        while (received < TOTAL_ITEMS)
        {
            if (consumerQueue.Dequeue(item))
            {
                EXPECT_EQ(item, received);
                received++;
            }
        }
        exit(0);
    }
    else
    {
        for (int i = 0; i < TOTAL_ITEMS; i++)
        {
            while (!producerQueue.Enqueue(i))
            {
            }
        }

        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);
        ASSERT_TRUE(WIFEXITED(status));
        ASSERT_EQ(WEXITSTATUS(status), 0);
    }
}

