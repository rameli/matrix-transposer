#include <iostream>
#include <string>

#include "MPSCQueueClass.h"

int main()
{
    Item item;
    MPSCQueueClass queue;

    int count = 0;

    while (count < 10) {
        if (queue.Dequeue(item))
        {
            count++;
            std::cout << "Count: " << count << " Value: " << item.value << std::endl;
        }
    }

    return 0;
}
