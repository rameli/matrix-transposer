#include <cstdint>

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
