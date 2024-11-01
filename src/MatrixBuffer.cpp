#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>

#include "MatrixBuffer.h"

MatrixBuffer::MatrixBuffer(size_t m, size_t n, size_t k) :
    shm_fd(-1),
    ptr(nullptr),
    num_rows(1UL << m),
    num_cols(1UL << n)
{
    // Generate the shared memory name in the specified format
    std::ostringstream oss;
    oss << "transpose_client_pid" << getpid() << "_k" << k;
    shm_name = oss.str();

    // Calculate size: 2^m * 2^n * sizeof(uint64_t)
    shm_size = num_rows * num_cols * sizeof(uint64_t);

    // Create shared memory object
    shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        throw std::runtime_error("Failed to create shared memory object");
    }

    // Set the size of the shared memory object
    if (ftruncate(shm_fd, shm_size) == -1)
    {
        unlink(); // Clean up before throwing
        throw std::runtime_error("Failed to set shared memory size");
    }

    // Map the shared memory object
    ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        unlink(); // Clean up before throwing
        throw std::runtime_error("Failed to map shared memory");
    }
}

MatrixBuffer::~MatrixBuffer()
{
    if (ptr != MAP_FAILED && ptr != nullptr)
    {
        munmap(ptr, shm_size); // Unmap shared memory
    }
    unlink(); // Unlink the shared memory object
}

void* MatrixBuffer::GetRawPointer() const
{
    return ptr; // Return the pointer to the shared memory
}

size_t MatrixBuffer::RowCount() const
{
    return num_rows; // Return number of rows
}

size_t MatrixBuffer::ColumnCount() const
{
    return num_cols; // Return number of columns
}

std::string MatrixBuffer::GetName() const
{
    return shm_name; // Return the shared memory name
}

void MatrixBuffer::unlink()
{
    if (shm_fd != -1)
    {
        shm_unlink(shm_name.c_str()); // Remove the shared memory object
        shm_name.clear();
    }
}
