#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "ipc/ClientRequest.h"
#include "ipc/MpscQueue.h"

int main()
{
    ClientRequest request;
    std::vector<std::unique_ptr<MpscQueue>> queues;
    std::vector<uint32_t> counts;

    constexpr size_t NUM_CLIENTS = 4;

    try
    {
        for (size_t i = 0; i < NUM_CLIENTS; ++i)
        {
            queues.push_back(std::make_unique<MpscQueue>(i, Endpoint::SERVER));
            counts.push_back(0);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    

    while (1)
    {
        for (size_t i = 0; i < NUM_CLIENTS; ++i)
        {
            if (queues[i]->Dequeue(request))
            {
                counts[i]++;
                std::cout << counts[i] << " Client ID: " << request.clientId << " Matrix Index: " << request.matrixIndex << std::endl;
            }
        }
        
    }

    return 0;
}
