#pragma once

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <cstddef>

namespace zero_copy_ipc {

// 环形缓冲区，支持多进程安全
template<typename T, std::size_t N>
struct ChunkQueue {
    T buffer[N];
    std::size_t head = 0;
    std::size_t tail = 0;
    boost::interprocess::interprocess_mutex mutex;
    boost::interprocess::interprocess_condition cond;

    // 入队，满则返回false
    bool push(const T& msg) {
        std::size_t next_tail = (tail + 1) % N;
        if (next_tail == head) {
            // 队列已满
            return false;
        }
        buffer[tail] = msg;
        tail = next_tail;
        return true;
    }

    // 出队，空则返回false
    bool pop(T& msg) {
        if (head == tail) {
            // 队列为空
            return false;
        }
        msg = buffer[head];
        head = (head + 1) % N;
        return true;
    }
};

} //