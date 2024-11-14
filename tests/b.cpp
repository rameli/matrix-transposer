#include <memory>
#include <iostream>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <cstdint>

#include "spsc-queue/SpscQueue.h"

int main(int argc, char* argv[])
{
    constexpr size_t CAPACITY = 1000;

    SpscQueue queue(11889944, SpscQueue::Role::Consumer, CAPACITY, "");

    uint32_t item;
    for (int i = 0; i < CAPACITY; i++)
    {
        while (!queue.Dequeue(item))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        std::cout << "Received: " << item << std::endl;
    }


    return 0;
}
