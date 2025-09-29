#pragma once

#include "shared_memory.hpp"
#include "pubsub_types.hpp"
#include "chunk_queue.hpp"
#include "ipc_utils.hpp"
#include "topic_types.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <boost/optional.hpp>
#include <atomic>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <utility>
#include <tuple>
#include <iostream>
#include <vector>
#include "ipc_utils.hpp"
#include "loghelper.h"

namespace zero_copy_ipc {

using namespace boost::interprocess;

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Subscriber {
public:
    class SubscribMessage {
    public:
        SubscribMessage(T* ptr) : ptr_(ptr) {}
        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }
    private:
        T* ptr_;
    };

    using OnMessageCallback = std::function<void(SubscribMessage)>;

    static_assert(((N & (N - 1)) == 0),
        "Queue size N must be in the form of 2^n (e.g., 1, 2, 4, 8, 16, 32, 64, 128, 256, 512...)");

    Subscriber(Topic topic, OnMessageCallback callback = nullptr, bool auto_start = true, int connect_timeout_ms = 50000)
        : topic_(topic)
        , shm_mgr_(nullptr)
        , subscriber_id_(generate_unique_id())
        , queue_(nullptr)
        , registry_(nullptr)
        , running_(true)
        , callback_(std::move(callback))
    {
        using namespace std::chrono;
        auto start_time = steady_clock::now();

        while (true) {
            try {
                // Try to open the shared memory
                shm_mgr_ = std::make_unique<SharedMemoryManager>(topic_to_string(topic) + "_shm");

                // If successful, find the queue and registry
                auto& segment = shm_mgr_->shm();
                
                auto queue_result = segment.find<ChunkQueue<T, N>>(needs_stl_allocator<T>::value ? "ChunkQueueStl" : "ChunkQueueBasic");
                
                auto registry_result = segment.find<SubscriberRegistryMap>("SubscriberRegistry");

                auto semaphore_result = segment.find<SemaphoreMap>("SemaphoreMap");

                if (queue_result.first && registry_result.first && semaphore_result.first) {
                    queue_ = queue_result.first;
                    registry_ = registry_result.first;
                    semaphore_ = semaphore_result.first;
                    LOGINFOLINE("[Subscriber %s, id %lu] SemaphoreMap found at %p, size: %zu", 
                        topic_to_string(topic).c_str(), subscriber_id_, 
                        semaphore_, semaphore_->size());
                    register_self();
                    LOGINFOLINE("[Subscriber %s, id %lu] Connected successfully!", topic_to_string(topic).c_str(), subscriber_id_);
                    break; // Successfully connected and initialized
                } else {
                    // This is the problematic case: SHM exists, but objects don't.
                     LOGINFOLINE("[Subscriber %s, id %lu] SHM opened, but objects not found. Queue found: %s, Registry found: %s, Semaphore found: %s. Retrying...",
                                topic_to_string(topic).c_str(), subscriber_id_, 
                                (queue_result.first != nullptr) ? "true" : "false", 
                                (registry_result.first != nullptr) ? "true" : "false", 
                                (semaphore_result.first != nullptr) ? "true" : "false");
                    LOGINFOLINE("[Subscriber %s, id %lu] SemaphoreMap find result: %p", 
                                topic_to_string(topic).c_str(), subscriber_id_, semaphore_);
                    shm_mgr_.reset();
                }

            } catch (const boost::interprocess::interprocess_exception& e) {
                // This is expected if the publisher hasn't started yet.
                LOGINFOLINE("[Subscriber %s, id %lu] Failed to connect to publisher's shared memory: %s", topic_to_string(topic).c_str(), subscriber_id_, e.what());
            }

            // Check for timeout
            auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
            if (elapsed > connect_timeout_ms) {
                LOGERRLINE("[Subscriber %s, id %lu] Failed to connect to publisher's shared memory: timeout.", topic_to_string(topic).c_str(), subscriber_id_);
                throw std::runtime_error("Failed to connect to publisher's shared memory: timeout.");
            }

            // Wait before retrying
            std::this_thread::sleep_for(milliseconds(1000));
        }

        // 自动启动
        if (auto_start) {
            start();
        }
    }

