#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include "MatrixBuffer.h"

static std::string CreateShmFilename(uint32_t uniqueId, size_t k)
{
    std::ostringstream oss;
    oss << "transpose_client_uid{" << uniqueId << "}_k{" << k << "}";
    return oss.str();
}

static bool ShmObjectExists(uint32_t uniqueId, size_t k)
{
    std::string expectedName = CreateShmFilename(uniqueId, k);

    std::filesystem::path SHM_DIR = "/dev/shm/";
    std::filesystem::path filePath = SHM_DIR / expectedName;
    return std::filesystem::exists(filePath);
}

static std::vector<size_t> gRowCountExponents = {0, 1, 2, 3, 10, 15, 20};
static std::vector<size_t> gColumnCountExponents = {0, 1, 2, 3, 10, 15, 20};

TEST(MatrixBufferExceptionTest, CorrectInit)
{
    uint32_t uniqueId = getpid();
    
    for (size_t m : gRowCountExponents)
    {
        for (size_t n : gColumnCountExponents)
        {
            for (size_t k = 0; k < 10; k++)
            {
                std::unique_ptr<MatrixBuffer> pMatrixBuffer;
                
                ASSERT_NO_THROW(pMatrixBuffer = std::make_unique<MatrixBuffer>(uniqueId, m, n, k));

                EXPECT_NE(pMatrixBuffer->GetRawPointer(), nullptr);
                EXPECT_NE(pMatrixBuffer->GetRawPointer(), MAP_FAILED);
                EXPECT_EQ(ShmObjectExists(uniqueId, k), true);

                EXPECT_EQ(pMatrixBuffer->RowCount(), 1UL << m);
                EXPECT_EQ(pMatrixBuffer->GetBufferSizeInBytes(), (1UL << m) * (1UL << n) * sizeof(uint64_t));
                EXPECT_EQ(pMatrixBuffer->GetElementCount(), (1UL << m) * (1UL << n));
                EXPECT_EQ(pMatrixBuffer->ColumnCount(), 1UL << n);
            }
        }
    }
}

