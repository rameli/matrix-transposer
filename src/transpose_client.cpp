#include <iostream>
#include <string>

#include "ipc/ClientRequest.h"
#include "ipc/MpscQueue.h"

int main()
{
    MpscQueue queue(Endpoint::CLIENT);
    ClientRequest request;

    int count = 0;

    while (count < 10000) {
        request.clientId = count;
        request.matrixIndex = count+3;
        queue.Enqueue(request);
        count++;
        std::cout << "Count: " << count << std::endl;
    }

    return 0;
}
