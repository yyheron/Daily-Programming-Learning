// FILEPATH: include/publisher.hpp
#pragma once

#include "shared_memory.hpp"
#include "pubsub_types.hpp"
#include "chunk_queue.hpp"
#include "topic_types.hpp"
#include "error_types.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <optional>

namespace zero_copy_ipc {

using namespace boost::interprocess;

constexpr std::size_t DEFAULT_QUEUE_SIZE = 128;
constexpr std::size_t SHM_SIZE = 1024 * 1024; // 1MB

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Publisher {
public:
    Publisher(Topic topic)
        : shm_mgr_(topic_to_string(topic) + "_shm", SHM_SIZE, true) // 1. 只创建共享内存，不打开
    {
        auto& shm = shm_mgr_.shm();

        // 2. 找到或构造 ChunkQueue
        queue_ = shm.find_or_construct<ChunkQueue<T, N>>("ChunkQueue")();

        // 3. 找到或构造 SubscriberRegistryMap
        //    需要先构造一个分配器实例，并把它传给 map 的构造函数
        const ShmAllocator alloc_inst(shm.get_segment_manager());
        registry_ = shm.find_or_construct<SubscriberRegistryMap>("SubscriberRegistry")(std::less<uint64_t>(), alloc_inst);
    }

    class LoanHandle {
    public:
        LoanHandle(T* ptr = nullptr, ChunkQueue<T, N>* queue = nullptr)
            : ptr_(ptr), queue_(queue), published_(false) {}

        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }

        bool publish() {
            if (published_ || !ptr_) return false;
            // 实际上有2种方案：
            // 1. 事件通知型：互斥锁+条件变量, 仅在tail更新时commit_slot()内部加锁
            // 2. 无锁，在subscriber take时混合等待：前N次快速轮询（利用CPU缓存局部性），超过阈值后调用yield()让出CPU
            queue_->commit_slot();
            queue_->cond.notify_all();
            published_ = true;
            return true;
        }

        LoanHandle(const LoanHandle&) = delete;
        LoanHandle& operator=(const LoanHandle&) = delete;
        LoanHandle(LoanHandle&& other) noexcept
            : ptr_(other.ptr_), queue_(other.queue_), published_(other.published_) {
            other.ptr_ = nullptr;
            other.queue_ = nullptr;
            other.published_ = true;
        }
        LoanHandle& operator=(LoanHandle&& other) noexcept {
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

        ~LoanHandle() {
            // 可选：析构时如果未 publish，可以自动回收 slot
        }

    private:
        T* ptr_;
        ChunkQueue<T, N>* queue_;
        bool published_;
    };


    class LoanResult {
    public:
        // 错误情况：只有错误状态，没有handle
        LoanResult(IpcErrorType error)
            : errSts_(error), handle_(std::nullopt) {}
        
        // 成功情况：有LoanHandle对象
        LoanResult(IpcErrorType error, LoanHandle&& handle)
            : errSts_(error), handle_(std::move(handle)) {}
        
        // 禁用拷贝（因为LoanHandle禁用了拷贝）
        LoanResult(const LoanResult&) = delete;
        LoanResult& operator=(const LoanResult&) = delete;
        
        // 允许移动
        LoanResult(LoanResult&& other) noexcept = default;
        LoanResult& operator=(LoanResult&& other) noexcept = default;
        
        IpcErrorType status() const {return errSts_;}
        std::optional<LoanHandle>& buffer() {
            return handle_;
        }

    private:
        IpcErrorType errSts_;
        std::optional<LoanHandle> handle_;
    };

    LoanResult loan() {

        if (registry_ && !registry_->empty()) {
            std::size_t slowest_head = queue_->tail;
            std::size_t max_dist = 0;
            for (const auto& pair : *registry_) {
                std::size_t head = pair.second.head.load(std::memory_order_acquire); // 原子读取
                std::size_t dist = (queue_->tail - head + N) % N;
                if (dist > max_dist) {
                    max_dist = dist;
                    slowest_head = head; // 可能出现的错误：如sub1是slowest,但在遍历到sub5时，sub1已经前进，此时可以写数据却未写
                    // 可能的解决方案：进行两次for循环，如一致则继续。但耗时增加
                }
            }

            if (((queue_->tail + 1) % N) == slowest_head) {
                return LoanResult(IpcErrorType::LoanBufferFull);
            }
        } else {
            return LoanResult(IpcErrorType::LoanNoSubscriber);
        }
        T* slot = &queue_->buffer[queue_->tail];
        return LoanResult(IpcErrorType::NoError, std::move(LoanHandle(slot, queue_)));
    }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
};

} // namespace zero_copy_ipc