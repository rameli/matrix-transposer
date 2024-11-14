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
    int i_start, i_end;
    int j_start, j_end;
};

void TransposeNaive(uint64_t* src, uint64_t* dst, int M, int N)
{
    // src is M x N
    // dst is N x M
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; ++j)
        {
            dst[j * M + i] = src[i * N + j];
        }
    }
}

// Naive single-threaded in-place transpose (for square matrices)
void TransposeNaiveInPlace(uint64_t* matrix, int N)
{
    for (int i = 0; i < N; ++i)
    {
        for (int j = i + 1; j < N; ++j)
        {
            std::swap(matrix[i * N + j], matrix[j * N + i]);
        }
    }
}

// Cache-friendly multi-threaded transpose using tiling (out-of-place)
void TransposeTiledMultiThreaded(uint64_t* src, uint64_t* dst, int M, int N, int B, int num_threads)
{
    int num_blocks_row = (M + B - 1) / B;
    int num_blocks_col = (N + B - 1) / B;
    std::vector<Block> blocks;

    // Prepare the list of blocks
    for (int bi = 0; bi < num_blocks_row; ++bi)
    {
        for (int bj = 0; bj < num_blocks_col; ++bj)
        {
            int i_start = bi * B;
            int j_start = bj * B;
            int i_end = std::min(i_start + B, M);
            int j_end = std::min(j_start + B, N);
            blocks.push_back({i_start, i_end, j_start, j_end});
        }
    }

    if (num_threads <= 0)
    {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
            num_threads = 4;  // Default to 4 threads if detection fails
    }

    std::vector<std::thread> threads(num_threads);

    // Assign blocks to threads
    for (int t = 0; t < num_threads; ++t)
    {
        threads[t] = std::thread([&, t]()
        {
            for (size_t idx = t; idx < blocks.size(); idx += num_threads)
            {
                Block& block = blocks[idx];
                for (int i = block.i_start; i < block.i_end; ++i)
                    for (int j = block.j_start; j < block.j_end; ++j)
                        dst[j * M + i] = src[i * N + j];
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }
}

// Corrected Cache-friendly multi-threaded in-place transpose using tiling (for square matrices)
void TransposeNaiveInPlaceMultiThreaded(uint64_t* matrix, int N, int B, int num_threads)
{
    int num_blocks = (N + B - 1) / B;
    std::vector<std::vector<Block>> thread_blocks(num_threads);

    // Assign blocks to threads ensuring no overlaps
    int block_count = 0;
    for (int bi = 0; bi < num_blocks; ++bi)
    {
        for (int bj = bi; bj < num_blocks; ++bj)
        { 
            // Include diagonal blocks
            int t = block_count % num_threads;
            int i_start = bi * B;
            int j_start = bj * B;
            int i_end = std::min(i_start + B, N);
            int j_end = std::min(j_start + B, N);

            thread_blocks[t].push_back({i_start, i_end, j_start, j_end});

            block_count++;
        }
    }

    if (num_threads <= 0)
    {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0)
        {
            num_threads = 4;
        }
    }

    std::vector<std::thread> threads(num_threads);

    // Assign blocks to threads
    for (int t = 0; t < num_threads; ++t)
    {
        threads[t] = std::thread([&, t](){
            for (const auto& block : thread_blocks[t])
            {
                int i_start = block.i_start;
                int i_end = block.i_end;
                int j_start = block.j_start;
                int j_end = block.j_end;

                if (i_start == j_start)
                {
                    // Diagonal block
                    for (int i = i_start; i < i_end; ++i)
                    {
                        for (int j = i + 1; j < j_end; ++j)
                        {
                            std::swap(matrix[i * N + j], matrix[j * N + i]);
                        }
                    }
                }
                else
                {
                    // Off-diagonal blocks
                    for (int i = i_start; i < i_end; ++i)
                    {
                        for (int j = j_start; j < j_end; ++j)
                        {
                            std::swap(matrix[i * N + j], matrix[j * N + i]);
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
void transpose_recursive(uint64_t* src, uint64_t* dst, int src_row, int src_col, int dst_row, int dst_col, int M, int N, int src_stride, int dst_stride)
{
    if (M <= 16 && N <= 16)
    {
        // Base case: transpose small block directly
        for (int i = 0; i < M; i++)
            for (int j = 0; j < N; j++)
                dst[(dst_row + j) * dst_stride + dst_col + i] =
                    src[(src_row + i) * src_stride + src_col + j];
    }
    else if (M >= N)
    {
        // Split along rows in src, adjust dst_col accordingly
        int M2 = M / 2;
        transpose_recursive(src, dst, src_row, src_col, dst_row, dst_col, M2, N, src_stride, dst_stride);
        transpose_recursive(src, dst, src_row + M2, src_col, dst_row, dst_col + M2, M - M2, N, src_stride, dst_stride);
    }
    else
    {
        // Split along columns in src, adjust dst_row accordingly
        int N2 = N / 2;
        transpose_recursive(src, dst, src_row, src_col, dst_row, dst_col, M, N2, src_stride, dst_stride);
        transpose_recursive(src, dst, src_row, src_col + N2, dst_row + N2, dst_col, M, N - N2, src_stride, dst_stride);
    }
}

void TransposeRecursive(uint64_t* src, uint64_t* dst, int M, int N)
{
    transpose_recursive(src, dst, 0, 0, 0, 0, M, N, N, M);
}

// Recursive cache-oblivious in-place transpose for square matrices
void transpose_recursive_in_place(uint64_t* matrix, int row, int col, int size, int stride)
{
    if (size <= 16)
    {
        // Base case: transpose small block directly
        for (int i = 0; i < size; ++i)
        {
            for (int j = i + 1; j < size; ++j)
            {
                std::swap(matrix[(row + i) * stride + col + j], matrix[(row + j) * stride + col + i]);
            }
        }
    }
    else
    {
        int size2 = size / 2;
        transpose_recursive_in_place(matrix, row, col, size2, stride);
        transpose_recursive_in_place(matrix, row + size2, col + size2, size - size2, stride);

        // Transpose the off-diagonal blocks
        for (int i = 0; i < size2; ++i)
        {
            for (int j = 0; j < size - size2; ++j)
            {
                std::swap(matrix[(row + i) * stride + col + size2 + j], matrix[(row + size2 + j) * stride + col + i]);
            }
        }
    }
}

void TransposeRecursiveInPlace(uint64_t* matrix, int N)
{
    transpose_recursive_in_place(matrix, 0, 0, N, N);
}

// Verify correctness by comparing two matrices
bool MatricesAreEqual(uint64_t* A, uint64_t* B, int rows, int cols)
{
    // A and B are matrices of size rows x cols
    for (int i = 0; i < rows * cols; ++i)
    {
        if (A[i] != B[i])
        {
            return false;
        }
    }
    return true;
}
