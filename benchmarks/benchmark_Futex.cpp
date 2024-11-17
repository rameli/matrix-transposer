#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <thread>

#include "futex/FutexSignaller.h"

static std::thread gPartnerThread;

static std::unique_ptr<FutexSignaller> pSelfWaitFutex;
static std::unique_ptr<FutexSignaller> pSelfWakeFutex;

static std::unique_ptr<FutexSignaller> pOtherWaitFutex;
static std::unique_ptr<FutexSignaller> pOtherWakeFutex;

uint32_t uniqueId;


static void DoSetup(const benchmark::State& state)
{
    uniqueId = getpid();

    pSelfWaitFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, "_f1");
    pOtherWakeFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waker,  "_f1");

    pOtherWaitFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waiter, "_f2");
    pSelfWakeFutex = std::make_unique<FutexSignaller>(uniqueId, FutexSignaller::Role::Waker,  "_f2");


    gPartnerThread = std::thread([]
    {
        pOtherWaitFutex->Wait();
        pOtherWakeFutex->Wake();
    });

}

static void DoTeardown(const benchmark::State& state)
{
    gPartnerThread.join();
    
    pSelfWaitFutex.reset();
    pSelfWakeFutex.reset();
    pOtherWaitFutex.reset();
    pOtherWakeFutex.reset();
}

static void BM_SpscQueueSeqLock_MultiThreaded_EnqueueDeque(benchmark::State& state)
{
    volatile uint32_t numItems;

    for (auto _ : state)
    {
        pSelfWakeFutex->Wake();
        pSelfWaitFutex->Wait();
    }
}
BENCHMARK(BM_SpscQueueSeqLock_MultiThreaded_EnqueueDeque)
    ->Setup(DoSetup)->Teardown(DoTeardown);