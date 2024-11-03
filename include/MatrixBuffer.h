#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <string>
#include <sstream>

/**
 * @class MatrixBuffer
 * @brief A class that manages a shared memory buffer for a 2^m by 2^n matrix of uint64_t elements.
 * 
 * This class provides a way to allocate and manage shared memory for a matrix, allowing multiple processes
 * to access the same data. The memory is allocated in a way that ensures proper cleanup when the object is
 * destroyed, adhering to the RAII principle.
 */
class MatrixBuffer
{
public:
    /**
     * @brief Constructs a MatrixBuffer instance.
     * 
     * @param uniqueId The unique identifier for the shared memory object.
     * @param m The exponent for the number of rows, resulting in 2^m rows.
     * @param n The exponent for the number of columns, resulting in 2^n columns.
     * @param k An arbitrary index used in the shared memory object filename in /dev/shm/.
     * 
     * This constructor creates a shared memory segment with the name formatted as
     * "transpose_client_shm_uid_k", where PID is the process ID and k is the provided index.
     * 
     * @throw std::runtime_error If shared memory creation or mapping fails.
     */
    MatrixBuffer(uint32_t uniqueId, size_t m, size_t n, size_t k);
    
    /**
     * @brief Destroys the MatrixBuffer instance and releases resources.
     * 
     * The destructor unmaps the shared memory and unlinks it from the system.
     */
    ~MatrixBuffer();

    /**
     * @brief Gets a pointer to the shared memory buffer.
     * 
     * @return A pointer to the allocated shared memory, which can be cast to uint64_t* for matrix access.
     */
    void* GetRawPointer() const;

    /**
     * @brief Gets the number of rows in the matrix.
     * 
     * @return The number of rows, calculated as 2^m.
     */
    size_t RowCount() const;

    /**
     * @brief Gets the number of columns in the matrix.
     * 
     * @return The number of columns, calculated as 2^n.
     */
    size_t ColumnCount() const;

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
    size_t GetElementCount() const;

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
    static std::string CreateShmObjectName(uint32_t uniqueId, size_t k);

private:
    uint32_t m_UniqueId;         ///< Unique identifier for the shared memory object.
    std::string m_ShmObjectName; ///< Name of the shared memory object.
    size_t m_BufferBytes;        ///< Size of the shared memory in bytes.
    int m_FileDescriptor;        ///< File descriptor for the shared memory object.
    void* m_RawPointer;          ///< Pointer to the mapped shared memory.
    size_t m_NumRows;            ///< Number of rows in the matrix.
    size_t m_NumColumns;         ///< Number of columns in the matrix.
    size_t m_BufferIndex;        ///< Index used in the shared memory object filename.

    /**
     * @brief Unlinks the shared memory object.
     * 
     * This function removes the shared memory object from the system.
     */
    void unlink(); // Unlink shared memory
};