    ~Subscriber() {
        stop();
        unregister_self();
    }

    void start() {
        if (message_thread_.joinable() || heartbeat_thread_.joinable()) {
            return; // 已启动
        }
        message_thread_ = std::thread(&Subscriber::take_continuously, this);
        heartbeat_thread_ = std::thread(&Subscriber::heartbeat_loop, this);
    }

    void stop() {
        running_.store(false, std::memory_order_relaxed);
        if (message_thread_.joinable()) {
            message_thread_.join();
        }
        if (heartbeat_thread_.joinable()) {
            heartbeat_thread_.join();
        }
    }

    void take_continuously() {
        auto it = registry_->find(subscriber_id_);
        if (it == registry_->end()) {
            LOGERRLINE("[Subscriber %s, id %lu] Error: Subscriber not found in registry.", topic_to_string(topic_).c_str(), subscriber_id_);
            return;
        }

        while (running_.load(std::memory_order_relaxed)) {  // 添加退出条件检查
            // 1. 获取本地的 head 副本，用于循环判断
            uint64_t local_head = it->second.head.load(std::memory_order_acquire);
            while (local_head != queue_->tail && running_.load(std::memory_order_relaxed)) {  // 增加运行状态检查
                // 队列不为空，读取数据
                T* ptr = &queue_->buffer[local_head].data;
                if (!ptr) {
                    LOGERRLINE("[Subscriber %s, id %lu] Error: Null pointer received.", topic_to_string(topic_).c_str(), subscriber_id_);
                    break;
                }

                try { // 调用用户提供的回调函数处理消息
                    LOGINFOLINE("[Subscriber %s, id %lu] Processing message in callback.", topic_to_string(topic_).c_str(), subscriber_id_);
                    callback_(SubscribMessage(ptr));
                } catch (const std::exception& e) {
                    LOGERRLINE("[Subscriber %s, id %lu] Handler exception: %s", topic_to_string(topic_).c_str(), subscriber_id_, e.what());
                }

                // 原子地推进共享内存中的 head
                it->second.head.store((local_head + 1) & (N - 1), std::memory_order_release);

                local_head = (local_head + 1) & (N - 1); // 更新本地 head 副本
                LOGINFOLINE("[Subscriber %s, id %lu] Updated local_head to: %lu", topic_to_string(topic_).c_str(), subscriber_id_, local_head);
            }

            if (!running_) break;  // 提前退出检查

            // 等待信号量
            auto sem = semaphore_->find(subscriber_id_);
            sem->second.wait();
            LOGDEBUGLINE("[Subscriber %s, id %lu] Woke up from semaphore wait.", topic_to_string(topic_).c_str(), subscriber_id_);
        }
    }

    boost::optional<SubscribMessage> take_one(int timeout_ms = -1) {
        auto it = registry_->find(subscriber_id_);
        if (it == registry_->end()) return boost::none;

        while (true) {
            // 获取本地的 head 副本，用于循环判断
            uint64_t local_head = it->second.head.load(std::memory_order_acquire);

            if (local_head != queue_->tail) {
                // 队列不为空，读取数据
                T* ptr = &queue_->buffer[local_head].data;
                if (!ptr) {
                    LOGERRLINE("[Subscriber %s, id %lu] Error: Null pointer received.", topic_to_string(topic_).c_str(), subscriber_id_);
                    return boost::none;
                }
                // 原子地推进共享内存中的 head
                it->second.head.store((local_head + 1) & (N - 1), std::memory_order_release);
                it->second.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                return SubscribMessage(ptr);
            }

        }
    }

