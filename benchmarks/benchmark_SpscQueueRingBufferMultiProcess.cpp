#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <thread>
#include <sys/wait.h>
#include <sys/types.h>

#include "spsc-queue/SpscQueueRingBuffer.h"
#include "futex/FutexSignaller.h"


static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueueRingBuffer> pQueue;
static std::unique_ptr<FutexSignaller> pFutex;

static uint32_t gProducerPid;
static uint32_t gConsumerPid;
static bool gIsProducer;

static void DoSetup(const benchmark::State& state)
{
    gProducerPid = getpid();
    gConsumerPid = fork();
    if (gConsumerPid < 0)
    {
        exit(1);
    }
    else if (gConsumerPid == 0)
    {
        // Consumer process (child)
        gIsProducer = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        pQueue = std::make_unique<SpscQueueRingBuffer>(gProducerPid, SpscQueueRingBuffer::Role::Consumer, CAPACITY, "");
        pFutex = std::make_unique<FutexSignaller>(gProducerPid, FutexSignaller::Role::Waker, "");
        pFutex->Wake();
    }
    else
    {
        // Producer process (parent)
        gIsProducer = true;
        pQueue = std::make_unique<SpscQueueRingBuffer>(gProducerPid, SpscQueueRingBuffer::Role::Producer, CAPACITY, "");
        pFutex = std::make_unique<FutexSignaller>(gProducerPid, FutexSignaller::Role::Waiter, "");
        pFutex->Wait();
    }
}

static void DoTeardown(const benchmark::State& state)
{
    if (gIsProducer)
    {
        int status;
        waitpid(gConsumerPid, &status, 0);
        pQueue.reset();
    }


}

// ============================================================================

static void BM_SpscQueueRingBufferEnqueueDeque(benchmark::State& state)
{
    uint32_t numItems = state.range(0);

    for (auto _ : state)
    {
        if (gIsProducer)
        {
            for (int i = 0; i < numItems; i++)
            {
                pQueue->Enqueue(i);
            }
            pFutex->Wait();
        }
        else
        {
            uint32_t item;
            for (int i = 0; i < numItems; i++)
            {
                pQueue->Dequeue(item);
                benchmark::DoNotOptimize(item);
            }
            pFutex->Wake();
            exit(0);
        }
    }
}
BENCHMARK(BM_SpscQueueRingBufferEnqueueDeque)
    ->Iterations(1)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgName("Item Count")
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(1000'000);