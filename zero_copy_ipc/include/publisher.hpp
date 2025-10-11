// FILEPATH: include/publisher.hpp
#pragma once

#include "shared_memory.hpp"
#include "pubsub_types.hpp"
#include "chunk_queue.hpp"
#include "topic_types.hpp"
#include "ipc_error_types.hpp"
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <chrono>
#include <boost/optional.hpp>
#include "ipc_utils.hpp"
#include "loghelper.h"

namespace zero_copy_ipc {

using namespace boost::interprocess;

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Publisher {
public:
    // 队列大小仅支持2^n；后续看是否有内存调整需求
    static_assert(((N & (N - 1)) == 0),
        "Queue size N must be in the form of 2^n (e.g., 1, 2, 4, 8, 16, 32, 64, 128, 256, 512...)");

    // SFINAE重载：处理需要STL分配器的情况
    template <typename Shm>
    void create_queue(Shm& shm, std::true_type /* needs_allocator */) {
        const ShmStlAllocator<T> allocator(shm.get_segment_manager());
        queue_ = shm.template find_or_construct<ChunkQueue<T, N>>("ChunkQueueStl")(allocator);
    }

    // SFINAE重载：处理不需要STL分配器的情况
    template <typename Shm>
    void create_queue(Shm& shm, std::false_type /* needs_allocator */) {
        queue_ = shm.template find_or_construct<ChunkQueue<T, N>>("ChunkQueueBasic")();
    }
    
    Publisher(Topic topic)
        : shm_mgr_(topic_to_string(topic) + "_shm", N * (sizeof(T) + 500), true) // 1. 只创建共享内存，不打开
    {
        LOGINFOLINE("[Publisher] Creating publisher for topic: %s, queue size: %zu", topic_to_string(topic).c_str(), N);
        // 获取共享内存段管理器
        auto& segment = shm_mgr_.shm();

        // 找到或构造 SubscriberRegistryMap
        const SubscriberRegMapAllocator registry_allocator(segment.get_segment_manager());
        registry_ = segment.find_or_construct<SubscriberRegistryMap>("SubscriberRegistry")(std::less<uint64_t>(), registry_allocator);
        if (!registry_) {
            LOGERRLINE("[Publisher] Failed to create SubscriberRegistryMap!");
            // 添加错误处理逻辑，如抛出异常或终止初始化
            throw std::runtime_error("Failed to initialize subscriber registry");
        }

        // 使用SFINAE分派到正确的创建函数，以创建ChunkQueue
        create_queue(segment, std::integral_constant<bool, needs_stl_allocator<T>::value>());
        if(!queue_) {
            LOGERRLINE("[Publisher] Failed to create ChunkQueue.");
        }
        // queue_ = create_queue(segment, 
        //              needs_stl_allocator<T>::value ? "ChunkQueueStl" : "ChunkQueueBasic",
        //              std::integral_constant<bool, needs_stl_allocator<T>::value>());
        //     // 代办项：
        //     // 1. shared_memory.hpp切换为managed_mapped_file（动态扩容支持）
        //     // 2. pulisher.cpp调整内存估算公式（避免容器扩容失败）
        //     // 3. pubsub_types.hpp中的各个容器测试

        // 找到或构造 SemaphoreMap
        const SemaphoreMapAllocator semaphore_allocator(segment.get_segment_manager());
        semaphore_ = segment.find_or_construct<SemaphoreMap>("SemaphoreMap")(std::less<uint64_t>(), semaphore_allocator);
        if (!semaphore_) {
            LOGERRLINE("[Publisher] Failed to create PublisherSemaphore!");
            throw std::runtime_error("Failed to initialize publisher semaphore");
        }

        // 找到或构造 PublisherCache
        cache_ = segment.find_or_construct<PublisherCache>("PublisherCache")();
        if (!cache_) {
            LOGERRLINE("[Publisher] Failed to create PublisherCache!");
            throw std::runtime_error("Failed to initialize publisher cache");
        }
    }

    class LoanHandle {
    public:
        LoanHandle(T* ptr = nullptr, ChunkQueue<T, N>* queue = nullptr, SemaphoreMap* semaphore = nullptr)
            : ptr_(ptr), queue_(queue), semaphore_(semaphore)  {
        }

        IpcErrorType publish() {
            LOGINFOLINE("[LoanHandle] publish() - semaphore_ pointer: %p", semaphore_);
            if (!ptr_ || !semaphore_ || !queue_) {
                if (!ptr_) return IpcErrorType::PublishSlotNotCreated;
                if (!semaphore_) return IpcErrorType::PublishSemaphoreNotCreated;
                if (!queue_) return IpcErrorType::PublishQueueNotCreated;
                return IpcErrorType::NoError;
            }
            queue_->commit_slot();
            // 通知所有订阅者
            if (!semaphore_->empty()) {
                for (auto& pair : *semaphore_) {
                    pair.second.post();
                }
            } else {
                return IpcErrorType::PublishNoSubscriber;
            }

            return IpcErrorType::NoError;
        }

        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }

        LoanHandle(const LoanHandle&) = delete;
        LoanHandle& operator=(const LoanHandle&) = delete;
        LoanHandle(LoanHandle&& other) noexcept
            : ptr_(other.ptr_), queue_(other.queue_), semaphore_(other.semaphore_) {
            other.ptr_ = nullptr;
            other.queue_ = nullptr;
            other.semaphore_ = nullptr;
        }
        LoanHandle& operator=(LoanHandle&& other) noexcept {
            if (this != &other) {
                ptr_ = other.ptr_;
                queue_ = other.queue_;
                semaphore_ = other.semaphore_;
                other.ptr_ = nullptr;
                other.queue_ = nullptr;
                other.semaphore_ = nullptr;
            }
            return *this;
        }

        ~LoanHandle(){}

    private:
        T* ptr_;
        ChunkQueue<T, N>* queue_;
        SemaphoreMap* semaphore_;
    };


