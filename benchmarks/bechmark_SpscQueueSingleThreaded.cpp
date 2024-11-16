#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>

#include "spsc-queue/SpscQueue.h"

static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueue> pQueue;

static void DoSetup(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueue>(getpid(), SpscQueue::Role::Producer, CAPACITY, "");
}

static void DoSetupDeque(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueue>(getpid(), SpscQueue::Role::Producer, CAPACITY, "");

    uint32_t numItems = state.range(0);

    for (int i = 0; i < numItems; i++)
    {
        pQueue->Enqueue(i);
    }
}

static void DoTeardown(const benchmark::State& state)
{
    pQueue.reset();
}

// ============================================================================

static void BM_SpscQueueEnque(benchmark::State& state)
{
    uint32_t numItems = state.range(0);

    for (auto _ : state)
    {
        for (int i = 0; i < numItems; i++)
        {
            pQueue->Enqueue(i);
        }
    }
}
BENCHMARK(BM_SpscQueueEnque)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgName("Item Count")
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(1000'000);


static void BM_SpscQueueDeque(benchmark::State& state)
{
    uint32_t numItems = state.range(0);

    for (auto _ : state)
    {
        for (int i = 0; i < numItems; i++)
        {
            uint32_t item;
            pQueue->Dequeue(item);
            benchmark::DoNotOptimize(item);
        }
    }
}
BENCHMARK(BM_SpscQueueDeque)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgName("Item Count")
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(1000'000);


static void BM_SpscQueueEnqueueDeque(benchmark::State& state)
{
    uint32_t numItems = state.range(0);

    for (auto _ : state)
    {
        for (int i = 0; i < numItems; i++)
        {
            uint32_t item;
            pQueue->Enqueue(i);
            pQueue->Dequeue(item);
            benchmark::DoNotOptimize(item);
        }
    }
}
BENCHMARK(BM_SpscQueueEnqueueDeque)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgName("Item Count")
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(1000'000);