#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <thread>

#include "spsc-queue/SpscQueue.h"
#include "futex/FutexSignaller.h"


static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueue> pQueue;
static std::unique_ptr<FutexSignaller> pFutex;

static uint32_t gProducerPid;
static bool gIsProducer;

static void DoSetup(const benchmark::State& state)
{
    gProducerPid = getpid();
    pid_t pid = fork();
    if (pid < 0)
    {
        exit(1);
    }
    else if (pid == 0)
    {
        exit(0);
        // Consumer process (child)
        gIsProducer = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // pQueue = std::make_unique<SpscQueue>(gProducerPid, SpscQueue::Role::Consumer, CAPACITY, "");
        // pFutex = std::make_unique<FutexSignaller>(gProducerPid, FutexSignaller::Role::Waker, "");
        // pFutex->Wake();
    }
    else
    {
        // Producer process (parent)
        gIsProducer = true;
        pQueue = std::make_unique<SpscQueue>(gProducerPid, SpscQueue::Role::Producer, CAPACITY, "");
        pFutex = std::make_unique<FutexSignaller>(gProducerPid, FutexSignaller::Role::Waiter, "");
        // pFutex->Wait();
    }
}

static void DoTeardown(const benchmark::State& state)
{
    pQueue.reset();
}

// ============================================================================

static void BM_SpscQueueEnqueueDeque(benchmark::State& state)
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
            // pFutex->Wait();
        }
        else
        {
            // uint32_t item;
            // for (int i = 0; i < numItems; i++)
            // {
            //     pQueue->Dequeue(item);
            //     benchmark::DoNotOptimize(item);
            // }
            // pFutex->Wake();
            // _exit(0);
        }
    }
}
BENCHMARK(BM_SpscQueueEnqueueDeque)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgName("Item Count")
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(1000'000);