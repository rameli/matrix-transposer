#include <cstdint>
#include <thread>
#include <vector>

struct Block
{
    uint32_t i_start, i_end;
    uint32_t j_start, j_end;
};

void TransposeTiledInPlaceMultiThreaded(uint64_t* matrix, uint32_t rowCount, uint32_t tileSize, uint32_t numThreads)
{
    uint32_t numBlocks = (rowCount + tileSize - 1) / tileSize;
    std::vector<std::vector<Block>> threadBlocks(numThreads);

    // Assign blocks to threads ensuring no overlaps
    uint32_t blockCount = 0;
    for (uint32_t bi = 0; bi < numBlocks; bi++)
    {
        for (uint32_t bj = bi; bj < numBlocks; bj++)
        { 
            // Include diagonal blocks
            uint32_t t = blockCount % numThreads;
            uint32_t i_start = bi * tileSize;
            uint32_t j_start = bj * tileSize;
            uint32_t i_end = std::min(i_start + tileSize, rowCount);
            uint32_t j_end = std::min(j_start + tileSize, rowCount);

            threadBlocks[t].push_back({i_start, i_end, j_start, j_end});

            blockCount++;
        }
    }

    if (numThreads <= 0)
    {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0)
        {
            numThreads = 4;
        }
    }

    std::vector<std::thread> threads(numThreads);

    // Assign blocks to threads
    for (uint32_t t = 0; t < numThreads; t++)
    {
        threads[t] = std::thread([&, t](){
            for (const auto& block : threadBlocks[t])
            {
                uint32_t i_start = block.i_start;
                uint32_t i_end = block.i_end;
                uint32_t j_start = block.j_start;
                uint32_t j_end = block.j_end;

                if (i_start == j_start)
                {
                    // Diagonal block
                    for (uint32_t i = i_start; i < i_end; i++)
                    {
                        for (uint32_t j = i + 1; j < j_end; j++)
                        {
                            std::swap(matrix[i * rowCount + j], matrix[j * rowCount + i]);
                        }
                    }
                }
                else
                {
                    // Off-diagonal blocks
                    for (uint32_t i = i_start; i < i_end; i++)
                    {
                        for (uint32_t j = j_start; j < j_end; j++)
                        {
                            std::swap(matrix[i * rowCount + j], matrix[j * rowCount + i]);
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