#include <cstdint>
#include <thread>
#include <vector>

struct Block
{
    uint32_t i_start, i_end;
    uint32_t j_start, j_end;
};

static void workerThread()
{
    
}

static void setup()
{

}

static void teardown()
{
    
}

void TransposeTiledMultiThreadedOptimized(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads)
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