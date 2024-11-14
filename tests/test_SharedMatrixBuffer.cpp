#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include "matrix-buf/SharedMatrixBuffer.h"

static std::string CreateSharedMatrixBufferShmFilename(uint32_t ownerPid, uint32_t k, const std::string& nameSuffix)
{
    std::ostringstream oss;
    oss << "mat_buff_uid{" << ownerPid << "}_k{" << k << "}" << nameSuffix;

    return oss.str();
}

static bool ShmObjectExists(uint32_t uniqueId, size_t k)
{
    std::string expectedName = CreateSharedMatrixBufferShmFilename(uniqueId, k, "");

    std::filesystem::path SHM_DIR = "/dev/shm/";
    std::filesystem::path filePath = SHM_DIR / expectedName;
    return std::filesystem::exists(filePath);
}

static std::vector<size_t> gRowCountExponents = { 4, 5, 6};
static std::vector<size_t> gColumnCountExponents = { 4, 5, 6};

TEST(MatrixBufferTestSuite, CorrectInit)
{
    uint32_t uniqueId = getpid();
    
    for (size_t m : gRowCountExponents)
    {
        for (size_t n : gColumnCountExponents)
        {
            for (size_t k = 0; k < 10; k++)
            {
                std::unique_ptr<SharedMatrixBuffer> pMatrixBuffer;
                
                ASSERT_NO_THROW(pMatrixBuffer = std::make_unique<SharedMatrixBuffer>(uniqueId, SharedMatrixBuffer::Endpoint::Client, m, n, k, SharedMatrixBuffer::BufferInitMode::Zero, ""));

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

TEST(MatrixBufferTestSuite, AccessSharedBuffer)
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
                std::unique_ptr<SharedMatrixBuffer> pMatrixBuffer;
                
                ASSERT_NO_THROW(pMatrixBuffer = std::make_unique<SharedMatrixBuffer>(uniqueId, SharedMatrixBuffer::Endpoint::Client, m, n, k, SharedMatrixBuffer::BufferInitMode::Zero, ""));
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

TEST(MatrixBufferTestSuite, SimultaneousAccessSingleThread)
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
        std::vector<std::unique_ptr<SharedMatrixBuffer>> pMatrixBuffersVec;

        for (size_t n : gColumnCountExponents)
        {
            for (size_t m : gRowCountExponents)
            {
                // Create multiple objects to access the shared memory
                pMatrixBuffersVec.clear();

                // The first object is the client (creates/destroys the shared memory object)
                std::unique_ptr<SharedMatrixBuffer> pMatrixBufferClient;
                ASSERT_NO_THROW(pMatrixBufferClient = std::make_unique<SharedMatrixBuffer>(uniqueId, SharedMatrixBuffer::Endpoint::Client, m, n, K, SharedMatrixBuffer::BufferInitMode::Zero, ""));
                EXPECT_NE(pMatrixBufferClient, nullptr);
                pMatrixBuffersVec.push_back(std::move(pMatrixBufferClient));

                // Subsequent objects are servers (access the shared memory object)
                for (uint32_t matrixBufferIndex = 1; matrixBufferIndex < numSimultaneousAccess; matrixBufferIndex++)
                {
                    std::unique_ptr<SharedMatrixBuffer> pMatrixBuffer;
                    ASSERT_NO_THROW(pMatrixBuffer = std::make_unique<SharedMatrixBuffer>(uniqueId, SharedMatrixBuffer::Endpoint::Server, m, n, K, SharedMatrixBuffer::BufferInitMode::Zero, ""));
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