#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "spsc-queue/SpscQueue.h"

TEST(SpscQueueTestSuite, CorrectInit)
{
    std::unique_ptr<SpscQueue> pQueue;
    EXPECT_NO_THROW(pQueue = std::make_unique<SpscQueue>(getpid(), SpscQueue::Role::Producer, 10, ""));
}