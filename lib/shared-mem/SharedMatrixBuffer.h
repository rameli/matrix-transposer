#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <sstream>

/**
 * @class SharedMatrixBuffer
 * @brief A class that manages a shared memory buffer for a 2^m by 2^n matrix of uint64_t elements.
 * 
 * This class provides a way to allocate and manage shared memory for a matrix, allowing multiple processes
 * to access the same data. The memory is allocated in a way that ensures proper cleanup when the object is
 * destroyed, adhering to the RAII principle.
 */
class SharedMatrixBuffer
{
public:
    enum class Endpoint
    {
        SERVER,
        CLIENT
    };

    /**
     * @brief Constructs a SharedMatrixBuffer instance.
     * 
     * @param uniqueId The unique identifier for the shared memory object.
     * @param m The exponent for the number of rows, resulting in 2^m rows.
     * @param n The exponent for the number of columns, resulting in 2^n columns.
     * @param k An arbitrary index used in the shared memory object filename in /dev/shm/.
     * @param endpoint The role of the endpoint in the network connection.
     * 
     * This constructor creates a shared memory segment with the name formatted as
     * "transpose_client_shm_uid_k", where PID is the process ID and k is the provided index.
     * 
     * @throw std::runtime_error If shared memory creation or mapping fails.
     */
    SharedMatrixBuffer(uint32_t uniqueId, uint32_t m, uint32_t n, uint32_t k, Endpoint endpoint, std::string suffix);
    
    /**
     * @brief Destroys the SharedMatrixBuffer instance and releases resources.
     * 
     * The destructor unmaps the shared memory and unlinks it from the system.
     */
    ~SharedMatrixBuffer();

    /**
     * @brief Gets a pointer to the shared memory buffer.
     * 
     * @return A pointer to the allocated shared memory, which can be cast to uint64_t* for matrix access.
     */
    uint64_t* GetRawPointer() const;

    /**
     * @brief Gets the number of rows in the matrix.
     * 
     * @return The number of rows, calculated as 2^m.
     */
    uint32_t RowCount() const;

    /**
     * @brief Gets the number of columns in the matrix.
     * 
     * @return The number of columns, calculated as 2^n.
     */
    uint32_t ColumnCount() const;

    /**
     * @brief Gets the name of the shared memory object.
     * 
     * @return The name of the shared memory object.
     */
    std::string GetName() const;

    /**
     * @brief Gets the number of elements in the matrix.
     * 
     * @return The number of elements in the matrix, calculated as 2^(m+n).
     */
    uint32_t GetElementCount() const;

    /**
     * @brief Gets the size of the shared memory buffer in bytes.
     * 
     * @return The size of the shared memory buffer in bytes.
     */
    size_t GetBufferSizeInBytes() const;

    /**
     * @brief Creates a unique name for the shared memory object.
     * 
     * @param uniqueId The unique identifier for the shared memory object.
     * @param k An arbitrary index used in the shared memory object filename.
     * @return A string containing the formatted name of the shared memory object.
     */
    static std::string CreateShmObjectName(uint32_t uniqueId, uint32_t k, std::string suffix);

private:
    uint32_t m_UniqueId;         ///< Unique identifier for the shared memory object.
    std::string m_ShmObjectName; ///< Name of the shared memory object.
    size_t m_BufferBytes;        ///< Size of the shared memory in bytes.
    int m_FileDescriptor;        ///< File descriptor for the shared memory object.
    void* m_RawPointer;          ///< Pointer to the mapped shared memory.
    uint32_t m_NumRows;          ///< Number of rows in the matrix.
    uint32_t m_NumColumns;       ///< Number of columns in the matrix.
    uint32_t m_BufferIndex;      ///< Index used in the shared memory object filename.
    Endpoint m_Endpoint;         ///< The role of the endpoint in the network connection.
    std::string m_Suffix;        ///< A suffix to append to the shared memory object name.
};
