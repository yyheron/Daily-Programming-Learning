#pragma once

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <cstddef>
#include <sys/eventfd.h>
#include <unistd.h>
#include <atomic>
#include <type_traits>
#include "ipc_utils.hpp"
#include "pubsub_types.hpp"
#include "loghelper.hpp"

namespace zero_copy_ipc {

using namespace boost::interprocess;

// 环形缓冲区，支持多进程安全
template<typename T, std::size_t N>
struct ChunkQueue {
private:
    using allocator_type = ShmStlAllocator<T>; // 添加分配器类型定义
    template <typename U = T>
    typename std::enable_if<needs_stl_allocator<U>::value>::type
    construct_element(void* slot, const allocator_type& alloc) {
        new (slot) T(alloc);
    }

    template <typename U = T>
    typename std::enable_if<!needs_stl_allocator<U>::value>::type
    construct_element(void* slot, const allocator_type& /*alloc*/) {
        new (slot) T{}; // 值初始化，比()更安全
    }

public:
    T buffer[N];
    alignas(64) std::atomic<uint64_t> tail{0};
    
    // 合并构造函数，默认参数为nullptr
    explicit ChunkQueue(const ShmStlAllocator<T>* alloc = nullptr) {
        LOGINFOLINE("[ChunkQueue] Constructor called, allocator: %d", (alloc != nullptr));
        for (auto& slot : buffer) {
            construct_element(&slot, *alloc);
        }
    }

    ~ChunkQueue() {
        LOGINFOLINE("[ChunkQueue] Destructor called");
        if (std::is_trivially_destructible<T>::value) return;
        for (auto& slot : buffer) {
            slot.~T();  // 显式调用析构函数
        }
    }

    // 队列大小仅支持2^n；后续看是否有内存调整需求
    static_assert(((N & (N - 1)) == 0),
        "Queue size N must be in the form of 2^n (e.g., 1, 2, 4, 8, 16, 32, 64, 128, 256, 512...)");
    // push, pop_ptr, borrow_slot 已经被移除，因为 head 是由每个 subscriber 自己管理的
    
    void commit_slot() {
        LOGINFOLINE("[ChunkQueue] commit_slot called");
        uint64_t expected = tail.load(std::memory_order_acquire);
        uint64_t desired = (expected + 1) & (N - 1);

        LOGINFOLINE("[ChunkQueue] commit_slot - expected: %d, desired: %d", expected, desired);
        while (!tail.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_acquire)) {
            desired = (expected + 1) & (N - 1);
        }
        LOGINFOLINE("[ChunkQueue] commit_slot - tail updated to: %d", desired);
    }

};

} //
