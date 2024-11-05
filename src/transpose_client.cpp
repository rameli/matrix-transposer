#include <iostream>
#include <string>

#include "ipc/ClientRequest.h"
#include "ipc/MpscQueue.h"

int main()
{
    MpscQueueClass queue;

    int count = 0;

    queue.Enqueue({101});
    queue.Enqueue({102});
    // queue.Enqueue({103});

    return 0;
}
