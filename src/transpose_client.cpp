#include <iostream>
#include <string>

#include "ipc/ClientRequest.h"
#include "ipc/MpscQueue.h"

int main(int argc, char* argv[])
{
    ClientRequest request;

    int count = 0;

    request.clientId = std::stoi(argv[1]);
    MpscQueue queue(request.clientId, Endpoint::CLIENT);


    while (1) {
        request.matrixIndex = count;
        queue.Enqueue(request);
        count++;
        std::cout << "Count: " << count << std::endl;
    }

    return 0;
}
