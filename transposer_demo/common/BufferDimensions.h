#pragma once

#include <cstdint>

struct BufferDimensions
{
    uint32_t m;
    uint32_t n;
    uint32_t k;

    uint32_t numRows;
    uint32_t numColumns;
};