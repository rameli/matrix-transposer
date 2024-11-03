#include <cstring>
#include <thread>
#include <filesystem>
#include <sys/mman.h>
#include <fcntl.h>
#include <memory>
#include <vector>
#include <gtest/gtest.h>

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

static std::vector<size_t> gRowCountExponents = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15};
static std::vector<size_t> gColumnCountExponents = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

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
    uint32_t uniqueId = getpid();
    uint64_t* pBufferPtr = nullptr;
    size_t numElements = 0;    
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
                    pBufferPtr[0] = 0x1234567890ABCDEF;
                    EXPECT_EQ(pBufferPtr[0], 0x1234567890ABCDEF);
                }
                else if (numElements == 2)
                {
                    pBufferPtr[0] = 0x1234567890ABCDEF;
                    pBufferPtr[numElements - 1] = 0xFEDCBA0987654321;

                    EXPECT_EQ(pBufferPtr[0], 0x1234567890ABCDEF);
                    EXPECT_EQ(pBufferPtr[numElements - 1], 0xFEDCBA0987654321);
                }
                else
                {
                    // More than four elements
                    pBufferPtr[0] = 0x1234567890ABCDEF;
                    pBufferPtr[numElements/2] = 0x1030507090A0C0E0;
                    pBufferPtr[numElements - 1] = 0xFEDCBA0987654321;

                    EXPECT_EQ(pBufferPtr[0], 0x1234567890ABCDEF);
                    EXPECT_EQ(pBufferPtr[numElements/2], 0x1030507090A0C0E0);
                    EXPECT_EQ(pBufferPtr[numElements - 1], 0xFEDCBA0987654321);
                }
            }
        }
    }
}

TEST(MatrixBufferExceptionTest, SimultaneousAccess)
{
    uint32_t uniqueId = getpid();
    uint64_t* pBufferPtr = nullptr;
    size_t numElements = 0;
    constexpr size_t k = 0;

    for (uint32_t numSimultaneousAccess = 1; numSimultaneousAccess < 10; numSimultaneousAccess++)
    {
        std::vector<std::unique_ptr<MatrixBuffer>> pMatrixBuffers;

        for (size_t n : gColumnCountExponents)
        {
            for (size_t m : gRowCountExponents)
            {
                for (uint32_t matrixBufferIndex = 0; matrixBufferIndex < numSimultaneousAccess; matrixBufferIndex++)
                {
                    ASSERT_NO_THROW(pMatrixBuffers[matrixBufferIndex] = std::make_unique<MatrixBuffer>(uniqueId, m, n, k));
                    EXPECT_NE(pMatrixBuffers[matrixBufferIndex], nullptr);
                }
                
                // First object writes to the shared memory
                pBufferPtr = static_cast<uint64_t*>(pMatrixBuffers[0]->GetRawPointer());
                EXPECT_NE(pBufferPtr, nullptr);
                EXPECT_NE(pBufferPtr, MAP_FAILED);

                numElements = pMatrixBuffers[0]->GetElementCount();
                EXPECT_EQ(numElements, (1UL << m) * (1UL << n));

                if (numElements == 1)
                {
                    EXPECT_NE(pBufferPtr[0], 0x1234567890ABCDEF);
                    pBufferPtr[0] = 0x1234567890ABCDEF;
                }
                else if (numElements == 2)
                {
                    EXPECT_NE(pBufferPtr[0], 0x1234567890ABCDEF);
                    EXPECT_NE(pBufferPtr[numElements - 1], 0xFEDCBA0987654321);

                    pBufferPtr[0] = 0x1234567890ABCDEF;
                    pBufferPtr[numElements - 1] = 0xFEDCBA0987654321;
                }
                else
                {
                    EXPECT_NE(pBufferPtr[0], 0x1234567890ABCDEF);
                    EXPECT_NE(pBufferPtr[numElements/2], 0x1030507090A0C0E0);
                    EXPECT_NE(pBufferPtr[numElements - 1], 0xFEDCBA0987654321);

                    // More than four elements
                    pBufferPtr[0] = 0x1234567890ABCDEF;
                    pBufferPtr[numElements/2] = 0x1030507090A0C0E0;
                    pBufferPtr[numElements - 1] = 0xFEDCBA0987654321;
                }

                // All objects read from the shared memory
                for (uint32_t matrixBufferIndex = 0; matrixBufferIndex < numSimultaneousAccess; matrixBufferIndex++)
                {
                    pBufferPtr = static_cast<uint64_t*>(pMatrixBuffers[matrixBufferIndex]->GetRawPointer());
                    EXPECT_NE(pBufferPtr, nullptr);
                    EXPECT_NE(pBufferPtr, MAP_FAILED);

                    numElements = pMatrixBuffers[matrixBufferIndex]->GetElementCount();
                    EXPECT_EQ(numElements, (1UL << m) * (1UL << n));

                    if (numElements == 1)
                    {
                        EXPECT_EQ(pBufferPtr[0], 0x1234567890ABCDEF);
                    }
                    else if (numElements == 2)
                    {
                        EXPECT_EQ(pBufferPtr[0], 0x1234567890ABCDEF);
                        EXPECT_EQ(pBufferPtr[numElements - 1], 0xFEDCBA0987654321);
                    }
                    else
                    {
                        EXPECT_EQ(pBufferPtr[0], 0x1234567890ABCDEF);
                        EXPECT_EQ(pBufferPtr[numElements/2], 0x1030507090A0C0E0);
                        EXPECT_EQ(pBufferPtr[numElements - 1], 0xFEDCBA0987654321);
                    }
                }
            }
        }
    }
}