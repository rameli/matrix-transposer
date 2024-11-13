/**
 * @file Futex.h
 * @brief Definition of the Futex class for managing futex-based synchronization using shared memory among multiple processes.
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @class Futex
 * @brief A class that provides basic futex-based synchronization using shared memory.
 * 
 * This class enables inter-process synchronization by using shared memory objects
 * and futex (fast user-space mutex) mechanisms.
 */
class Futex {
public:
    enum class Endpoint
    {
        SERVER,
        CLIENT
    };

    /**
     * @brief Constructs a Futex object with a unique identifier. The server uses this identifier to access the same shared memory futex integer.
     * 
     * @param uniqueId A unique identifier used to create the shared memory object.
     * @param endpoint The role of the endpoint in the network connection.
     */
    Futex(uint32_t uniqueId, Endpoint endpoint);

    /**
     * @brief Destructor for the Futex class.
     * 
     * Cleans up resources associated with the shared memory and futex operations.
     */
    ~Futex();

    /**
     * @brief Blocks the calling thread until the futex is woken.
     * 
     * This function causes the calling thread to wait until another thread or process calls wake().
     */
    void Wait();

    bool IsWaiting();

    /**
     * @brief Wakes a thread waiting on this futex.
     * 
     * This function unblocks a single thread waiting on the futex, allowing it to proceed.
     */
    void Wake();

private:
    /**
     * @brief Generates a unique shared memory object name based on the given ID. Each client process has a single futex used by the server to indicate that the previouos request has been processed.
     * 
     * @param uniqueId The unique identifier used to generate the shared memory name.
     * @return A string representing the shared memory object name.
     */
    std::string CreateShmObjectName(uint32_t uniqueId);

    /**
     * @brief Initializes the shared memory and sets up the futex structure.
     * 
     * This function allocates and maps the shared memory needed for futex operations.
     */
    void InitializeSharedMemory();

    uint32_t m_UniqueId;                 ///< Unique identifier for the futex instance.
    std::string m_ShmObjectName;         ///< Name of the shared memory object in /dev/shm.
    int m_FileDescriptor;                ///< File descriptor for the shared memory object.
    std::atomic<uint32_t> *m_RawPointer; ///< Pointer to the futex counter in shared memory.
    Endpoint m_Endpoint;                 ///< The role of the endpoint in the network connection.
};
