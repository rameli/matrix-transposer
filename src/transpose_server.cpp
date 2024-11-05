#include <iostream>
#include <string>

#include "ipc/ClientRequest.h"
#include "ipc/MpscQueue.h"

int main()
{
    ClientRequest request;
    MpscQueue queue(Endpoint::SERVER);

    int count = 0;

    while (count < 10000) {
        if (queue.Dequeue(request))
        {
            count++;
            std::cout << "Count: " << count << " Value: " << request.matrixIndex << std::endl;
        }
    }

    return 0;
}
