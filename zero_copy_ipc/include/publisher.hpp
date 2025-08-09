// FILEPATH: include/publisher.hpp
#pragma once

#include "shared_memory.hpp"
#include "pubsub_types.hpp"
#include "chunk_queue.hpp"
#include "topic_types.hpp"
#include "ipc_error_types.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <boost/optional.hpp>
#include "ipc_utils.hpp"

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
        std::cout << "[Publisher] Creating publisher for topic: " << topic_to_string(topic) << ", queue size: " << N << std::endl;
        // 1. 获取共享内存段管理器
        auto& segment = shm_mgr_.shm();

        // 2. 找到或构造 SubscriberRegistryMap
        const ShmAllocator registry_allocator(segment.get_segment_manager());
        registry_ = segment.find_or_construct<SubscriberRegistryMap>("SubscriberRegistry")(std::less<uint64_t>(), registry_allocator);
        std::cout << "[Publisher] Subscriber registry initialized." << std::endl;
        // 使用SFINAE分派到正确的创建函数
        create_queue(segment, std::integral_constant<bool, needs_stl_allocator<T>::value>());
        if(queue_) {
            std::cout << "[Publisher] ChunkQueue created." << std::endl;
        } else {
            std::cerr << "[Publisher] Failed to create ChunkQueue." << std::endl;
        }
        // queue_ = create_queue(segment, 
        //              needs_stl_allocator<T>::value ? "ChunkQueueStl" : "ChunkQueueBasic",
        //              std::integral_constant<bool, needs_stl_allocator<T>::value>());
        //     // 代办项：
        //     // 2. shared_memory.hpp切换为managed_mapped_file（动态扩容支持）
        //     // 3. pulisher.cpp调整内存估算公式（避免容器扩容失败）
        //     // 1. pubsub_types.hpp封装std::stack（完全隔离）
        //     // 例如：
        //     // template <typename T>
        //     // struct stack {
        //     // private:
        //     //     deque<T> c;
        //     // public:
        //     //     explicit stack(const typename deque<T>::allocator_type& alloc) : c(alloc) {}
        //     //     void push(const T& val) { c.push_back(val); }
        //     //     void pop() { c.pop_back(); }
        //     //     T& top() { return c.back(); }
        //     //     // 实现其他必要接口...
        //     // };

        cache_ = segment.find_or_construct<PublisherCache>("PublisherCache")();
        std::cout << "[Publisher] PublisherCache created." << std::endl;
    }

    class LoanHandle {
    public:
        LoanHandle(T* ptr = nullptr, ChunkQueue<T, N>* queue = nullptr)
            : ptr_(ptr), queue_(queue), published_(false) {}

        bool publish() {
            if (published_ || !ptr_) return false;
            queue_->commit_slot();
            published_ = true;
            return true;
        }

        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }

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

        ~LoanHandle(){}

    private:
        T* ptr_;
        ChunkQueue<T, N>* queue_;
        bool published_;
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
            return LoanResult(IpcErrorType::LoanQueueNotCreated);
        }

        // 快速检查缓存
        const uint64_t cached_head = cache_->cached_slowest_head.load(std::memory_order_acquire);
        if (((queue_->tail + 1) & (N - 1)) != cached_head) {
            T* slot = &queue_->buffer[queue_->tail];
            return LoanResult(IpcErrorType::NoError, std::move(LoanHandle(slot, queue_)));
        }
    
        // 如果缓存不命中，遍历所有订阅者，找到最慢的一个
        if (registry_ && !registry_->empty()) {
            uint64_t slowest_head = queue_->tail;
            uint64_t max_dist = 0;
            for (const auto& pair : *registry_) {
                uint64_t head = pair.second.head.load(std::memory_order_acquire); // 原子读取
                uint64_t dist = (queue_->tail - head + N) & (N - 1);
                if (dist > max_dist) {
                    max_dist = dist;
                    slowest_head = head; // 可能出现的错误：如sub1是slowest,但在遍历到sub5时，sub1已经前进，此时可以写数据却未写
                    // 可能的解决方案：进行两次for循环，如一致则继续。但耗时增加
                }
            }
        
            if (((queue_->tail + 1) & (N - 1)) == slowest_head) {
                std::cout << "[Publisher] LoanBufferFull." << std::endl;
                return LoanResult(IpcErrorType::LoanBufferFull);
            }
             // 更新缓存
            cache_->cached_slowest_head.store(slowest_head, std::memory_order_release);
        } else {
            std::cout << "[Publisher] LoanNoSubscriber." << std::endl;
            return LoanResult(IpcErrorType::LoanNoSubscriber);
        }
    
        T* slot = &queue_->buffer[queue_->tail];
        return LoanResult(IpcErrorType::NoError, std::move(LoanHandle(slot, queue_)));
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
};

} // namespace zero_copy_ipc
