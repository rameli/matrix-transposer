#include "MPSCQueue.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

constexpr const char* SHM_NAME = "/mpsc_queue_shms";

MPSCQueue::MPSCQueue()
    : enqueue_pos(0), dequeue_pos(0), capacity(BUFFER_SIZE), mask(BUFFER_SIZE - 1) {
    initialize();
}

MPSCQueue::~MPSCQueue() {}

void MPSCQueue::initialize() {
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        buffer[i].sequence.store(i, std::memory_order_relaxed);
    }
}

bool MPSCQueue::enqueue(const Item& item) {
    size_t pos = enqueue_pos.fetch_add(1, std::memory_order_relaxed);
    Node* node = &buffer[pos & mask];

    size_t seq = node->sequence.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

    while (diff != 0) {
        seq = node->sequence.load(std::memory_order_acquire);
        diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
        usleep(10);  // Backoff to reduce contention
    }

    node->data = item;
    node->sequence.store(pos + 1, std::memory_order_release);
    return true;
}

bool MPSCQueue::dequeue(Item& item) {
    Node* node = &buffer[dequeue_pos & mask];
    size_t seq = node->sequence.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(dequeue_pos + 1);

    if (diff == 0) {
        item = node->data;
        node->sequence.store(dequeue_pos + capacity, std::memory_order_release);
        dequeue_pos++;
        return true;
    } else {
        return false;
    }
}

int MPSCQueue::createSharedMemory() {
    int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd >= 0) {
        if (ftruncate(fd, sizeof(MPSCQueue)) == -1) {
            perror("ftruncate");
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
    } else if (errno == EEXIST) {
        shm_unlink(SHM_NAME);
        fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
        if (fd < 0) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
        if (ftruncate(fd, sizeof(MPSCQueue)) == -1) {
            perror("ftruncate");
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
    } else {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    return fd;
}

MPSCQueue* MPSCQueue::attachSharedMemory(int fd) {
    MPSCQueue* queue = (MPSCQueue*)mmap(nullptr, sizeof(MPSCQueue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (queue == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    return queue;
}

void MPSCQueue::detachSharedMemory(MPSCQueue* queue, int fd) {
    munmap(queue, sizeof(MPSCQueue));
    close(fd);
    shm_unlink(SHM_NAME);
}
