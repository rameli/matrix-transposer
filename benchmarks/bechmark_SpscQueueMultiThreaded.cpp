#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <thread>

#include "spsc-queue/SpscQueue.h"

static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueue> pQueue;

static std::thread gConsumerThread;

static void DoSetup(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueue>(getpid(), SpscQueue::Role::Producer, CAPACITY, "");
    gConsumerThread = std::thread([&]()
    {
        uint32_t item;
        for (int i = 0; i < state.range(0); i++)
        {
            pQueue->Dequeue(item);
            benchmark::DoNotOptimize(item);
        }
    });
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
        for (int i = 0; i < numItems; i++)
        {
            pQueue->Enqueue(i);
        }

        gConsumerThread.join();
    }
}
BENCHMARK(BM_SpscQueueEnqueueDeque)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgName("Item Count")
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(1000'000);