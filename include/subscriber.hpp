#pragma once

#include "shared_memory.hpp"
#include "chunk_queue.hpp"
#include "ipc_utils.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <optional>
#include <atomic>
#include <unistd.h>

namespace zero_copy_ipc {

using bip = boost::interprocess;

constexpr std::size_t DEFAULT_QUEUE_SIZE = 128;
constexpr std::size_t SHM_SIZE = 1024 * 1024; // 1MB

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Subscriber {
public:
    Subscriber(Topic topic)
        : shm_mgr_(topic_to_string(topic) + "_shm", SHM_SIZE, false),
          subscriber_id_(generate_unique_id())
    {
        auto& shm = shm_mgr_.shm();

        auto queue_result = shm.find<ChunkQueue<T, N>>("ChunkQueue");
        if (queue_result.first) {
            queue_ = queue_result.first;
        } else {
            // 错误处理：队列不存在
        }

        auto registry_result = shm.find<SubscriberRegistryMap>("SubscriberRegistry");
        if (registry_result.first) {
            registry_ = registry_result.first;
        } else {
            // 错误处理：注册表不存在
        }

        register_self();
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

        // 取出原子 head 的值
        std::size_t head = it->second.head.load(std::memory_order_acquire);

        // 1. subscriber 已经读到队列的尾部，等待生产者写入
        // 由于每个sub都会锁一下，是否对pub的影响较大？仍需探索lock_free方案
        bip::scoped_lock<bip::interprocess_mutex> qlock(queue_->mutex);
        if (timeout_ms < 0) {
            while (head == queue_->tail) {
                // 条件变量的作用是让 Subscriber 在“无新数据”时休眠，只有 Publisher 发布新数据时才唤醒。
                // 如果使用无锁方案，忙等会浪费大量 CPU，尤其是 Subscriber 很多时。
                // 如果数据发布需要极高性能、低延迟、数据频繁，则可以去掉此处的qlock以及条件变量。
                queue_->cond.wait(qlock);
            }
        } else {
            auto abs_time = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(timeout_ms);
            while (head == queue_->tail) {
                if (!queue_->cond.timed_wait(qlock, abs_time)) {
                    it->second.last_heartbeat = boost::posix_time::second_clock::universal_time();
                    return std::nullopt;
                }
            }
        }
        // 2. 等到新数据后，立即释放队列锁
        qlock.unlock();

        // 3. 读取数据
        T* ptr = &queue_->buffer[head];

        // 4. 原子推进 head，无需加锁
        head.store((local_head + 1) % N, std::memory_order_release);
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
            SubscriberInfo info = {queue_->tail, boost::posix_time::second_clock::universal_time()};
            registry_->insert({subscriber_id_, info});
        }
    }

    void unregister_self() {
        if (registry_) {
            registry_->erase(subscriber_id_);
        }
    }

    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
    SubscriberRegistryMap* registry_;
    uint64_t subscriber_id_;
};

} // namespace zero_copy_ipc