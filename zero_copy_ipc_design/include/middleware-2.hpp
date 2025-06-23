#pragma once

#include "shared_memory.hpp"
#include "chunk_queue.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <cstring>

namespace zero_copy_ipc {

constexpr std::size_t DEFAULT_QUEUE_SIZE = 128;
constexpr std::size_t SHM_SIZE = 1024 * 1024; // 1MB

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Publisher {
public:
    Publisher(const std::string& topic)
        : shm_mgr_(topic + "_shm", SHM_SIZE, true)
    {
        using namespace boost::interprocess;
        auto& shm = shm_mgr_.shm();
        queue_ = shm.find_or_construct<ChunkQueue<T, N>>("queue")();
    }

    bool publish(const T& msg) {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(queue_->mutex);
        bool result = queue_->push(msg);
        queue_->cond.notify_all();
        return result;
    }

    // 添加支持移动语义的重载
    // 但此时仍不能称为零拷贝，因为移动语言也分多种情况
    // 形象比喻
    // 你在自己家（本地内存）做了一个菜（消息对象），
    // publish 就是把菜打包送到小区食堂（共享内存 ring buffer），
    // 移动语义是你把锅直接送过去（而不是重新炒一份），
    // 但锅还是放在食堂的厨房（共享内存），你家里的锅和食堂的锅不是同一个。

    // 场景1：直接构造临时对象
    // struct Message {
    //     std::vector<int> data;  // 假设这是"锅"（动态分配的资源）
    // };
    // void publish_example() {
    //     Publisher<Message> pub("topic");
    //     // 临时对象场景
    //     pub.publish(Message{std::vector<int>{1,2,3}});  // 临时对象的vector直接移动到共享内存
    // }

    // 场景2：函数返回值
    // Message create_message() {
    //     Message msg;
    //     msg.data = std::vector<int>{1,2,3};
    //     return msg;  // 这里编译器会做RVO（返回值优化），避免拷贝
    // }
    // void publish_example2() {
    //     Publisher<Message> pub("topic");
    //     pub.publish(create_message());  // 没有额外拷贝，RVO确保直接在共享内存需要的位置构造
    // }

    // 场景3：显式移动已存在的对象
    // void publish_example3() {
    //     Publisher<Message> pub("topic");
    //     Message msg;
    //     msg.data = std::vector<int>{1,2,3};  // msg.data指向堆上分配的内存
    //     pub.publish(std::move(msg));  // msg.data的所有权转移到ring buffer
    //     // 此时msg.data为nullptr，原来的内存被ring buffer接管
    // }
    // "锅直接送过去"对应的行为
    // 1. 临时对象的情况：
    // vector的内存（"锅"）会直接移动到ring buffer，不需要重新分配内存并拷贝数据，临时对象的生命周期结束时，内存的所有权已经转移
    // 2. 返回值优化(RVO)的情况：
    // 编译器会直接在目标位置（最终ring buffer会使用的内存位置）构造对象，完全避免了拷贝和移动
    // 这就像"在食堂直接做菜"，而不是在家做好再送过去
    // 3. 显式移动的情况：
    // vector的内存所有权从msg转移到ring buffer
    // 原来msg.data指向的内存现在归ring buffer所有
    // 这就是真正的"把锅送过去"
    // 因此，需要在中间件中设计类似场景2的封装，实现placement new的功能，即，#直接在共享内存的指定位置构造对象#
    // 这个思路非常接近高性能共享内存 IPC 框架的最佳实践，完全避免拷贝和移动，实现真正的“零拷贝”！
    // 然而我发现，你通过 loan() 拿到的 slot，本身就是 ring buffer（共享内存）上的一块内存。
    // 用户直接在这块内存上“原地构造”或“原地写入”数据。是否还需要移动语义？
    // 不需要。因为你根本没有“从A搬到B”的过程，数据一开始就在B（共享内存slot）上。
    bool publish(T&& msg) {
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(queue_->mutex);
        bool result = queue_->push(std::move(msg));  // 使用 std::move
        queue_->cond.notify_all();
        return result;
    }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
};

template<typename T, std::size_t N = DEFAULT_QUEUE_SIZE>
class Subscriber {
public:
    Subscriber(const std::string& topic)
        : shm_mgr_(topic + "_shm", SHM_SIZE, false)
    {
        using namespace boost::interprocess;
        auto& shm = shm_mgr_.shm();
        queue_ = shm.find<ChunkQueue<T, N>>("queue").first;
    }

    bool take(T& msg) {
        if (!queue_) return false;
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(queue_->mutex);
        while (queue_->head == queue_->tail) {
            queue_->cond.wait(lock);
        }
        return queue_->pop(msg);
    }

private:
    SharedMemoryManager shm_mgr_;
    ChunkQueue<T, N>* queue_;
};

} //