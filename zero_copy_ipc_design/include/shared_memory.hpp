#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <string>

namespace zero_copy_ipc {

class SharedMemoryManager {
public:
    // create: true=创建并清空, false=只打开
    SharedMemoryManager(const std::string& name, std::size_t size, bool create)
        : shm_name_(name)
    {
        using namespace boost::interprocess;
        if (create) {
            // 先移除旧的
            shared_memory_object::remove(shm_name_.c_str());
            shm_ = std::make_unique<managed_shared_memory>(create_only, shm_name_.c_str(), size);
        } else {
            shm_ = std::make_unique<managed_shared_memory>(open_only, shm_name_.c_str());
        }
    }

    ~SharedMemoryManager() {
        // 可选：只在创建者析构时移除共享内存
        // boost::interprocess::shared_memory_object::remove(shm_name_.c_str());
    }

    boost::interprocess::managed_shared_memory& shm() { return *shm_; }

private:
    std::string shm_name_;
    std::unique_ptr<boost::interprocess::managed_shared_memory> shm_;
};

} //