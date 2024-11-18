#include <cstdint>
#include <thread>
#include <vector>

struct Block
{
    uint32_t iStart, iEnd;
    uint32_t jStart, jEnd;
};

void TransposeTiledMultiThreaded(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads)
{
    uint32_t numBlocksInRow = (rowCount + tileSize - 1) / tileSize;
    uint32_t numBlocksInCol = (colCount + tileSize - 1) / tileSize;
    
    std::vector<std::thread> threads(numThreads);
    std::vector<Block> blocks(numBlocksInRow * numBlocksInCol);

    // Works only for matrices with row/column count of power of 2
    for (uint32_t bj = 0; bj < numBlocksInCol; bj++)
    {
        for (uint32_t bi = 0; bi < numBlocksInRow; bi++)
        {
            uint32_t iStart = bi * tileSize;
            uint32_t jStart = bj * tileSize;
            uint32_t iEnd = iStart + tileSize, rowCount;
            uint32_t jEnd = jStart + tileSize, colCount;
            blocks.push_back({iStart, iEnd, jStart, jEnd});
        }
    }

    for (uint32_t t = 0; t < numThreads; t++)
    {
        threads[t] = std::thread([&, t]()
        {
            for (size_t idx = t; idx < blocks.size(); idx += numThreads)
            {
                Block& block = blocks[idx];
                for (uint32_t i = block.iStart; i < block.iEnd; i++)
                {
                    for (uint32_t j = block.jStart; j < block.jEnd; j++)
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