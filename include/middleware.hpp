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
    // LoanedSample 是一个封装了 slot 指针和 publish 行为的对象
    class LoanedSample {
    public:
        LoanedSample(T* ptr, ChunkQueue<T, N>* queue)
            : ptr_(ptr), queue_(queue), published_(false) {}

        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }

        // 发布消息
        bool publish() {
            if (published_ || !ptr_) return false;
            bool result = queue_->commit_slot();
            if (result) {
                queue_->cond.notify_all();
            }
            published_ = true;
            return result;
        }

        // 禁止拷贝，允许移动
        LoanedSample(const LoanedSample&) = delete;
        LoanedSample& operator=(const LoanedSample&) = delete;
        LoanedSample(LoanedSample&& other) noexcept
            : ptr_(other.ptr_), queue_(other.queue_), published_(other.published_) {
            other.ptr_ = nullptr;
            other.queue_ = nullptr;
            other.published_ = true;
        }
        LoanedSample& operator=(LoanedSample&& other) noexcept {
            if (this != &other) {
                ptr_ = other.ptr_;
                queue_ = other.queue_;
                published_ = other.published_;
                other.ptr_ = nullptr;
                other.queue_ = nullptr;
                other.published_ = true;
            }
            return *this;
        }

        ~LoanedSample() {
            // 可选：析构时如果未 publish，可以自动回收 slot
        }

    private:
        T* ptr_;
        ChunkQueue<T, N>* queue_;
        bool published_;
    };

    // loan() 返回一个 LoanedSample
    std::optional<LoanedSample> loan() {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(queue_->mutex);
        T* slot = queue_->borrow_slot();
        if (!slot) return std::nullopt;
        return LoanedSample(slot, queue_);
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