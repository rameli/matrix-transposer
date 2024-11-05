#pragma once

#include <atomic>
#include <cstddef>

#include "ClientRequest.h"

// Buffer size for the ring buffer.
// *** Must be a power of 2 ***
constexpr size_t BUFFER_SIZE = 1024;

// Cache line size for alignment.
constexpr size_t CACHE_LINE_SIZE = 64;

/**
 * @brief Represents a node in the multi-producer, single-consumer (MPSC) queue.
 *
 * Each node contains a sequence number and a client request, with padding to ensure proper cache alignment.
 */
struct alignas(CACHE_LINE_SIZE) Node
{
    /**
     * @brief The sequence number of the node.
     *
     * This atomic variable is used to track the sequence of
     * operations on the node, providing a way to manage concurrent access safely.
     */
    std::atomic<size_t> sequence;

    /**
     * @brief The client request data stored in the node.
     *
     * This holds the actual client request that is being processed or transferred in the MPSC queue.
     */
    ClientRequest data;

    /**
     * @brief Padding to maintain cache line alignment.
     *
     * This array is used to ensure that the size of the Node structure
     * aligns with the cache line size, reducing false sharing
     * and improving performance.
     */
    char padding[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>) - sizeof(ClientRequest)];
};

/**
 * @brief Represents a multi-producer, single-consumer (MPSC) queue ring buffer.
 *
 * This structure implements a ring buffer to allow multiple producers to
 * enqueue client requests while a single consumer dequeues them. It uses
 * atomic variables to manage access and ensure thread safety.
 */
struct alignas(CACHE_LINE_SIZE) MpscQueueRingBuffer
{
    /**
     * @brief The position for the next enqueue operation.
     *
     * This atomic variable keeps track of where the next item will be enqueued in the buffer.
     */
    std::atomic<size_t> enqueuePosition;

    /**
     * @brief Padding to maintain cache line alignment.
     *
     * This padding is added to ensure that enqueuePosition aligns with
     * the cache line size, preventing false sharing.
     */
    char pad1[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

    /**
     * @brief The position for the next dequeue operation.
     *
     * This variable indicates where the next item will be dequeued
     * from the buffer.
     */
    size_t dequeuePosition;

    /**
     * @brief Padding to maintain cache line alignment.
     *
     * Similar to pad1, this padding ensures that dequeuePosition aligns
     * correctly in memory.
     */
    char pad2[CACHE_LINE_SIZE - sizeof(size_t)];

    /**
     * @brief The total capacity of the ring buffer.
     *
     * This indicates the maximum number of items that can be stored in the buffer at any given time.
     */
    size_t capacity;

    /**
     * @brief The mask for indexing into the buffer.
     *
     * This mask is used to calculate the appropriate index in the buffer based on the enqueue and dequeue positions.
     */
    size_t mask;

    /**
     * @brief The array of nodes forming the ring buffer.
     *
     * This is the actual storage for client requests, allowing for efficient enqueue and dequeue operations.
     */
    Node buffer[BUFFER_SIZE];
};
