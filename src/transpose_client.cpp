#include <iostream>
#include <string>

#include "MPSCQueueClass.h"

int main()
{
    Item item;
    MPSCQueueClass queue;

    int count = 0;

    queue.Enqueue({101});
    queue.Enqueue({102});
    // queue.Enqueue({103});

    return 0;
}
