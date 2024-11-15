#include <cstdint>

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