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
     * @param m The exponent for the number of rows, resulting in 2^m rows.
     * @param n The exponent for the number of columns, resulting in 2^n columns.
     * @param k An arbitrary index used in the shared memory name.
     * 
     * This constructor creates a shared memory segment with the name formatted as
     * "transpose_client_shm_PID_k", where PID is the process ID and k is the provided index.
     * 
     * @throw std::runtime_error If shared memory creation or mapping fails.
     */
    MatrixBuffer(size_t m, size_t n, size_t k);
    
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

private:
    std::string shm_name; ///< Name of the shared memory object.
    size_t shm_size;      ///< Size of the shared memory in bytes.
    int shm_fd;          ///< File descriptor for the shared memory object.
    void* ptr;           ///< Pointer to the mapped shared memory.
    size_t num_rows;     ///< Number of rows in the matrix.
    size_t num_cols;     ///< Number of columns in the matrix.

    /**
     * @brief Unlinks the shared memory object.
     * 
     * This function removes the shared memory object from the system.
     */
    void unlink(); // Unlink shared memory
};
