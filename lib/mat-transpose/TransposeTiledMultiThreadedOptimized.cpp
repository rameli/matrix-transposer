#include <cstdint>
#include <thread>
#include <vector>

struct Block
{
    uint32_t i_start, i_end;
    uint32_t j_start, j_end;
};

std::vector<Block> gBlocks;
uint32_t gNumThreads;
uint64_t* gSrc;
uint64_t* gDst;
static uint32_t gRowCount;
static uint32_t gColCount;
std::vector<std::thread> gThreads;

static void workerThread(uint32_t t)
{
    for (size_t idx = t; idx < gBlocks.size(); idx += gNumThreads)
    {
        Block& block = gBlocks[idx];
        for (uint32_t i = block.i_start; i < block.i_end; i++)
        {
            for (uint32_t j = block.j_start; j < block.j_end; j++)
            {
                gDst[j * gRowCount + i] = gSrc[i * gColCount + j];
            }
        }
    }
}

static void setup(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads)
{
    uint32_t numBlocksInRow = (rowCount + tileSize - 1) / tileSize;
    uint32_t numBlocksInCol = (colCount + tileSize - 1) / tileSize;

    gNumThreads = numThreads;
    gSrc = src;
    gDst = dst;
    gRowCount = rowCount;
    gColCount = colCount;

    gBlocks.clear();


    gThreads = std::vector<std::thread>(gNumThreads);

    for (uint32_t bi = 0; bi < numBlocksInRow; bi++)
    {
        for (uint32_t bj = 0; bj < numBlocksInCol; bj++)
        {
            uint32_t i_start = bi * tileSize;
            uint32_t j_start = bj * tileSize;
            uint32_t i_end = std::min(i_start + tileSize, rowCount);
            uint32_t j_end = std::min(j_start + tileSize, colCount);
            gBlocks.push_back({i_start, i_end, j_start, j_end});
        }
    }

}

static void teardown()
{
    
}

void TransposeTiledMultiThreadedOptimized(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads)
{
    setup(src, dst, rowCount, colCount, tileSize, numThreads);

    for (uint32_t t = 0; t < numThreads; t++)
    {
        gThreads[t] = std::thread(workerThread, t);
    }

    for (auto& th : gThreads)
    {
        th.join();
    }
}