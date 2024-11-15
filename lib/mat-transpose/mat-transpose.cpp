#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>

struct Block
{
    uint32_t i_start, i_end;
    uint32_t j_start, j_end;
};

void TransposeNaive(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount)
{
    for (uint32_t i = 0; i < rowCount; i++)
    {
        for (uint32_t j = 0; j < colCount; j++)
        {
            dst[j * rowCount + i] = src[i * colCount + j];
        }
    }
}

void TransposeNaiveInPlace(uint64_t* matrix, uint32_t rowCount)
{
    for (uint32_t i = 0; i < rowCount; i++)
    {
        for (uint32_t j = i + 1; j < rowCount; j++)
        {
            std::swap(matrix[i * rowCount + j], matrix[j * rowCount + i]);
        }
    }
}

void TransposeTiledMultiThreaded(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads)
{
    uint32_t numBlocksInRow = (rowCount + tileSize - 1) / tileSize;
    uint32_t numBlocksInCol = (colCount + tileSize - 1) / tileSize;
    std::vector<Block> blocks;

    // Prepare the list of blocks
    for (uint32_t bi = 0; bi < numBlocksInRow; bi++)
    {
        for (uint32_t bj = 0; bj < numBlocksInCol; bj++)
        {
            uint32_t i_start = bi * tileSize;
            uint32_t j_start = bj * tileSize;
            uint32_t i_end = std::min(i_start + tileSize, rowCount);
            uint32_t j_end = std::min(j_start + tileSize, colCount);
            blocks.push_back({i_start, i_end, j_start, j_end});
        }
    }

    if (numThreads <= 0)
    {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0)
        {
            numThreads = 8;
        }
    }

    std::vector<std::thread> threads(numThreads);

    for (uint32_t t = 0; t < numThreads; t++)
    {
        threads[t] = std::thread([&, t]()
        {
            for (size_t idx = t; idx < blocks.size(); idx += numThreads)
            {
                Block& block = blocks[idx];
                for (uint32_t i = block.i_start; i < block.i_end; i++)
                {
                    for (uint32_t j = block.j_start; j < block.j_end; j++)
                    {
                        dst[j * rowCount + i] = src[i * colCount + j];
                    }
                }
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }
}

void TransposeTiledInPlaceMultiThreaded(uint64_t* matrix, uint32_t rowCount, uint32_t tileSize, uint32_t numThreads)
{
    uint32_t numBlocks = (rowCount + tileSize - 1) / tileSize;
    std::vector<std::vector<Block>> threadBlocks(numThreads);

    // Assign blocks to threads ensuring no overlaps
    uint32_t blockCount = 0;
    for (uint32_t bi = 0; bi < numBlocks; bi++)
    {
        for (uint32_t bj = bi; bj < numBlocks; bj++)
        { 
            // Include diagonal blocks
            uint32_t t = blockCount % numThreads;
            uint32_t i_start = bi * tileSize;
            uint32_t j_start = bj * tileSize;
            uint32_t i_end = std::min(i_start + tileSize, rowCount);
            uint32_t j_end = std::min(j_start + tileSize, rowCount);

            threadBlocks[t].push_back({i_start, i_end, j_start, j_end});

            blockCount++;
        }
    }

    if (numThreads <= 0)
    {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0)
        {
            numThreads = 4;
        }
    }

    std::vector<std::thread> threads(numThreads);

    // Assign blocks to threads
    for (uint32_t t = 0; t < numThreads; t++)
    {
        threads[t] = std::thread([&, t](){
            for (const auto& block : threadBlocks[t])
            {
                uint32_t i_start = block.i_start;
                uint32_t i_end = block.i_end;
                uint32_t j_start = block.j_start;
                uint32_t j_end = block.j_end;

                if (i_start == j_start)
                {
                    // Diagonal block
                    for (uint32_t i = i_start; i < i_end; i++)
                    {
                        for (uint32_t j = i + 1; j < j_end; j++)
                        {
                            std::swap(matrix[i * rowCount + j], matrix[j * rowCount + i]);
                        }
                    }
                }
                else
                {
                    // Off-diagonal blocks
                    for (uint32_t i = i_start; i < i_end; i++)
                    {
                        for (uint32_t j = j_start; j < j_end; j++)
                        {
                            std::swap(matrix[i * rowCount + j], matrix[j * rowCount + i]);
                        }
                    }
                }
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }
}

// Recursive cache-oblivious transpose for rectangular matrices (out-of-place)
void transpose_recursive(uint64_t* src, uint64_t* dst, uint32_t src_row, uint32_t src_col, uint32_t dst_row, uint32_t dst_col, uint32_t M, uint32_t N, uint32_t src_stride, uint32_t dst_stride)
{
    if (M <= 16 && N <= 16)
    {
        // Base case: transpose small block directly
        for (uint32_t i = 0; i < M; i++)
        {
            for (uint32_t j = 0; j < N; j++)
            {
                dst[(dst_row + j) * dst_stride + dst_col + i] =
                    src[(src_row + i) * src_stride + src_col + j];
            }
        }
    }
    else if (M >= N)
    {
        // Split along rows in src, adjust dst_col accordingly
        uint32_t M2 = M / 2;
        transpose_recursive(src, dst, src_row, src_col, dst_row, dst_col, M2, N, src_stride, dst_stride);
        transpose_recursive(src, dst, src_row + M2, src_col, dst_row, dst_col + M2, M - M2, N, src_stride, dst_stride);
    }
    else
    {
        // Split along columns in src, adjust dst_row accordingly
        uint32_t N2 = N / 2;
        transpose_recursive(src, dst, src_row, src_col, dst_row, dst_col, M, N2, src_stride, dst_stride);
        transpose_recursive(src, dst, src_row, src_col + N2, dst_row + N2, dst_col, M, N - N2, src_stride, dst_stride);
    }
}

void TransposeRecursive(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount)
{
    transpose_recursive(src, dst, 0, 0, 0, 0, rowCount, colCount, colCount, rowCount);
}

// Recursive cache-oblivious in-place transpose for square matrices
void transposeRecursiveInPlace(uint64_t* matrix, uint32_t row, uint32_t col, uint32_t size, uint32_t stride)
{
    if (size <= 16)
    {
        // Base case: transpose small block directly
        for (uint32_t i = 0; i < size; i++)
        {
            for (uint32_t j = i + 1; j < size; j++)
            {
                std::swap(matrix[(row + i) * stride + col + j], matrix[(row + j) * stride + col + i]);
            }
        }
    }
    else
    {
        uint32_t size2 = size / 2;
        transposeRecursiveInPlace(matrix, row, col, size2, stride);
        transposeRecursiveInPlace(matrix, row + size2, col + size2, size - size2, stride);

        // Transpose the off-diagonal blocks
        for (uint32_t i = 0; i < size2; i++)
        {
            for (uint32_t j = 0; j < size - size2; j++)
            {
                std::swap(matrix[(row + i) * stride + col + size2 + j], matrix[(row + size2 + j) * stride + col + i]);
            }
        }
    }
}

void TransposeRecursiveInPlace(uint64_t* matrix, uint32_t rowCount)
{
    transposeRecursiveInPlace(matrix, 0, 0, rowCount, rowCount);
}

// Verify correctness by comparing two matrices
bool MatricesAreEqual(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount)
{
    // A and B are matrices of size rows x cols
    for (uint32_t i = 0; i < rowCount * colCount; i++)
    {
        if (src[i] != dst[i])
        {
            return false;
        }
    }
    return true;
}
