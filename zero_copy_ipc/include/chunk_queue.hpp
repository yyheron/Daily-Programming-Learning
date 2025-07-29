#pragma once

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <cstddef>
#include <sys/eventfd.h>
#include <unistd.h>
#include <atomic>

namespace zero_copy_ipc {

using namespace boost::interprocess;

// 环形缓冲区，支持多进程安全
template<typename T, std::size_t N>
struct ChunkQueue {
    T buffer[N];
    alignas(64) std::atomic<uint64_t> tail{0};
    int event_fd = -1; // 新增eventfd
    
    ChunkQueue() {
        event_fd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
        if (event_fd == -1) {
            perror("eventfd create failed");
        }
    }
    ~ChunkQueue() {
        if (event_fd != -1) close(event_fd);
    }

    // 队列大小仅支持2^n - 1；后续看是否有内存调整需求
    static_assert(((N & (N + 1)) == 0),
        "Queue size N must be in the form of 2^n - 1 (e.g., 7, 15, 255, 1023, 2047, 4095, 8191, 16383, 32767, 65535)");
    // push, pop_ptr, borrow_slot 已经被移除，因为 head 是由每个 subscriber 自己管理的
    
    void commit_slot() {
        uint64_t expected = tail.load(std::memory_order_relaxed);
        uint64_t desired = (expected + 1) & N;
        while (!tail.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed)) {
            desired = (expected + 1) & N;
        }
        uint64_t val = 1;
        write(event_fd, &val, sizeof(val)); // 通知所有订阅者
    }

};

} //
