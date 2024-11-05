#include "MPSCQueueClass.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cerrno>

constexpr const char* SHM_NAME = "/mpsc_queue_shm";

MPSCQueueClass::MPSCQueueClass()
{
    CreateSharedMemory();
    InitializeQueue();
}

MPSCQueueClass::~MPSCQueueClass()
{
    DetachSharedMemory();
}

void MPSCQueueClass::CreateSharedMemory()
{
    m_FileDescriptor = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);

    if (m_FileDescriptor < 0)
    {
        throw std::runtime_error("shm_open error");
    } 

    if (ftruncate(m_FileDescriptor, sizeof(MPSCQueue)) == -1)
    {
        shm_unlink(SHM_NAME);
        throw std::runtime_error("ftruncate error");
    }

    m_Queue = (MPSCQueue*)mmap(nullptr, sizeof(MPSCQueue), PROT_READ | PROT_WRITE, MAP_SHARED, m_FileDescriptor, 0);
    if (m_Queue == MAP_FAILED) {
        shm_unlink(SHM_NAME);
        close(m_FileDescriptor);
        throw std::runtime_error("mmap error");
    }
    close(m_FileDescriptor);
}

void MPSCQueueClass::DetachSharedMemory() {
    munmap(m_Queue, sizeof(MPSCQueue));
    // shm_unlink(SHM_NAME);
}

void MPSCQueueClass::InitializeQueue() {
    m_Queue->enqueue_pos.store(0, std::memory_order_relaxed);
    m_Queue->dequeue_pos = 0;
    m_Queue->capacity = BUFFER_SIZE;
    m_Queue->mask = BUFFER_SIZE - 1;

    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        m_Queue->buffer[i].sequence.store(i, std::memory_order_relaxed);
    }
}

void MPSCQueueClass::Enqueue(const Item& item)
{
    size_t pos = m_Queue->enqueue_pos.fetch_add(1, std::memory_order_relaxed);
    Node* node = &m_Queue->buffer[pos & m_Queue->mask];

    size_t seq = node->sequence.load(std::memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)pos;

    while (diff != 0) {
        seq = node->sequence.load(std::memory_order_acquire);
        diff = (intptr_t)seq - (intptr_t)pos;
    }

    node->data = item;
    node->sequence.store(pos + 1, std::memory_order_release);
}

bool MPSCQueueClass::Dequeue(Item& item) {
    Node* node = &m_Queue->buffer[m_Queue->dequeue_pos & m_Queue->mask];
    size_t seq = node->sequence.load(std::memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)(m_Queue->dequeue_pos + 1);

    if (diff == 0) {
        item = node->data;
        node->sequence.store(m_Queue->dequeue_pos + m_Queue->capacity, std::memory_order_release);
        m_Queue->dequeue_pos++;
        return true;
    } else {
        return false;
    }
}