    class LoanResult {
    public:
        // 错误情况：只有错误状态，没有handle
        LoanResult(IpcErrorType error)
            : errSts_(error), handle_(boost::none) {}
        
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
        boost::optional<LoanHandle>& buffer() { return handle_;}

    private:
        IpcErrorType errSts_;
        boost::optional<LoanHandle> handle_;
    };

    LoanResult loan() {
        if(!queue_) {
            LOGERRLINE("[Publisher] LoanQueueNotCreated.");
            return LoanResult(IpcErrorType::LoanQueueNotCreated);
        }

        // 快速检查缓存
        const uint64_t cached_head = cache_->cached_slowest_head.load(std::memory_order_acquire);
        if (((queue_->tail + 1) & (N - 1)) != cached_head) {
            T* slot = &queue_->buffer[queue_->tail].data;
            return LoanResult(IpcErrorType::NoError, std::move(LoanHandle(slot, queue_, semaphore_)));
        }
    
        // 如果缓存不命中，遍历所有订阅者，找到最慢的一个
        if (registry_ && !registry_->empty()) {
            uint64_t slowest_head = queue_->tail;
            uint64_t max_dist = 0;
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            // auto now = get_current_time_ms();            
            const auto timeout = 5000; // 5秒超时阈值
            for (auto it = registry_->begin(); it != registry_->end();) {
                uint64_t head = it->second.head.load(std::memory_order_acquire); // 原子读取
                uint64_t dist = (queue_->tail - head + N) & (N - 1);

                // 检查心跳超时
                if (now - it->second.last_heartbeat > timeout) {
                    LOGINFOLINE("[Publisher] Removing stale subscriber %lu (heartbeat timeout)", it->first);
                    LOGINFOLINE("[Publisher] Subscriber %lu last heartbeat: %lu ms, now: %lu ms, timeout: %lu ms", it->first, it->second.last_heartbeat, now, timeout);
                    semaphore_->erase(it->first); // 删除对应的信号量
                    it = registry_->erase(it); // 安全删除订阅者
                    cache_->cached_slowest_head.store(queue_->tail, std::memory_order_release); // 重置缓存
                } else {
                    if (dist > max_dist) {
                        max_dist = dist;
                        slowest_head = head; // 可能出现的错误：如sub1是slowest,但在遍历到sub5时，sub1已经前进，此时可以写数据却未写
                        // 可能的解决方案：进行两次for循环，如一致则继续。但耗时增加
                    }
                    it++;
                }
            }
        
            if (((queue_->tail + 1) & (N - 1)) == slowest_head) {
                LOGERRLINE("[Publisher] LoanBufferFull.");
                return LoanResult(IpcErrorType::LoanBufferFull);
            }
             // 更新缓存
            cache_->cached_slowest_head.store(slowest_head, std::memory_order_release);
        } else {
            LOGERRLINE("[Publisher] LoanNoSubscriber.");
            return LoanResult(IpcErrorType::LoanNoSubscriber);
        }
    
        T* slot = &queue_->buffer[queue_->tail].data;
        return LoanResult(IpcErrorType::NoError, std::move(LoanHandle(slot, queue_, semaphore_)));
    }
    
    // Todo: 生产者根据压力状态调整生产节奏
    // void publisher_loop() {
    //     while (running) {
    //         float max_pressure = query_subscriber_pressure();
    //         adjust_publish_rate(max_pressure);
    //         // ...
    //     }
    // }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
    PublisherCache* cache_;
    SemaphoreMap* semaphore_;
};

} // namespace zero_copy_ipc