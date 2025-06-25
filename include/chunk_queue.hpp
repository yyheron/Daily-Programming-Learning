#pragma once

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <cstddef>

namespace zero_copy_ipc {

using bip = boost::interprocess;

// 环形缓冲区，支持多进程安全
template<typename T, std::size_t N>
struct ChunkQueue {
    T buffer[N];
    std::size_t tail = 0;
    bip::interprocess_mutex mutex;
    bip::interprocess_condition cond;

    // push, pop_ptr, borrow_slot 已经被移除，因为 head 是由每个 subscriber 自己管理的
    
    void commit_slot() {
        bip::scoped_lock<bip::interprocess_mutex> lock(mutex);
        tail = (tail + 1) % N;
    }

};

} //