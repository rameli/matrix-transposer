#include <cstdint>
#include <algorithm>

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