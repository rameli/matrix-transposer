#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <thread>

#include "spsc-queue/SpscQueueRingBuffer.h"

static constexpr size_t CAPACITY = 1024*1024;
static std::unique_ptr<SpscQueueRingBuffer> pQueue;

static std::thread gConsumerThread;

static void DoSetup(const benchmark::State& state)
{
    pQueue = std::make_unique<SpscQueueRingBuffer>(getpid(), SpscQueueRingBuffer::Role::Producer, CAPACITY, "");
    uint32_t numItems = state.range(0);
    
    gConsumerThread = std::thread([&]()
    {
        uint32_t item;
        for (int i = 0; i < numItems; i++)
        {
            while(!pQueue->Dequeue(item))
            {
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            }
            benchmark::DoNotOptimize(item);
        }
    });
}

static void DoTeardown(const benchmark::State& state)
{
    gConsumerThread.join();
    pQueue.reset();
}

static void BM_SpscQueueRingBuffer_MultiThreaded_EnqueueDeque(benchmark::State& state)
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
BENCHMARK(BM_SpscQueueRingBuffer_MultiThreaded_EnqueueDeque)
    ->Setup(DoSetup)->Teardown(DoTeardown)
    ->ArgName("Item Count")->Arg(1)->Arg(1000)->Arg(1000'000);
