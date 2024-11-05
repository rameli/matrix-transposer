#ifndef MPSC_QUEUE_H
#define MPSC_QUEUE_H

#include <atomic>
#include <cstddef>
#include <set>

constexpr size_t BUFFER_SIZE = 1024;  // Must be a power of two

struct Item {
    int value;  // Data type stored in the queue
};

class MPSCQueue {
public:
    MPSCQueue();
    ~MPSCQueue();

    bool enqueue(const Item& item);
    bool dequeue(Item& item);
    void initialize();

    static int createSharedMemory();
    static MPSCQueue* attachSharedMemory(int fd);
    static void detachSharedMemory(MPSCQueue* queue, int fd);

private:
    struct alignas(64) Node {
        std::atomic<size_t> sequence;
        Item data;
        char padding[64 - sizeof(std::atomic<size_t>) - sizeof(Item)];
    };

    std::atomic<size_t> enqueue_pos; // Producer index
    size_t dequeue_pos;              // Consumer index (only consumer modifies)
    size_t capacity;
    size_t mask;
    Node buffer[BUFFER_SIZE];
};

#endif // MPSC_QUEUE_H
