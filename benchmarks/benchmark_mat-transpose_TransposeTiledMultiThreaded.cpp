#include <benchmark/benchmark.h>
#include <cstdint>
#include <sstream>

#include "mat-transpose/mat-transpose.h"

static uint64_t* originalMat;
static uint64_t* transposeRes;
static uint32_t rowCount;
static uint32_t columnCount;
static uint32_t tileSize;
static uint32_t numThreads;

static void DoSetup(const benchmark::State& state)
{
    uint32_t m = state.range(0);
    uint32_t n = state.range(1);
    tileSize = state.range(2);
    numThreads = state.range(3);

    rowCount = 1 << m;
    columnCount = 1 << n;

    originalMat = new uint64_t[rowCount * columnCount];
    transposeRes = new uint64_t[columnCount * rowCount];

    for (uint64_t i = 0; i < rowCount * columnCount; ++i)
    {
        originalMat[i] = i;
    }

    // TransposeTiledMultiThreaded_setup(originalMat, transposeRes, rowCount, columnCount, tileSize, numThreads);

}

static void DoTeardown(const benchmark::State& state)
{
    // TransposeTiledMultiThreaded_teardown();

    delete[] originalMat;
    delete[] transposeRes;
}

static void BM_TransposeTiledMultiThreaded(benchmark::State& state)
{
    for (auto _ : state)
    {
        TransposeTiledMultiThreaded(originalMat, transposeRes, rowCount, columnCount, tileSize, numThreads);
    }
}

// Set up the benchmark ranges and add names for each argument
BENCHMARK(BM_TransposeTiledMultiThreaded)
    ->Setup(DoSetup)
    ->Teardown(DoTeardown)
    ->ArgsProduct({{10},
                   {10},
                   {32},
                   {8}})
    ->ArgNames({"m", "n", "tileSize", "numThreads"})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
