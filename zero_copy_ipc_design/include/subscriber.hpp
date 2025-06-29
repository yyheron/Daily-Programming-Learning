#pragma once

#include "shared_memory.hpp"
#include "pubsub_types.hpp"
#include "chunk_queue.hpp"
#include "ipc_utils.hpp"
#include "topic_types.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <optional>
#include <atomic>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <utility>
#include <tuple>

namespace zero_copy_ipc {

using namespace boost::interprocess;

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Subscriber {
public:
    Subscriber(Topic topic, int connect_timeout_ms = 5000)
        : shm_mgr_(nullptr), subscriber_id_(generate_unique_id()), queue_(nullptr), registry_(nullptr)
    {
        using namespace std::chrono;
        auto start_time = steady_clock::now();

        while (true) {
            try {
                // Try to open the shared memory
                shm_mgr_ = std::make_unique<SharedMemoryManager>(topic_to_string(topic) + "_shm", SHM_SIZE, false);
                
                // If successful, find the queue and registry
                auto& shm = shm_mgr_->shm();
                auto queue_result = shm.find<ChunkQueue<T, N>>("ChunkQueue");
                auto registry_result = shm.find<SubscriberRegistryMap>("SubscriberRegistry");

                if (queue_result.first && registry_result.first) {
                    queue_ = queue_result.first;
                    registry_ = registry_result.first;
                    register_self();
                    return; // Successfully connected and initialized
                }
                // If objects not found, something is wrong, but we might retry
                
            } catch (const boost::interprocess::interprocess_exception& e) {
                // This is expected if the publisher hasn't started yet.
            }

            // Check for timeout
            auto elapsed = duration_cast<milliseconds>(steady_clock::now() - start_time).count();
            if (elapsed > connect_timeout_ms) {
                throw std::runtime_error("Failed to connect to publisher's shared memory: timeout.");
            }
            
            // Wait before retrying
            std::this_thread::sleep_for(milliseconds(100));
        }
    }

    ~Subscriber() {
        unregister_self();
    }

    class SubscribMessage {
    public:
        SubscribMessage(T* ptr) : ptr_(ptr) {}
        T* operator->() { return ptr_; }
        T& operator*() { return *ptr_; }
    private:
        T* ptr_;
    };

    std::optional<SubscribMessage> take(int timeout_ms = -1) {

        auto it = registry_->find(subscriber_id_);
        if (it == registry_->end()) {
            return std::nullopt;
        }

        // 1. 获取本地的 head 副本，用于循环判断
        std::size_t local_head = it->second.head.load(std::memory_order_acquire);

        // 2. subscriber 已经读到队列的尾部，等待生产者写入
        // 由于每个sub都会锁一下，是否对pub的影响较大？仍需探索lock_free方案
        scoped_lock<interprocess_mutex> qlock(queue_->mutex);
        if (timeout_ms < 0) {
            while (local_head == queue_->tail) {
                // 条件变量的作用是让 Subscriber 在"无新数据"时休眠，只有 Publisher 发布新数据时才唤醒。
                // 如果使用无锁方案，忙等会浪费大量 CPU，尤其是 Subscriber 很多时。
                // 如果数据发布需要极高性能、低延迟、数据频繁，则可以去掉此处的qlock以及条件变量。
                queue_->cond.wait(qlock);
            }
        } else {
            auto abs_time = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(timeout_ms);
            while (local_head == queue_->tail) {
                if (!queue_->cond.timed_wait(qlock, abs_time)) {
                    it->second.last_heartbeat = boost::posix_time::second_clock::universal_time();
                    return std::nullopt;
                }
            }
        }
        // 3. 等到新数据后，立即释放队列锁
        qlock.unlock();

        // 4. 读取数据
        T* ptr = &queue_->buffer[local_head];

        // 5. 原子地推进共享内存中的 head
        it->second.head.store((local_head + 1) % N, std::memory_order_release);
        it->second.last_heartbeat = boost::posix_time::second_clock::universal_time();

        return SubscribMessage(ptr);
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
            // 这个过程叫做“就地构造” (in-place construction)。
            registry_->emplace(
                std::piecewise_construct,
                std::forward_as_tuple(subscriber_id_),
                std::forward_as_tuple(queue_->tail, boost::posix_time::second_clock::universal_time())
            );
        } else {
            std::cerr << "Subscriber not connected to publisher's shared memory." << std::endl;
        }
    }

    void unregister_self() {
        if (registry_) {
            registry_->erase(subscriber_id_);
        }
    }

    std::unique_ptr<SharedMemoryManager> shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
    uint64_t subscriber_id_;
};

} // namespace zero_copy_ipc