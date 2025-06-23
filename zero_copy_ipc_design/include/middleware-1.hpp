#pragma once

#include "shared_memory.hpp"
#include "chunk_queue.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <cstring>

namespace zero_copy_ipc {

constexpr std::size_t DEFAULT_QUEUE_SIZE = 128;
constexpr std::size_t SHM_SIZE = 1024 * 1024; // 1MB

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Publisher {
public:
    Publisher(const std::string& topic)
        : shm_mgr_(topic + "_shm", SHM_SIZE, true)
    {
        using namespace boost::interprocess;
        auto& shm = shm_mgr_.shm();
        queue_ = shm.find_or_construct<ChunkQueue<T, N>>("queue")();
    }

    // 通过调用publish(), 消息被拷贝到共享内存中。我们可以通过移动语义来避免拷贝
    bool publish(const T& msg) {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(queue_->mutex);
        bool result = queue_->push(msg);
        queue_->cond.notify_all();
        return result;
    }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
};

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Subscriber {
public:
    Subscriber(const std::string& topic)
        : shm_mgr_(topic + "_shm", SHM_SIZE, false)
    {
        using namespace boost::interprocess;
        auto& shm = shm_mgr_.shm();
        queue_ = shm.find<ChunkQueue<T, N>>("queue").first;
    }

    bool take(T& msg) {
        if (!queue_) return false;
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(queue_->mutex);
        while (queue_->head == queue_->tail) {
            queue_->cond.wait(lock);
        }
        return queue_->pop(msg);
    }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
};

} //