#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <mutex>
#include <atomic>
#include <cstring>
#include <list>
#include "pubsub_types.hpp"

namespace zero_copy_ipc {

// 动态内存池类，用于管理共享内存中的动态分配部分：由于chunkqueue是连续的，所以内存池也是连续分配的，不会产生内存碎片
class DynamicMemoryPool {
public:
    using SegmentManager = boost::interprocess::managed_shared_memory::segment_manager;
    using Allocator = boost::interprocess::allocator<char, SegmentManager>;
    using BlockInfoAllocator = boost::interprocess::allocator<MemoryBlockInfo, SegmentManager>;
    using BlockInfoVector = boost::interprocess::vector<MemoryBlockInfo, BlockInfoAllocator>;

    DynamicMemoryPool(SegmentManager& segment, std::size_t initial_size)
        : segment_manager_(segment_manager), total_size_(initial_size)
    {
        // 分配初始内存块
        memory_ = segment_manager_->allocate(initial_size);
        // 初始化块信息，第一个块是整个可用空间
        blocks_.reserve(100); // 预分配空间以避免频繁扩容
        blocks_.emplace_back(MemoryBlockInfo{0, initial_size, false});
    }
    
    // 分配内存
    void* allocate(size_t size) {
        // 简单的首次适配算法
        for (auto& block : blocks_) {
            if (!block.used && block.size >= size) {
                block.used = true;
                return static_cast<char*>(memory_) + block.offset;
            }
        }
        // 没有足够大的空闲块，需要扩容
        return nullptr; // 实际应该触发resize
    }
    
    // 释放内存
    void deallocate(void* ptr) {
        uint64_t offset = static_cast<char*>(ptr) - static_cast<char*>(memory_);
        for (auto& block : blocks_) {
            if (block.offset == offset && block.used) {
                block.used = false;
                // 可选：合并相邻的空闲块
                return;
            }
        }
    }
    
    // 调整大小并迁移数据
    bool resize(size_t new_size) {
        // 分配新的更大内存
        void* new_memory = segment_manager_->allocate(new_size);
        if (!new_memory) return false;
        
        // 迁移已使用的数据块
        for (const auto& block : blocks_) {
            if (block.used) {
                void* src = static_cast<char*>(memory_) + block.offset;
                void* dest = static_cast<char*>(new_memory) + block.offset;
                std::memcpy(dest, src, block.size);
            }
        }
        
        // 释放旧内存并更新指针
        segment_manager_->deallocate(memory_);
        memory_ = new_memory;
        total_size_ = new_size;
        
        return true;
    }
    
    // 获取内存池总大小
    size_t size() const { return total_size_; }
    
private:
    SegmentManager* segment_manager_;
    void* memory_;
    size_t total_size_;
    BlockInfoVector blocks_;
};


} // namespace zero_copy_ipc