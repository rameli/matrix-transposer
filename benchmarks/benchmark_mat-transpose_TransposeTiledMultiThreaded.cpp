#include <benchmark/benchmark.h>
#include <cstdint>

#include "mat-transpose/mat-transpose.h"

static uint64_t* originalMat;
static uint64_t* transposeRes;
static uint32_t rowCount;
static uint32_t columnCount;
static uint32_t tileSize;
static uint32_t numThreads;

static void DoSetup(const benchmark::State& state)
{
    uint32_t m = 4;
    uint32_t n = 4;

    rowCount = 1 << m;
    columnCount = 1 << n;
    tileSize = 32;
    numThreads = 8;

    originalMat = new uint64_t[rowCount * columnCount];
    transposeRes = new uint64_t[columnCount * rowCount];

    for (uint64_t i = 0; i < rowCount * columnCount; ++i)
    {
        originalMat[i] = i;
    }

}

static void DoTeardown(const benchmark::State& state)
{
        delete[] originalMat;
        delete[] transposeRes;
}

static void BM_TransposeTiledMultiThreaded(benchmark::State& state)
{
    for (auto _ : state)
    {
        TransposeTiledMultiThreadedOptimized(originalMat, transposeRes, rowCount, columnCount, tileSize, numThreads);
    }
}
BENCHMARK(BM_TransposeTiledMultiThreaded)->Setup(DoSetup)->Teardown(DoTeardown);

BENCHMARK_MAIN();