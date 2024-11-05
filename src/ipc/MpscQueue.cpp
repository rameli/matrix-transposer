#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include "ipc/MpscQueue.h"

MpscQueue::MpscQueue(Endpoint endpoint) :
    m_FileDescriptor{-1},
    m_ShmObjectName {"/matrix_transposer_server_queue"},
    m_Endpoint{endpoint}
{
    CreateSharedMemory(m_Endpoint);
    InitializeQueue();
}

MpscQueue::~MpscQueue()
{
    if (m_Queue != MAP_FAILED && m_Queue != nullptr)
    {
        munmap(m_Queue, sizeof(m_Queue));
    }

    // Only the client should unlink the shared memory object
    if (m_Endpoint == Endpoint::SERVER)
    {
        shm_unlink(m_ShmObjectName.c_str());
    }
}

void MpscQueue::CreateSharedMemory(Endpoint endpoint)
{
    // Only the server should initialize the shared memory object for the futex.
    // If the object already exists, unlink it and try again.
    if (m_Endpoint == Endpoint::SERVER)
    {
        // Client opens the mutex shared memory object with the O_EXCL flag
        m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);

        if (m_FileDescriptor < 0)
        {
            if (errno == EEXIST) {
                shm_unlink(m_ShmObjectName.c_str());
                m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
            }
            if (m_FileDescriptor < 0) {
                if (m_FileDescriptor == -1)
                {
                    throw std::runtime_error("Failed to create shared memory object for client process");
                }
            }
        }
    }
    else if (m_Endpoint == Endpoint::CLIENT)
    {
        // Server opens the mutex shared memory object without the O_EXCL flag
        m_FileDescriptor = shm_open(m_ShmObjectName.c_str(), O_CREAT | O_RDWR, 0666);

        if (m_FileDescriptor < 0)
        {
            throw std::runtime_error("Failed to create shared memory object for client process");
        }
    }

    if (ftruncate(m_FileDescriptor, sizeof(MpscQueueRingBuffer)) == -1)
    {
        // Destructor won't be called if an exception is thrown in the constructor
        close(m_FileDescriptor);
        shm_unlink(m_ShmObjectName.c_str());
        throw std::runtime_error("ftruncate error");
    }

    m_Queue = (MpscQueueRingBuffer*)mmap(nullptr, sizeof(MpscQueueRingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);
    if (m_Queue == MAP_FAILED) {
        // Destructor won't be called if an exception is thrown in the constructor
        close(m_FileDescriptor);
        shm_unlink(m_ShmObjectName.c_str());
        throw std::runtime_error("mmap error");
    }

    // According to https://man7.org/linux/man-pages/man3/shm_open.3.html
    // "After a call to mmap(2) the file descriptor may be closed without affecting the memory mapping."
    close(m_FileDescriptor);
}

void MpscQueue::InitializeQueue() {
    m_Queue->enqueuePosition.store(0, std::memory_order_relaxed);
    m_Queue->dequeuePosition = 0;
    m_Queue->capacity = BUFFER_SIZE;
    m_Queue->mask = BUFFER_SIZE - 1;

    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        m_Queue->buffer[i].sequence.store(i, std::memory_order_relaxed);
    }
}

void MpscQueue::Enqueue(const ClientRequest& request)
{
    size_t pos = m_Queue->enqueuePosition.fetch_add(1, std::memory_order_relaxed);
    Node* node = &m_Queue->buffer[pos & m_Queue->mask];

    size_t seq = node->sequence.load(std::memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)pos;

    while (diff != 0) {
        seq = node->sequence.load(std::memory_order_acquire);
        diff = (intptr_t)seq - (intptr_t)pos;
        usleep(1);
    }

    node->data = request;
    node->sequence.store(pos + 1, std::memory_order_release);
}

bool MpscQueue::Dequeue(ClientRequest& request) {
    Node* node = &m_Queue->buffer[m_Queue->dequeuePosition & m_Queue->mask];
    size_t seq = node->sequence.load(std::memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)(m_Queue->dequeuePosition + 1);

    if (diff == 0) {
        request = node->data;
        node->sequence.store(m_Queue->dequeuePosition + m_Queue->capacity, std::memory_order_release);
        m_Queue->dequeuePosition++;
        return true;
    } else {
        return false;
    }
}