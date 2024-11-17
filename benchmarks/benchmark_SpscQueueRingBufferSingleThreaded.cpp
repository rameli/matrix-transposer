#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>

#include "spsc-queue/SpscQueueRingBuffer.h"

static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueueRingBuffer> pQueue;

static void DoSetup(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueueRingBuffer>(getpid(), SpscQueueRingBuffer::Role::Producer, CAPACITY, "");
}

static void DoSetupDeque(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueueRingBuffer>(getpid(), SpscQueueRingBuffer::Role::Producer, CAPACITY, "");

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

static void BM_SpscQueueRingBufferEnque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueRingBufferEnque)
    ->Iterations(1)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);

static void BM_SpscQueueRingBufferDeque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueRingBufferDeque)
    ->Iterations(1)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);


static void BM_SpscQueueRingBufferEnqueueDeque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueRingBufferEnqueueDeque)
    ->Iterations(1)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);
