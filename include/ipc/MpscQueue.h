#pragma once

#include "MpscQueueRingBuffer.h"
#include "utilities/Endpoint.h"

/**
 * @brief Represents a multi-producer, single-consumer (Mpsc) queue for inter-process and inter-thread communication.
 *
 * This class provides a thread-safe and process-safe implementation of an Mpsc queue
 * using shared memory for communication between producers and a single
 * consumer. It facilitates the enqueuing and dequeuing of client requests.
 */
class MpscQueue
{
public:
    /**
     * @brief Constructs an MpscQueue instance.
     *
     * Initializes the Mpsc queue with the specified endpoint.
     *
     * @param endpoint The endpoint used to owner the shared memory object.
     * 
     * @note The size parameter is ignored if the endpoint is CLIENT.
     */
    MpscQueue(Endpoint endpoint);

    /**
     * @brief Destructs the MpscQueue instance.
     *
     * Cleans up resources and detaches shared memory.
     */
    ~MpscQueue();

    /**
     * @brief Enqueues a client request.
     *
     * Adds the provided client request to the queue for processing.
     *
     * @param request The client request to be enqueued.
     */
    void Enqueue(const ClientRequest& request);

    /**
     * @brief Dequeues a client request.
     *
     * Removes a client request from the queue and stores it in the
     * provided reference.
     *
     * @param request A reference to a ClientRequest where the
     *                dequeued request will be stored.
     * @return true if the request was successfully dequeued,
     *         false if the queue was empty.
     */
    bool Dequeue(ClientRequest& request);

private:
    /**
     * @brief Creates shared memory for the queue.
     *
     * Initializes shared memory resources for inter-process communication.
     * 
     * @param endpoint The endpoint used to owner the shared memory object.
     */
    void CreateSharedMemory(Endpoint endpoint);

    /**
     * @brief Detaches shared memory resources.
     *
     * Cleans up and detaches any allocated shared memory.
     */
    void DetachSharedMemory();

    /**
     * @brief Initializes the queue.
     *
     * Prepares the queue for use, setting up necessary data structures.
     */
    void InitializeQueue();

    int m_FileDescriptor;          ///< The file descriptor for the shared memory object.
    std::string m_ShmObjectName;   ///< The name of the shared memory object.
    MpscQueueRingBuffer* m_Queue;  ///< Pointer to the Mpsc queue implementation.
    Endpoint m_Endpoint;           ///< The role of the endpoint in the network connection.
};
