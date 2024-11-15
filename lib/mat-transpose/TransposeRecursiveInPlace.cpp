#include <cstdint>
#include <algorithm>

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
