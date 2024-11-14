#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include <gtest/gtest.h>
#include "MemoryUtils.h"

TEST(MemoryUtilsTestSuite, CacheLineSize)
{
    std::size_t cacheLineSize = MemoryUtils::GetCacheLineSize();
    // Expect the cache line size to be a reasonable value (usually a multiple of 8)
    EXPECT_GT(cacheLineSize, 0);
    EXPECT_TRUE((cacheLineSize % 8) == 0) << "Cache line size should be a multiple of 8 bytes";
}

TEST(MemoryUtilsTestSuite, L1DataCacheSize)
{
    std::size_t l1DataCacheSize = MemoryUtils::GetL1DataCacheSize();
    // Expect a positive non-zero L1 data cache size (typically in KB range)
    EXPECT_GT(l1DataCacheSize, 0);
}

TEST(MemoryUtilsTestSuite, L1InstructionCacheSize)
{
    std::size_t l1InstructionCacheSize = MemoryUtils::GetL1InstructionCacheSize();
    // Expect a positive non-zero L1 instruction cache size (typically in KB range)
    EXPECT_GT(l1InstructionCacheSize, 0);
}

TEST(MemoryUtilsTestSuite, L2CacheSize)
{
    std::size_t l2CacheSize = MemoryUtils::GetL2CacheSize();
    // Expect a positive non-zero L2 cache size (usually larger than L1 cache)
    EXPECT_GT(l2CacheSize, 0);
    EXPECT_GE(l2CacheSize, MemoryUtils::GetL1DataCacheSize());
}

TEST(MemoryUtilsTestSuite, L3CacheSize)
{
    std::size_t l3CacheSize = MemoryUtils::GetL3CacheSize();
    // Expect a positive non-zero L3 cache size (if present, usually larger than L2 cache)
    if (l3CacheSize > 0) {
        EXPECT_GE(l3CacheSize, MemoryUtils::GetL2CacheSize());
    }
}

TEST(MemoryUtilsTestSuite, TotalMemory)
{
    std::size_t totalMemory = MemoryUtils::GetTotalMemory();
    // Expect total memory to be a reasonably large value
    EXPECT_GT(totalMemory, 0);
}

TEST(MemoryUtilsTestSuite, FreeMemory)
{
    std::size_t freeMemory = MemoryUtils::GetFreeMemory();
    // Expect free memory to be less than or equal to total memory
    EXPECT_GE(MemoryUtils::GetTotalMemory(), freeMemory);
}

TEST(MemoryUtilsTestSuite, UsedMemory)
{
    std::size_t usedMemory = MemoryUtils::GetUsedMemory();
    // Expect used memory to be less than or equal to total memory
    EXPECT_GE(MemoryUtils::GetTotalMemory(), usedMemory);
    // Free + used memory should approximately equal total memory
    EXPECT_NEAR(MemoryUtils::GetTotalMemory(), MemoryUtils::GetFreeMemory() + usedMemory, 1024 * 1024);
}
