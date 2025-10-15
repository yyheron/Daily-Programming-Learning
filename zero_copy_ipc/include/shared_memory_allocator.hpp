#pragma once

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
#include <functional>
#include <stack>
#include <queue>
#include <forward_list>
#include <set>
#include <map>
#include <mutex>

#include "shared_memory.hpp"
#include "pubsub_types.hpp"
#include "loghelper.h"

// // 共享内存STL分配器
// template <typename T>
// class ShmStlAllocator {
// public:
//     using value_type = T;
//     using pointer = T*;
//     using const_pointer = const T*;
//     using reference = T&;
//     using const_reference = const T&;
//     using size_type = std::size_t;
//     using difference_type = std::ptrdiff_t;
//     using SegmentManager = boost::interprocess::managed_shared_memory::segment_manager;
//     template <typename U>
//     struct rebind {
//         using other = ShmStlAllocator<U>;
//     };
//     ShmStlAllocator() = delete;
//     ShmStlAllocator(SegmentManager* segment_manager, DynamicMemoryPool* memory_pool = nullptr)
//         : segment_manager_(segment_manager), memory_pool_(memory_pool) {
//         if (!memory_pool_) {
//             // 如果没有提供内存池，创建一个
//             memory_pool_ = segment_manager_->find_or_construct<DynamicMemoryPool>("DynamicMemoryPool")(segment_manager, 1024 * 1024); // 初始1MB
//         }
//     }
//     template <typename U>
//     ShmStlAllocator(const ShmStlAllocator<U>& other)
//         : segment_manager_(other.segment_manager_), memory_pool_(other.memory_pool_) {}
//     T* allocate(std::size_t n) {
//         size_t size = n * sizeof(T);
//         void* ptr = memory_pool_->allocate(size);
//         if (!ptr) {
//             // 尝试扩容内存池
//             if (memory_pool_->resize(memory_pool_->size() * 2)) {
//                 ptr = memory_pool_->allocate(size);
//             }
//         }
//         return static_cast<T*>(ptr);
//     }
//     void deallocate(T* p, std::size_t) {
//         memory_pool_->deallocate(p);
//     }
//     SegmentManager* segment_manager() const { return segment_manager_; }
//     DynamicMemoryPool* memory_pool() const { return memory_pool_; }
// private:
//     SegmentManager* segment_manager_;
//     DynamicMemoryPool* memory_pool_;
//     template <typename U>
//     friend class ShmStlAllocator;
// };

namespace zero_copy_ipc {

// The allocator for the map, which uses the shared memory segment manager
using SubscriberRegMapAllocator = boost::interprocess::allocator<
    std::pair<const uint64_t, SubscriberInfo>,
    boost::interprocess::managed_mapped_file::segment_manager
>;

// The map itself, which stores subscriber IDs and their info
using SubscriberRegistryMap = boost::interprocess::map<
    uint64_t,
    SubscriberInfo,
    std::less<uint64_t>,
    SubscriberRegMapAllocator
>;

using SemaphoreMapAllocator = boost::interprocess::allocator<
    std::pair<const uint64_t, interprocess_semaphore>,
    boost::interprocess::managed_mapped_file::segment_manager
>;

using SemaphoreMap = boost::interprocess::map<
    uint64_t,
    interprocess_semaphore,
    std::less<uint64_t>,
    SemaphoreMapAllocator
>;

// 可动态扩容的共享内存分配器
template <typename T, typename SegmentManager>
class DynamicShmAllocator : public boost::interprocess::allocator<T, SegmentManager> {

public:

    using value_type = T;
    using base_allocator = boost::interprocess::allocator<T, SegmentManager>;
    using pointer = typename base_allocator::pointer;
    // using const_pointer = typename base_allocator::const_pointer;
    // using reference = typename base_allocator::reference;
    // using const_reference = typename base_allocator::const_reference;
    using size_type = typename base_allocator::size_type;
    // using difference_type = typename base_allocator::difference_type;
    // using segment_manager = SegmentManager;
    // using propagate_on_container_move_assignment = typename base_allocator::propagate_on_container_move_assignment;

    DynamicShmAllocator(SharedMemoryManager* shm_manager, std::function<void()> on_expand_callback = nullptr)
        : base_allocator(shm_manager->get_segment_manager())
        , m_segment_manager(shm_manager->get_segment_manager())
        , m_shm_manager(shm_manager)
        , m_on_expand_callback(on_expand_callback)
    {
        LOGINFOLINE("[DynamicShmAllocator] Created for topic %s", m_shm_manager ? m_shm_manager->get_segment_name().c_str() : "unknown");
    }

    template <typename U>
    DynamicShmAllocator(const DynamicShmAllocator<U, SegmentManager>& other)
        : base_allocator(other.get_segment_manager())
        , m_segment_manager(other.get_segment_manager())
        , m_shm_manager(other.get_shm_manager())
        , m_on_expand_callback(other.get_expansion_callback())
    {
    }

    // 比较运算符（STL容器要求）
    template <typename U>
    bool operator==(const DynamicShmAllocator<U, SegmentManager>&) const { return true; }
    
    template <typename U>
    bool operator!=(const DynamicShmAllocator<U, SegmentManager>&) const { return false; }

    // 分配内存，处理扩容逻辑
    pointer allocate(size_type n)
    {
        try 
        {
            // 首先尝试正常分配
            LOGINFOLINE("[DynamicShmAllocator] Allocating %zu bytes", n * sizeof(T));
            return base_allocator::allocate(n);
        }
        catch (const boost::interprocess::bad_alloc&)
        {
            if (m_shm_manager)
            {
                // std::lock_guard<std::mutex> lock(m_alloc_mutex); // 由于是一发多收，所以扩容在同一时间只能有一个线程进行，实际上没必要加锁
                // 通知publisher和subscriber有扩容需求
                if (m_on_expand_callback) 
                {
                    m_on_expand_callback();
                }
                size_t grow_size = m_shm_manager->get_grow_size();
                if (m_segment_manager->grow(grow_size))
                {
                    LOGINFOLINE("[DynamicShmAllocator] Expanded shared memory by %d bytes for topic %s", grow_size, m_shm_manager->get_segment_name().c_str());
                }
                else
                {
                    LOGERRLINE("[DynamicShmAllocator] Failed to expand shared memory");
                }
            }

            // 再次尝试分配
            return base_allocator::allocate(n);
        }
    }
            
    
    // 解分配内存
    void deallocate(pointer p, size_type n)
    {
        base_allocator::deallocate(p, n);
    }
    
    // 获取段管理器（用于复制构造）
    SegmentManager* get_segment_manager() const { return m_segment_manager; }
    SharedMemoryManager* get_shm_manager() const { return m_shm_manager; }
    std::function<void()> get_expansion_callback() const { return m_on_expand_callback; }

private:

    SegmentManager* m_segment_manager;
    size_t m_expansion_size; // 每次扩容的大小
    SharedMemoryManager* m_shm_manager; // 共享内存管理器指针
    std::function<void()> m_on_expand_callback; // 扩容回调
    // std::mutex m_alloc_mutex; // 保护分配操作
};

// 创建可监控的共享内存向量类型
template <typename T>
using ShmStlAllocator = DynamicShmAllocator<T, boost::interprocess::managed_mapped_file::segment_manager>;


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