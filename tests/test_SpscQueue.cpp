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

    // Queue should be empty now
    uint32_t item;
    ASSERT_FALSE(queue.Dequeue(item));
}

TEST(SpscQueueTestSuite, TwoProcesses)
{
    constexpr size_t CAPACITY = 1000;

    uint32_t producerPid = getpid();
    pid_t pid = fork();
    if (pid == -1)
    {
        FAIL() << "Failed to fork process";
    }
    else if (pid == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Wait for producer to start
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
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        exit(0);
    }
    else
    {
        SpscQueue queue(producerPid, SpscQueue::Role::Producer, CAPACITY, "");

        for (int i = 0; i < CAPACITY; i++)
        {
            while (!queue.Enqueue(i))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        int status;
        waitpid(pid, &status, 0);
        ASSERT_TRUE(WIFEXITED(status));
        ASSERT_EQ(WEXITSTATUS(status), 0);
    }
}

// // Test case 3: Attempt to Enqueue when the queue is full
// TEST(SpscQueueTestSuite, TestQueueFull) {
//     cleanup_shm(); // Ensure no leftover shared memory

//     SpscQueue queue(shm_name, capacity, true);

//     // Fill the queue
//     for (int i = 0; i < capacity - 1; ++i) {
//         ASSERT_TRUE(queue.Enqueue(i));
//     }

//     // Attempt to Enqueue one more item
//     ASSERT_FALSE(queue.Enqueue(999));

//     cleanup_shm();
// }

// // Test case 4: Attempt to Dequeue when the queue is empty
// TEST(SpscQueueTestSuite, TestQueueEmpty) {
//     cleanup_shm(); // Ensure no leftover shared memory

//     SpscQueue queue(shm_name, capacity, true);

//     uint32_t item;
//     ASSERT_FALSE(queue.Dequeue(item));

//     cleanup_shm();
// }

// // Test case 5: Enqueue and Dequeue multiple items across processes
// TEST(SpscQueueTestSuite, TestMultipleItems) {
//     cleanup_shm(); // Ensure no leftover shared memory

//     pid_t pid = fork();
//     if (pid == -1) {
//         FAIL() << "Failed to fork process";
//     } else if (pid == 0) {
//         // Child process - Consumer
//         SpscQueue queue(shm_name, capacity, false);

//         int received = 0;
//         int expected_total = 100;
//         uint32_t item;
//         while (received < expected_total) {
//             if (queue.Dequeue(item)) {
//                 EXPECT_EQ(item, received);
//                 ++received;
//             } else {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1));
//             }
//         }
//         exit(0);
//     } else {
//         // Parent process - Producer
//         SpscQueue queue(shm_name, capacity, true);

//         int total_items = 100;
//         for (int i = 0; i < total_items; ++i) {
//             while (!queue.Enqueue(i)) {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(1));
//             }
//         }

//         // Wait for child process to finish
//         int status;
//         waitpid(pid, &status, 0);
//         ASSERT_TRUE(WIFEXITED(status));
//         ASSERT_EQ(WEXITSTATUS(status), 0);

//         cleanup_shm();
//     }
// }

