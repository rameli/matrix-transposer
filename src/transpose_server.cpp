#include <iostream>
#include <string>

#include "ipc/ClientRequest.h"
#include "ipc/MpscQueue.h"

int main()
{
    ClientRequest request;
    MpscQueue queue(Endpoint::SERVER);

    int count = 0;

    while (1) {
        if (queue.Dequeue(request))
        {
            count++;
            std::cout << "Count: " << count << " Client ID: " << request.clientId << " Matrix Index: " << request.matrixIndex << std::endl;
        }
    }

    return 0;
}
