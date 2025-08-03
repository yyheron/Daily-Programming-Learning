#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> // Needed for ptime
#include <atomic>
#include <optional>
#include "ipc_error_types.hpp"

constexpr std::size_t DEFAULT_QUEUE_SIZE = 63;
// constexpr std::size_t SHM_SIZE = 1024 * 1024 * 200; // 200M

namespace zero_copy_ipc {

// The information stored for each subscriber in the registry
struct SubscriberInfo {
    alignas(64) std::atomic<uint64_t> head; // 原子变量，避免伪共享
    uint64_t last_heartbeat; // 用uint64_t替换ptime
    // uint8_t ressure_level;  // 0-255表示消费压力
    // uint8_t pressure_level;  // 0-255表示生产压力

    SubscriberInfo(uint64_t h, uint64_t t)
        : head(h), last_heartbeat(t) {}
    // SubscriberInfo(uint64_t h, uint64_t t, uint8_t ressure_level, uint8_t pressure_level)
    //     : head(h), last_heartbeat(t), ressure_level(ressure_level), pressure_level(pressure_level) {}
    SubscriberInfo(const SubscriberInfo&) = delete;
    SubscriberInfo& operator=(const SubscriberInfo&) = delete;
    SubscriberInfo(SubscriberInfo&&) = default;
    SubscriberInfo& operator=(SubscriberInfo&&) = default;
    SubscriberInfo() = default;
};

// The allocator for the map, which uses the shared memory segment manager
using ShmAllocator = boost::interprocess::allocator<
    std::pair<const uint64_t, SubscriberInfo>,
    boost::interprocess::managed_shared_memory::segment_manager
>;

// The map itself, which stores subscriber IDs and their info
using SubscriberRegistryMap = boost::interprocess::map<
    uint64_t,
    SubscriberInfo,
    std::less<uint64_t>,
    ShmAllocator
>;

struct PublisherCache {
    // 多publisher场景下，使用atomic来存储。但当前实现中只有一个publisher
    alignas(64) std::atomic<uint64_t> cached_slowest_head{0};
};
} // namespace zero_copy_ipc
