#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>

#include "spsc-queue/SpscQueueSeqLock.h"

static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueueSeqLock> pQueue;

static void DoSetup(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueueSeqLock>(getpid(), SpscQueueSeqLock::Role::Producer, CAPACITY, "");
}

static void DoSetupDeque(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueueSeqLock>(getpid(), SpscQueueSeqLock::Role::Producer, CAPACITY, "");

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

static void BM_SpscQueueSeqLock_Enque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueSeqLock_Enque)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);

static void BM_SpscQueueSeqLock_Deque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueSeqLock_Deque)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);


static void BM_SpscQueueSeqLock_EnqueueDeque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueSeqLock_EnqueueDeque)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);