TEST(MatrixBufferExceptionTest, AccessSharedBuffer)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    
    uint32_t uniqueId = getpid();
    uint64_t* pBufferPtr = nullptr;
    size_t numElements = 0;

    uint64_t FIRST_ELEMENT_VALUE = dist(gen);
    uint64_t MID_ELEMENT_VALUE = dist(gen);
    uint64_t LAST_ELEMENT_VALUE = dist(gen);

    for (size_t k = 0; k < 10; k++)
    {
        for (size_t n : gColumnCountExponents)
        {
            for (size_t m : gRowCountExponents)
            {
                std::unique_ptr<MatrixBuffer> pMatrixBuffer;
                
                ASSERT_NO_THROW(pMatrixBuffer = std::make_unique<MatrixBuffer>(uniqueId, m, n, k));
                EXPECT_NE(pMatrixBuffer, nullptr);

                pBufferPtr = static_cast<uint64_t*>(pMatrixBuffer->GetRawPointer());
                EXPECT_NE(pBufferPtr, nullptr);
                EXPECT_NE(pBufferPtr, MAP_FAILED);

                numElements = pMatrixBuffer->GetElementCount();
                EXPECT_EQ(numElements, (1UL << m) * (1UL << n));

                if (numElements == 1)
                {
                    pBufferPtr[0] = FIRST_ELEMENT_VALUE;
                    EXPECT_EQ(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                }
                else if (numElements == 2)
                {
                    pBufferPtr[0] = FIRST_ELEMENT_VALUE;
                    pBufferPtr[numElements - 1] = LAST_ELEMENT_VALUE;

                    EXPECT_EQ(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                    EXPECT_EQ(pBufferPtr[numElements - 1], LAST_ELEMENT_VALUE);
                }
                else
                {
                    // More than four elements
                    pBufferPtr[0] = FIRST_ELEMENT_VALUE;
                    pBufferPtr[numElements/2] = MID_ELEMENT_VALUE;
                    pBufferPtr[numElements - 1] = LAST_ELEMENT_VALUE;

                    EXPECT_EQ(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                    EXPECT_EQ(pBufferPtr[numElements/2], MID_ELEMENT_VALUE);
                    EXPECT_EQ(pBufferPtr[numElements - 1], LAST_ELEMENT_VALUE);
                }
            }
        }
    }
}

TEST(MatrixBufferExceptionTest, SimultaneousAccess)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    
    uint32_t uniqueId = getpid();
    uint64_t* pBufferPtr = nullptr;
    size_t numElements = 0;

    uint64_t FIRST_ELEMENT_VALUE = dist(gen);
    uint64_t MID_ELEMENT_VALUE = dist(gen);
    uint64_t LAST_ELEMENT_VALUE = dist(gen);

    constexpr size_t K = 0;

    for (uint32_t numSimultaneousAccess = 1; numSimultaneousAccess < 10; numSimultaneousAccess++)
    {
        std::vector<std::unique_ptr<MatrixBuffer>> pMatrixBuffersVec;

        for (size_t n : gColumnCountExponents)
        {
            for (size_t m : gRowCountExponents)
            {
                // Create multiple objects to access the shared memory
                pMatrixBuffersVec.clear();
                for (uint32_t matrixBufferIndex = 0; matrixBufferIndex < numSimultaneousAccess; matrixBufferIndex++)
                {
                    std::unique_ptr<MatrixBuffer> pMatrixBuffer;
                    ASSERT_NO_THROW(pMatrixBuffer = std::make_unique<MatrixBuffer>(uniqueId, m, n, K));
                    EXPECT_NE(pMatrixBuffer, nullptr);
                    pMatrixBuffersVec.push_back(std::move(pMatrixBuffer));
                }
                
                // First object writes to the shared memory
                pBufferPtr = static_cast<uint64_t*>(pMatrixBuffersVec[0]->GetRawPointer());
                EXPECT_NE(pBufferPtr, nullptr);
                EXPECT_NE(pBufferPtr, MAP_FAILED);

                numElements = pMatrixBuffersVec[0]->GetElementCount();
                EXPECT_EQ(numElements, (1UL << m) * (1UL << n));

                if (numElements == 1)
                {
                    // No randomly correct residual values from before
                    EXPECT_NE(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                    pBufferPtr[0] = FIRST_ELEMENT_VALUE;
                }
                else if (numElements == 2)
                {
                    // No randomly correct residual values from before
                    EXPECT_NE(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                    EXPECT_NE(pBufferPtr[numElements - 1], LAST_ELEMENT_VALUE);

                    pBufferPtr[0] = FIRST_ELEMENT_VALUE;
                    pBufferPtr[numElements - 1] = LAST_ELEMENT_VALUE;
                }
                else
                {
                    // No randomly correct residual values from before
                    EXPECT_NE(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                    EXPECT_NE(pBufferPtr[numElements/2], MID_ELEMENT_VALUE);
                    EXPECT_NE(pBufferPtr[numElements - 1], LAST_ELEMENT_VALUE);

                    // More than four elements
                    pBufferPtr[0] = FIRST_ELEMENT_VALUE;
                    pBufferPtr[numElements/2] = MID_ELEMENT_VALUE;
                    pBufferPtr[numElements - 1] = LAST_ELEMENT_VALUE;
                }

                // All objects read from the shared memory
                for (uint32_t matrixBufferIndex = 0; matrixBufferIndex < numSimultaneousAccess; matrixBufferIndex++)
                {
                    pBufferPtr = static_cast<uint64_t*>(pMatrixBuffersVec[matrixBufferIndex]->GetRawPointer());
                    EXPECT_NE(pBufferPtr, nullptr);
                    EXPECT_NE(pBufferPtr, MAP_FAILED);

                    numElements = pMatrixBuffersVec[matrixBufferIndex]->GetElementCount();
                    EXPECT_EQ(numElements, (1UL << m) * (1UL << n));

                    if (numElements == 1)
                    {
                        EXPECT_EQ(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                    }
                    else if (numElements == 2)
                    {
                        EXPECT_EQ(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                        EXPECT_EQ(pBufferPtr[numElements - 1], LAST_ELEMENT_VALUE);
                    }
                    else
                    {
                        EXPECT_EQ(pBufferPtr[0], FIRST_ELEMENT_VALUE);
                        EXPECT_EQ(pBufferPtr[numElements/2], MID_ELEMENT_VALUE);
                        EXPECT_EQ(pBufferPtr[numElements - 1], LAST_ELEMENT_VALUE);
                    }
                }
            }
        }
    }
}