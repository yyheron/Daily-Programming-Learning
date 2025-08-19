#pragma once

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/container/allocator_traits.hpp>
#include <cstddef>
#include <unistd.h>
#include <atomic>
#include <type_traits>
#include "ipc_utils.hpp"
#include "pubsub_types.hpp"
#include "loghelper.h"

namespace zero_copy_ipc {

using namespace boost::interprocess;

// 环形缓冲区，支持多进程安全
template<typename T, std::size_t N>
struct ChunkQueue {
private:
    // Wrapper to prevent default construction of T
    union Slot {
        T data;
        Slot() {} // Do not initialize data
        ~Slot() {} // Do not destroy data
    };

    // SFINAE overload for constructing elements that need an STL allocator.
    template <typename U = T>
    auto construct_element(void* slot, const ShmStlAllocator<T>& alloc) -> std::enable_if_t<needs_stl_allocator<U>::value> {
        new (slot) U(alloc);
    }

    // SFINAE overload for constructing elements that do not need an STL allocator.
    template <typename U = T>
    auto construct_element(void* slot) -> std::enable_if_t<!needs_stl_allocator<U>::value> {
        new (slot) U{}; // Value initialization is safer than ()
    }

public:
    Slot buffer[N]; // Use a union to avoid default construction.
    alignas(64) std::atomic<uint64_t> tail{0};
    
    // Constructor for types that NEED an allocator.
    // Enabled only when needs_stl_allocator<T> is true.
    template <typename U = T, typename = std::enable_if_t<needs_stl_allocator<U>::value>>
    explicit ChunkQueue(const ShmStlAllocator<T>& alloc) {
        for (auto& slot : buffer) {
            construct_element(&slot.data, alloc);
        }
    }
    
    // Default constructor for types that DO NOT need an allocator.
    // Enabled only when needs_stl_allocator<T> is false.
    template <typename U = T, typename = std::enable_if_t<!needs_stl_allocator<U>::value>>
    explicit ChunkQueue() {
        for (auto& slot : buffer) {
            construct_element(&slot.data);
        }
    }

    ~ChunkQueue() {
        LOGINFOLINE("[ChunkQueue] Destructor called");
        if (std::is_trivially_destructible<T>::value) return;
        for (auto& slot : buffer) {
            slot.data.~T();  // ��式调用析构函数
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