// FILEPATH: include/publisher.hpp
#pragma once

#include "shared_memory.hpp"
#include "chunk_queue.hpp"
#include "ipc_utils.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <optional>

namespace zero_copy_ipc {

using bip = boost::interprocess;

constexpr std::size_t DEFAULT_QUEUE_SIZE = 128;
constexpr std::size_t SHM_SIZE = 1024 * 1024; // 1MB

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Publisher {
public:
    Publisher(const std::string& topic)
        : shm_mgr_(topic + "_shm", SHM_SIZE, true) // 1. 创建或打开共享内存
    {
        auto& shm = shm_mgr_.shm();

        // 2. 找到或构造 ChunkQueue
        queue_ = shm.find_or_construct<ChunkQueue<T, N>>("ChunkQueue")();

        // 3. 找到或构造 SubscriberRegistryMap
        //    需要先构造一个分配器实例，并把它传给 map 的构造函数
        const ShmAllocator alloc_inst(shm.get_segment_manager());
        registry_ = shm.find_or_construct<SubscriberRegistryMap>("SubscriberRegistry")(std::less<uint64_t>(), alloc_inst);
    }

    class LoanResult {
    public:
        LoanResult(T* ptr, ChunkQueue<T, N>* queue)
            : ptr_(ptr), queue_(queue), published_(false) {}

        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }

        bool publish() {
            if (published_ || !ptr_) return false;
            queue_->commit_slot();
            queue_->cond.notify_all();
            published_ = true;
            return true;
        }

        LoanResult(const LoanResult&) = delete;
        LoanResult& operator=(const LoanResult&) = delete;
        LoanResult(LoanResult&& other) noexcept
            : ptr_(other.ptr_), queue_(other.queue_), published_(other.published_) {
            other.ptr_ = nullptr;
            other.queue_ = nullptr;
            other.published_ = true;
        }
        LoanResult& operator=(LoanResult&& other) noexcept {
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

        ~LoanResult() {
            // 可选：析构时如果未 publish，可以自动回收 slot
        }

    private:
        T* ptr_;
        ChunkQueue<T, N>* queue_;
        bool published_;
    };

    std::optional<LoanResult> loan() {
        bip::scoped_lock<bip::interprocess_mutex> lock(queue_->mutex);

        if (registry_ && !registry_->empty()) {
            std::size_t slowest_head = queue_->tail;
            std::size_t max_dist = 0;
            for (const auto& pair : *registry_) {
                std::size_t dist = (queue_->tail - pair.second.head + N) % N;
                if (dist > max_dist) {
                    max_dist = dist;
                    slowest_head = pair.second.head;
                }
            }

            if (((queue_->tail + 1) % N) == slowest_head) {
                return std::nullopt;
            }
        }

        T* slot = &queue_->buffer[queue_->tail];
        return LoanResult(slot, queue_);
    }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
};

} // namespace zero_copy_ipc