    std::vector<SubscribMessage> take_batch(int max_messages = 8) {
        std::vector<SubscribMessage> messages;
        messages.reserve(max_messages);

        auto it = registry_->find(subscriber_id_);
        if (it == registry_->end()) {
            return messages;
        }

        // 1. 读取当前 head
        uint64_t local_head = it->second.head.load(std::memory_order_acquire);
        // 2. 读取当前 tail（快照）
        uint64_t tail = queue_->tail;

        int count = 0;
        while (local_head != tail && count < max_messages) {
            T* ptr = &queue_->buffer[local_head].data;
            messages.emplace_back(ptr);
            local_head = (local_head + 1) & (N - 1);
            ++count;
        }

        // 3. 批量推进 head
        if (count > 0) {
            it->second.head.store(local_head, std::memory_order_release);
            it->second.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        return messages;
    }


private:
    uint64_t generate_unique_id() {
        static std::atomic<uint32_t> counter{0};
        uint64_t pid = getpid();
        return (pid << 32) | counter++;
    }

    void register_self() {
        if (registry_ && queue_ && semaphore_) {
            // SubscriberInfo info = {queue_->tail, boost::posix_time::second_clock::universal_time()};
            // registry_->insert({subscriber_id_, info});
            // 不能把 SubscriberInfo 对象传给 map，根本原因是head是atomic，拷贝/移动构造自动删除
            // 而是要告诉 map 如何直接在它自己的内存里把 SubscriberInfo 对象创建出来。
            // 这个过程叫做"就地构造" (in-place construction)。
            registry_->emplace(
                std::piecewise_construct,
                std::forward_as_tuple(subscriber_id_),
                std::forward_as_tuple(queue_->tail, 
                                      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count())
            );
            // semaphore_->emplace(
            //     std::piecewise_construct,
            //     std::forward_as_tuple(subscriber_id_),
            //     std::forward_as_tuple(0)
            // );
            // 添加以下日志
            auto [it, inserted] = semaphore_->emplace(
                std::piecewise_construct,
                std::forward_as_tuple(subscriber_id_),
                std::forward_as_tuple(0)
            );
            if (inserted) {
                LOGINFOLINE("[Subscriber %s, id %lu] Semaphore constructed successfully at %p, initial value: 0",   
                topic_to_string(topic_).c_str(), subscriber_id_, &(it->second));
            } else {
                LOGERRLINE("[Subscriber %s, id %lu] Failed to construct semaphore! ID %lu already exists", 
                topic_to_string(topic_).c_str(), subscriber_id_, subscriber_id_);
            }
            LOGINFOLINE("[Subscriber %s, id %lu] Registered successfully.", topic_to_string(topic_).c_str(), subscriber_id_);
        } else {
            LOGERRLINE("Subscriber not connected to publisher's shared memory.");
        }
    }

    void unregister_self() {
        if (registry_) {
            registry_->erase(subscriber_id_);
        }
        if (semaphore_) {
            semaphore_->erase(subscriber_id_);
        }
        LOGINFOLINE("[Subscriber %s, id %lu] Unregistered successfully.", topic_to_string(topic_).c_str(), subscriber_id_);
    }

    void heartbeat_loop() {
        while (running_.load(std::memory_order_relaxed)) {
            if (registry_) {
                auto it = registry_->find(subscriber_id_);
                if (it != registry_->end()) {
                    // 定期更新心跳（每1秒）
                    it->second.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                    // it->second.last_heartbeat = get_current_time_ms();
                    LOGDEBUGLINE("[Subscriber %s, id %lu] Heartbeat updated in background thread", 
                        topic_to_string(topic_).c_str(), subscriber_id_);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    Topic topic_;
    std::unique_ptr<SharedMemoryManager> shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
    uint64_t subscriber_id_;
    SemaphoreMap* semaphore_;
    std::atomic<bool> running_;
    OnMessageCallback callback_;
    std::thread message_thread_;
    std::thread heartbeat_thread_;
};
} // namespace zero_copy_ipc