#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace zero_copy_ipc {

using namespace boost::interprocess;

class SharedMemoryManager {
public:
    // create: true=创建并清空, false=只打开
    SharedMemoryManager(const std::string& name, std::size_t size = 1024 * 1024, bool create = false)
        : shm_name_(get_namespaced_name(name)), is_creator_(create)
    {
        if (is_creator_) {
            setup_linux_ipc_dir();
            // Publisher (creator) logic
            publisher_lock_ = std::make_unique<named_mutex>(open_or_create, (shm_name_ + "_pub_lock").c_str());
            if (!publisher_lock_->try_lock()) {
                throw std::runtime_error("Publisher lock is already held. Only one Publisher is allowed per topic!");
            }

            // Got the lock, now we are the unique publisher.
            // Clean up any previous SHM segment before creating a new one.
            shared_memory_object::remove(shm_name_.c_str());
            try {
                shm_ = std::make_unique<managed_shared_memory>(create_only, shm_name_.c_str(), size);
            } catch (...) {
                publisher_lock_->unlock(); // Unlock if SHM creation fails
                throw; // Re-throw the exception
            }
        } else {
            // Subscriber (opener) logic
            shm_ = std::make_unique<managed_shared_memory>(open_only, shm_name_.c_str());
        }
    }

    ~SharedMemoryManager() {
        if (is_creator_) {
            // The creator is responsible for cleaning up the lock and the shared memory
            if(publisher_lock_) publisher_lock_->unlock();
            named_mutex::remove((shm_name_ + "_pub_lock").c_str());
            shared_memory_object::remove(shm_name_.c_str());
        }
    }

    managed_shared_memory& shm() { return *shm_; }

    // 提供静态方法用于手动清理遗留的共享内存
    static void remove(const std::string& name) {
        shared_memory_object::remove(name.c_str());
    }

    static std::string get_namespaced_name(const std::string& name) {
        return "ipc/" + name;
    }

    void setup_linux_ipc_dir() {
        // Create /dev/shm/ipc directory if it doesn't exist.
        // This is only relevant for Linux.
        const char* ipc_dir = "/dev/shm/ipc";
        struct stat st;
        if (stat(ipc_dir, &st) != 0) {
            // Directory doesn't exist, try to create it.
            if (mkdir(ipc_dir, 0777) != 0 && errno != EEXIST) {
                throw std::runtime_error("Failed to create directory " + std::string(ipc_dir) + ": " + strerror(errno));
            }
        } else if (!S_ISDIR(st.st_mode)) {
            throw std::runtime_error(std::string(ipc_dir) + " exists but is not a directory.");
        }
    }

private:
    std::string shm_name_;
    std::unique_ptr<managed_shared_memory> shm_;
    std::unique_ptr<named_mutex> publisher_lock_;
    bool is_creator_;
};

} // namespace zero_copy_ipc