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
    Subscriber(const std::string& topic)
        : shm_mgr_(topic + "_shm", SHM_SIZE, false),
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
        bip::scoped_lock<bip::interprocess_mutex> lock(queue_->mutex);

        auto it = registry_->find(subscriber_id_);
        if (it == registry_->end()) {
            return std::nullopt;
        }

        std::size_t& head = it->second.head;
        if (timeout_ms < 0) {
            while (head == queue_->tail) {
                queue_->cond.wait(lock);
            }
        } else {
            auto abs_time = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(timeout_ms);
            while (head == queue_->tail) {
                if (!queue_->cond.timed_wait(lock, abs_time)) {
                    it->second.last_heartbeat = boost::posix_time::second_clock::universal_time();
                    return std::nullopt;
                }
            }
        }

        T* ptr = &queue_->buffer[head];
        head = (head + 1) % N;
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