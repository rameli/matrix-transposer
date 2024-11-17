#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "futex/FutexSignaller.h"

static std::thread gPartnerThread;

static std::unique_ptr<FutexSignaller> pFutexWaiter;
static std::unique_ptr<FutexSignaller> pFutexWaker;

uint32_t uniqueId;

static void DoSetup(const benchmark::State& state)
{
    uniqueId = getpid();

    pFutexWaiter  = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, "");
    pFutexWaker = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waker,  "");

    gPartnerThread = std::thread([]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        pFutexWaker->m_RawPointer->store(1, std::memory_order_release);
    });
}

static void DoTeardown(const benchmark::State& state)
{
    gPartnerThread.join();
    
    pFutexWaker.reset();
    pFutexWaiter.reset();
    
}

static void BM_Futex(benchmark::State& state)
{
    for (auto _ : state)
    {
        // pFutexWaiter->Wait();
        while (pFutexWaiter->m_RawPointer->load(std::memory_order_relaxed) == 0)
        {
        }
    }
}
BENCHMARK(BM_Futex)
    // ->Iterations(2)
    ->Setup(DoSetup)->Teardown(DoTeardown);