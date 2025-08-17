#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/unordered_set.hpp> 
#include <boost/unordered_map.hpp>

#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <stack>
#include <queue>
#include <forward_list>
#include <set>
#include <map>

#include <atomic>
#include <boost/optional.hpp>
#include "ipc_error_types.hpp"

constexpr std::size_t DEFAULT_QUEUE_SIZE = 64;
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

using SemaphoreMapAllocator = boost::interprocess::allocator<
    std::pair<const uint64_t, interprocess_semaphore>,
    boost::interprocess::managed_shared_memory::segment_manager
>;

using SemaphoreMap = boost::interprocess::map<
    uint64_t,
    interprocess_semaphore,
    std::less<uint64_t>,
    SemaphoreMapAllocator
>;

// slowest head 缓存，用于快速获取最慢的subscriber
struct PublisherCache {
    // 多publisher场景下，使用atomic来存储。但当前实现中只有一个publisher
    alignas(64) std::atomic<uint64_t> cached_slowest_head{0};
};

// 添加共享内存向量分配器和类型定义
template <typename T>
using ShmStlAllocator = boost::interprocess::allocator<
    T, 
    boost::interprocess::managed_shared_memory::segment_manager
>;

template <typename CharT = char>
using basic_string = boost::interprocess::basic_string<CharT, std::char_traits<CharT>, ShmStlAllocator<CharT>>;
using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

template <typename T>
using vector = boost::interprocess::vector<T, ShmStlAllocator<T>>;

template <typename T>
using list = boost::interprocess::list<T, ShmStlAllocator<T>>;

template <typename T>
using deque = boost::interprocess::deque<T, ShmStlAllocator<T>>;

template <typename Key, typename T>
using map = boost::interprocess::map<Key, T, std::less<Key>, ShmStlAllocator<std::pair<const Key, T>>>;

template <typename T>
using set = boost::interprocess::set<T, std::less<T>, ShmStlAllocator<T>>;

template <typename T> 
using unordered_set = boost::unordered::unordered_set<T, std::hash<T>, std::equal_to<T>, ShmStlAllocator<T>>;

template <typename Key, typename T>
using unordered_map = boost::unordered::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, ShmStlAllocator<std::pair<const Key, T>>>;

template <typename T>
using stack = std::stack<T, deque<T>>;

template <typename T>
using queue = std::queue<T, deque<T>>;

template <typename T>
using priority_queue = std::priority_queue<T, vector<T>>;

template <typename T>
using forward_list = std::forward_list<T, ShmStlAllocator<T>>;

template <typename T>
using multiset = std::multiset<T, std::less<T>, ShmStlAllocator<T>>;

template <typename Key, typename T>
using multimap = std::multimap<Key, T, std::less<Key>, ShmStlAllocator<std::pair<const Key, T>>>;

template <typename T1, typename T2>
using pair = std::pair<T1, T2>;
} // namespace zero_copy_ipc
