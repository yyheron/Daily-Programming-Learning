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
#include <sys/epoll.h>
#include "ipc_utils.hpp"
#include "loghelper.hpp"

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
        , epoll_fd_(-1)
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

                if (queue_result.first && registry_result.first) {
                    queue_ = queue_result.first;
                    registry_ = registry_result.first;
                    register_self();
                    LOGINFOLINE("[Subscriber %s, id %d] Connected successfully!", topic_to_string(topic).c_str(), subscriber_id_);
                    break; // Successfully connected and initialized
                } else {
                    // This is the problematic case: SHM exists, but objects don't.
                     LOGINFOLINE("[Subscriber %s, id%llu] SHM opened, but objects not found. Queue found: %s, Registry found: %s. Retrying...",
                                topic_to_string(topic).c_str(), subscriber_id_, 
                                (queue_result.first != nullptr) ? "true" : "false", 
                                (registry_result.first != nullptr) ? "true" : "false");
                    shm_mgr_.reset();
                }

            } catch (const boost::interprocess::interprocess_exception& e) {
                // This is expected if the publisher hasn't started yet.
                LOGINFOLINE("[Subscriber %s, id%llu] Failed to connect to publisher's shared memory: %s", topic_to_string(topic).c_str(), subscriber_id_, e.what());
            }

            // Check for timeout
            auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
            if (elapsed > connect_timeout_ms) {
                LOGERRLINE("[Subscriber %s, id%llu] Failed to connect to publisher's shared memory: timeout.", topic_to_string(topic).c_str(), subscriber_id_);
                throw std::runtime_error("Failed to connect to publisher's shared memory: timeout.");
            }

            // Wait before retrying
            std::this_thread::sleep_for(milliseconds(100));
        }

        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) throw std::runtime_error("epoll_create1 failed");
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = queue_->event_fd;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, queue_->event_fd, &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");

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
        if (message_thread_.joinable()) {
            return; // 已经启动
        }
        message_thread_ = std::thread(&Subscriber::take_continuously, this);
    }

    void stop() {
        running_.store(false, std::memory_order_relaxed);
        if (message_thread_.joinable()) {
            message_thread_.join();
        }
        if (epoll_fd_ != -1) {
            close(epoll_fd_);
            epoll_fd_ = -1;
        }
    }

    void take_continuously() {
        auto it = registry_->find(subscriber_id_);
        if (it == registry_->end()) {
            LOGERRLINE("[Subscriber %s, id%llu] Error: Subscriber not found in registry.", topic_to_string(topic_).c_str(), subscriber_id_);
            return;
        }

        while (running_.load(std::memory_order_relaxed)) {  // 添加退出条件检查
            // 1. 获取本地的 head 副本，用于循环判断
            uint64_t local_head = it->second.head.load(std::memory_order_acquire);
            while (local_head != queue_->tail && running_.load(std::memory_order_relaxed)) {  // 增加运行状态检查
                // 队列不为空，读取数据
                T* ptr = &queue_->buffer[local_head];
                if (!ptr) {
                    LOGERRLINE("[Subscriber %s, id%llu] Error: Null pointer received.", topic_to_string(topic_).c_str(), subscriber_id_);
                    break;
                }

                try { // 调用用户提供的回调函数处理消息
                    LOGINFOLINE("[Subscriber %s, id%llu] Processing message in callback.", topic_to_string(topic_).c_str(), subscriber_id_);
                    callback_(SubscribMessage(ptr));
                } catch (const std::exception& e) {
                    LOGERRLINE("[Subscriber %s, id%llu] Handler exception: %s", topic_to_string(topic_).c_str(), subscriber_id_, e.what());
                }

                // 原子地推进共享内存中的 head
                it->second.head.store((local_head + 1) & (N - 1), std::memory_order_release);
                it->second.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                local_head = (local_head + 1) & (N - 1); // 更新本地 head 副本
                LOGINFOLINE("[Subscriber %s, id%llu] Updated local_head to: %llu", topic_to_string(topic_).c_str(), subscriber_id_, local_head);
            }

            if (!running_) break;  // 提前退出检查

            // 队列为空，阻塞在 epoll
            epoll_event events[1];
            int nfds = epoll_wait(epoll_fd_, events, 1, -1); // 永久阻塞，直到有事件发生：这里可以增加超时选项
            if (nfds <= 0) {
                LOGERRLINE("[Subscriber %s, id%llu] epoll_wait error.", topic_to_string(topic_).c_str(), subscriber_id_);
                continue;
            }
            // 消费 eventfd
            uint64_t val;
            // while (read(queue_->event_fd, &val, sizeof(val)) > 0);
            size_t bytes_read = read(queue_->event_fd, &val, sizeof(val));
            if (bytes_read <= 0) {
                LOGERRLINE("[Subscriber %s, id%llu] Failed to read eventfd: %s, bytes_read: %llu", topic_to_string(topic_).c_str(), subscriber_id_, strerror(errno), bytes_read);
            } else {
                LOGINFOLINE("[Subscriber %s, id%llu] Read eventfd notification, val: %llu, bytes_read: %llu", topic_to_string(topic_).c_str(), subscriber_id_, val, bytes_read);
            }
            // event_fd是publisher发布的时候write一次，
            // 但每个subscriber在take_continuously的时候在buffer没有了消息之后，会把所以event_fd清空。这种逻辑是不对的。​
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
                T* ptr = &queue_->buffer[local_head];
                if (!ptr) {
                    LOGERRLINE("[Subscriber %s, id%llu] Error: Null pointer received.", topic_to_string(topic_).c_str(), subscriber_id_);
                    return boost::none;
                }
                // 原子地推进共享内存中的 head
                it->second.head.store((local_head + 1) & (N - 1), std::memory_order_release);
                it->second.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                // 消费 eventfd 中的剩余事件，确保计数器清零
                uint64_t val;
                while (read(queue_->event_fd, &val, sizeof(val)) > 0);

                // 重置 epoll 监听，确保后续事件能被捕获
                epoll_event ev;
                ev.events = EPOLLIN | EPOLLET; // 使用边缘触发模式
                ev.data.fd = queue_->event_fd;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, queue_->event_fd, &ev) == -1) {
                    LOGERRLINE("[Subscriber %s, id%llu] epoll_ctl modify failed.", topic_to_string(topic_).c_str(), subscriber_id_);
                    return boost::none;
                }

                return SubscribMessage(ptr);
            }

            // 队列为空，阻塞在 epoll
            epoll_event events[1];
            int nfds = epoll_wait(epoll_fd_, events, 1, -1); // 永久阻塞，直到有事件发生
            if (nfds <= 0) {
                LOGERRLINE("[Subscriber %s, id%llu] epoll_wait error.", topic_to_string(topic_).c_str(), subscriber_id_);
                return boost::none;
            }
            // 消费 eventfd
            uint64_t val;
            while (read(queue_->event_fd, &val, sizeof(val)) > 0);
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
            T* ptr = &queue_->buffer[local_head];
            messages.emplace_back(ptr);
            local_head = (local_head + 1) & (N - 1);
            ++count;
        }

        // 3. 批量推进 head
        if (count > 0) {
            it->second.head.store(local_head, std::memory_order_release);
            it->second.last_heartbeat = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
        if (registry_ && queue_) {
            // SubscriberInfo info = {queue_->tail, boost::posix_time::second_clock::universal_time()};
            // registry_->insert({subscriber_id_, info});
            // 不能把 SubscriberInfo 对象传给 map，根本原因是head是atomic，拷贝/移动构造自动删除
            // 而是要告诉 map 如何直接在它自己的内存里把 SubscriberInfo 对象创建出来。
            // 这个过程叫做"就地构造" (in-place construction)。
            registry_->emplace(
                std::piecewise_construct,
                std::forward_as_tuple(subscriber_id_),
                std::forward_as_tuple(queue_->tail, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
            );
        } else {
            LOGERRLINE("Subscriber not connected to publisher's shared memory.");
        }
    }

    void unregister_self() {
        if (registry_) {
            registry_->erase(subscriber_id_);
        }
    }

    Topic topic_;
    std::unique_ptr<SharedMemoryManager> shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
    uint64_t subscriber_id_;
    int epoll_fd_;
    std::atomic<bool> running_;
    OnMessageCallback callback_;
    std::thread message_thread_;
};
} // namespace zero_copy_ipc
