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

#include "spsc-queue/SpscQueue.h"

TEST(SpscQueueTestSuite, SingleProcess)
{
    constexpr size_t CAPACITY = 1024*1024;

    SpscQueue queue(getpid(), SpscQueue::Role::Producer, CAPACITY, "");

    for (int i = 0; i < CAPACITY; ++i)
    {
        ASSERT_TRUE(queue.Enqueue(i));
    }

    ASSERT_FALSE(queue.Enqueue(CAPACITY));

    for (int i = 0; i < CAPACITY; i++)
    {
        uint32_t item;
        ASSERT_TRUE(queue.Dequeue(item));
        ASSERT_EQ(item, i);
    }

    uint32_t item;
    ASSERT_FALSE(queue.Dequeue(item));
}

TEST(SpscQueueTestSuite, TwoProcesses)
{
    constexpr size_t CAPACITY = 1024*1024;

    uint32_t producerPid = getpid();
    SpscQueue queue(producerPid, SpscQueue::Role::Producer, CAPACITY, "");

    pid_t pid = fork();
    if (pid == -1)
    {
        FAIL() << "Failed to fork process";
    }
    else if (pid == 0)
    {
        SpscQueue queue(producerPid, SpscQueue::Role::Consumer, CAPACITY, "");

        int received = 0;
        uint32_t item;
        while (received < CAPACITY)
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
        for (int i = 0; i < CAPACITY; i++)
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

TEST(SpscQueueTestSuite, TestQueueFull)
{
    constexpr size_t CAPACITY = 1024*1024;
    SpscQueue queue(getpid(), SpscQueue::Role::Producer, CAPACITY, "");

    for (int i = 0; i < CAPACITY; i++)
    {
        ASSERT_TRUE(queue.Enqueue(i));
    }

    ASSERT_FALSE(queue.Enqueue(0));
}

TEST(SpscQueueTestSuite, TestQueueEmpty)
{
    constexpr size_t CAPACITY = 1024*1024;
    SpscQueue queue(getpid(), SpscQueue::Role::Producer, CAPACITY, "");

    uint32_t item;
    ASSERT_FALSE(queue.Dequeue(item));
}

TEST(SpscQueueTestSuite, TestMultipleItems)
{
    uint32_t producerPid = getpid();
    constexpr size_t CAPACITY = 1024*1024;
    constexpr size_t TOTAL_ITEMS = 1234;

    SpscQueue producerQueue(producerPid, SpscQueue::Role::Producer, CAPACITY, "");

    pid_t pid = fork();
    if (pid == -1)
    {
        FAIL() << "Failed to fork process";
    }
    else if (pid == 0)
    {
        SpscQueue consumerQueue(producerPid, SpscQueue::Role::Consumer, CAPACITY, "");

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

