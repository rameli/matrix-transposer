#pragma once

#include <atomic>
#include <cstddef>
#include <set>

constexpr size_t BUFFER_SIZE = 1024;
constexpr size_t CACHE_LINE_SIZE = 64;

struct Item
{
    int value;
};

struct alignas(CACHE_LINE_SIZE) Node
{
    std::atomic<size_t> sequence;
    Item data;
    char padding[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>) - sizeof(Item)];
};

struct alignas(CACHE_LINE_SIZE) MPSCQueue
{
    std::atomic<size_t> enqueue_pos;
    char pad1[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];
    size_t dequeue_pos;
    char pad2[CACHE_LINE_SIZE - sizeof(size_t)];
    size_t capacity;
    size_t mask;
    Node buffer[BUFFER_SIZE];
};


class MPSCQueueClass
{
public:
    MPSCQueueClass();
    ~MPSCQueueClass();

    void Enqueue(const Item& item);
    bool Dequeue(Item& item);

private:
    void CreateSharedMemory();
    void DetachSharedMemory();
    void InitializeQueue();

    int m_FileDescriptor;

    MPSCQueue* m_Queue;
};


