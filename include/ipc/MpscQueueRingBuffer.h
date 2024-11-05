#pragma once

#include <atomic>
#include <cstddef>

#include "ClientRequest.h"

constexpr size_t BUFFER_SIZE = 1024;
constexpr size_t CACHE_LINE_SIZE = 64;

struct alignas(CACHE_LINE_SIZE) Node
{
    std::atomic<size_t> sequence;
    ClientRequest data;
    char padding[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>) - sizeof(ClientRequest)];
};

struct alignas(CACHE_LINE_SIZE) MpscQueueRingBuffer
{
    std::atomic<size_t> enqueue_pos;
    char pad1[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];
    size_t dequeue_pos;
    char pad2[CACHE_LINE_SIZE - sizeof(size_t)];
    size_t capacity;
    size_t mask;
    Node buffer[BUFFER_SIZE];
};
