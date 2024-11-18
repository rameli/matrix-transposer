#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include "mat-transpose/mat-transpose.h"

TEST(MatTransposeTestSuite, Naive2x2)
{
    uint64_t src[] = {1, 2, 3, 4};
    uint64_t dst[4] = {0};
    uint64_t expected[] = {1, 3, 2, 4};

    TransposeNaive(src, dst, 2, 2);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 2, 2));
}

TEST(MatTransposeTestSuite, Naive3x2)
{
    uint64_t src[] = {1, 2, 3, 4, 5, 6};
    uint64_t dst[6] = {0};
    uint64_t expected[] = {1, 3, 5, 2, 4, 6};

    TransposeNaive(src, dst, 3, 2);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 2, 3));
}

TEST(MatTransposeTestSuite, Naive2x3)
{
    uint64_t src[] = {1, 2, 3, 4, 5, 6};
    uint64_t dst[6] = {0};
    uint64_t expected[] = {1, 4, 2, 5, 3, 6};

    TransposeNaive(src, dst, 2, 3);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 3, 2));
}

TEST(MatTransposeTestSuite, Naive4x4)
{
    uint64_t src[] = {1, 2, 3, 4,
                      5, 6, 7, 8,
                      9, 10, 11, 12,
                      13, 14, 15, 16};
    uint64_t dst[16] = {0};
    uint64_t expected[] = {1, 5, 9, 13,
                           2, 6, 10, 14,
                           3, 7, 11, 15,
                           4, 8, 12, 16};

    TransposeNaive(src, dst, 4, 4);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 4, 4));
}

TEST(MatTransposeTestSuite, Naive16x16)
{
    const int size = 16;
    std::vector<uint64_t> src(size * size);
    std::vector<uint64_t> dst(size * size, 0);
    std::vector<uint64_t> expected(size * size);

    // Initialize
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            src[i * size + j] = i * size + j + 1;
            expected[j * size + i] = i * size + j + 1;
        }
    }

    TransposeNaive(src.data(), dst.data(), size, size);

    EXPECT_TRUE(MatricesAreEqual(dst.data(), expected.data(), size, size));
}

TEST(MatTransposeTestSuite, Naive32x32)
{
    const int size = 32;
    std::vector<uint64_t> src(size * size);
    std::vector<uint64_t> dst(size * size, 0);
    std::vector<uint64_t> expected(size * size);

    // Initialize
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            src[i * size + j] = i * size + j + 1;
            expected[j * size + i] = i * size + j + 1;
        }
    }

    TransposeNaive(src.data(), dst.data(), size, size);

    EXPECT_TRUE(MatricesAreEqual(dst.data(), expected.data(), size, size));
}

TEST(MatTransposeTestSuite, Naive1x1)
{
    uint64_t src[] = {42};
    uint64_t dst[1] = {0};
    uint64_t expected[] = {42};

    TransposeNaive(src, dst, 1, 1);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 1, 1));
}

TEST(MatTransposeTestSuite, Naive1x5)
{
    uint64_t src[] = {10, 20, 30, 40, 50};
    uint64_t dst[5] = {0};
    uint64_t expected[] = {10, 20, 30, 40, 50};

    TransposeNaive(src, dst, 1, 5);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 5, 1));
}

TEST(MatTransposeTestSuite, Naive5x1)
{
    uint64_t src[] = {10, 20, 30, 40, 50};
    uint64_t dst[5] = {0};
    uint64_t expected[] = {10, 20, 30, 40, 50};

    TransposeNaive(src, dst, 5, 1);

    EXPECT_TRUE(MatricesAreEqual(dst, expected, 1, 5));
}

class TileMultiThreadedTest : public ::testing::TestWithParam<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> {};

TEST_P(TileMultiThreadedTest, TileMultiThreaded)
{
    uint32_t m = std::get<0>(GetParam());
    uint32_t n = std::get<1>(GetParam());
    uint32_t tileSize = std::get<2>(GetParam());
    uint32_t numThreads = std::get<3>(GetParam());

    uint32_t rowCount = 1 << m;
    uint32_t columnCount = 1 << n;

    uint64_t* originalMat = new uint64_t[rowCount * columnCount];  // Original matrix
    uint64_t* transposeRes = new uint64_t[columnCount * rowCount]; // Transposed matrix for algorithms
    uint64_t* refTranspose = new uint64_t[columnCount * rowCount]; // Transposed matrix from naive algorithm for verification

    for (uint64_t i = 0; i < rowCount * columnCount; ++i)
    {
        originalMat[i] = i;
    }


    TransposeNaive(originalMat, refTranspose, rowCount, columnCount);
    TransposeTiledMultiThreaded(originalMat, transposeRes, rowCount, columnCount, tileSize, numThreads);

    EXPECT_TRUE(MatricesAreEqual(transposeRes, refTranspose, columnCount, rowCount));

    delete[] originalMat;
    delete[] transposeRes;
    delete[] refTranspose;
}

INSTANTIATE_TEST_SUITE_P
(
    TileMultiThreadedTests,
    TileMultiThreadedTest,
    ::testing::Combine(
        ::testing::Values(10, 12), // m
        ::testing::Values(10, 12), // n
        ::testing::Values(32, 64, 128), // tileSize
        ::testing::Values(1, 2, 4, 8, 16, 32)  // numThreads
    )
);