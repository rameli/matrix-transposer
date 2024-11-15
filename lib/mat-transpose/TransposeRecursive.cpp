#include <cstdint>

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
