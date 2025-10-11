#pragma once

#define BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SHARED_MEMORY
#define BOOST_INTERPROCESS_SHARED_DIR_PATH "/dev/shm/ipc"

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace zero_copy_ipc {

using namespace boost::interprocess;

class SharedMemoryManager {
public:
    // create: true=创建并清空, false=只打开
    SharedMemoryManager(const std::string& name, std::size_t size = 1024 * 50, bool create = false) // 增加默认大小
        : shm_name_(name), is_creator_(create), file_path_()
    {
        ipcdetail::shared_filepath(shm_name_.c_str(), file_path_);
        lock_file_path_ = file_path_ + ".lock";
        if (is_creator_) {
            setup_shm_ipc_dir();
            std::ofstream lock_file(lock_file_path_);
            if (!lock_file) {
                throw std::runtime_error("Failed to create lock file");
            }
            lock_file.close();
            publisher_lock_ = std::make_unique<file_lock>(lock_file_path_.c_str());
            std::cout << "[SharedMemoryManager] Publisher lock created: " << (publisher_lock_ ? "success" : "failed") << std::endl;
            if (!publisher_lock_->try_lock()) {
                try {
                    shm_ = std::make_unique<managed_mapped_file>(open_only, file_path_.c_str());
                } catch (const interprocess_exception& e) {
                    std::cerr << "[SharedMemoryManager] Failed to open existing mapped file: " << e.what() << std::endl;
                    throw;
                }
                std::cout << "[SharedMemoryManager] Failed to create mapped file: " << shm_name_ << ", opened existing one" << std::endl;
                is_creator_ = false; // 本进程不是唯一创建者
                return;
            }
            
            // Got the lock, now we are the unique publisher.
            // 对于managed_mapped_file，我们不需要移除，而是可以选择截断或打开现有文件
            try {
                // 尝试以读写方式打开现有文件
                try {
                    shm_ = std::make_unique<managed_mapped_file>(open_only, file_path_.c_str());
                    std::cout << "[SharedMemoryManager] Opened existing mapped file: " << file_path_ << std::endl;
                } catch (const interprocess_exception&) {
                    // 文件不存在，创建新文件
                    shm_ = std::make_unique<managed_mapped_file>(create_only, file_path_.c_str(), size);
                    std::cout << "[SharedMemoryManager] Created new mapped file: " << file_path_ << " (size: " << size << " bytes)" << std::endl;
                }
            } catch (const interprocess_exception& e) {
                std::cerr << "[SharedMemoryManager] Boost Interprocess exception: " << e.what() 
                          << ", error code: " << e.get_error_code() << std::endl;
                publisher_lock_->unlock();
                throw;
            } catch (const std::exception& e) {
                std::cerr << "[SharedMemoryManager] Standard exception: " << e.what() << std::endl;
                publisher_lock_->unlock();
                throw;
            } catch (...) {
                std::cerr << "[SharedMemoryManager] Unknown exception occurred" << std::endl;
                publisher_lock_->unlock();
                throw;
            }
        } else {
            // Subscriber (opener) logic
            shm_ = std::make_unique<managed_mapped_file>(open_only, file_path_.c_str());
        }
    }

    ~SharedMemoryManager() {
        if (is_creator_) {
            // The creator is responsible for cleaning up the lock, but not the file itself (to allow persistence)
            if(publisher_lock_) publisher_lock_->unlock();
            std::remove(lock_file_path_.c_str());
            // 注意：我们不再自动移除文件，以支持数据持久化
            // 如果需要清理文件，可以使用静态的remove方法
        }
    }

    // 获取共享内存段的引用
    managed_mapped_file& shm() { return *shm_; }

    // 提供静态方法用于手动清理文件
    static void remove(const std::string& name) {
        std::string file_path;
        std::string lock_file_path;
        ipcdetail::shared_filepath(name.c_str(), file_path);
        lock_file_path = file_path + ".lock";
        std::cout << "[SharedMemoryManager] Removing mapped file: " << file_path << std::endl;
        file_mapping::remove(file_path.c_str());
        std::remove(lock_file_path.c_str());
    }

    // 获取文件路径
    const std::string& get_file_path() const {
        return file_path_;
    }

    // 动态扩容方法
    bool resize(std::size_t new_size, bool force = false) {
        if (!is_creator_ || !shm_) {
            std::cerr << "[SharedMemoryManager] Resize failed: not creator or no valid shared memory" << std::endl;
            return false;
        }
        
        try {
            // 获取当前大小
            std::size_t current_size = shm_->get_size();
            if (new_size <= current_size && !force) {
                std::cout << "[SharedMemoryManager] New size is not larger than current size: " << current_size << " bytes" << std::endl;
                return false;
            }
            
            // 首先关闭当前的mapped file
            shm_.reset();
            
            // 使用新大小重新打开
            // 注意：boost的managed_mapped_file本身不支持动态扩容
            // 这里我们创建一个新文件并迁移数据（简化版）
            file_mapping::remove(file_path_.c_str()); // 先删除旧文件
            shm_ = std::make_unique<managed_mapped_file>(create_only, file_path_.c_str(), new_size);
            
            std::cout << "[SharedMemoryManager] Resized file to: " << new_size << " bytes" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[SharedMemoryManager] Resize failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool is_shm_ipc_dir_exist() {
        struct stat st;
        return (stat(ipc_dir_, &st) == 0);
    }

    // 目录设置方法保持不变
    void setup_shm_ipc_dir() {
        std::lock_guard<std::mutex> lock(ipc_dir_setup_mutex_);
        if (!is_shm_ipc_dir_exist()) {
            mkdir(ipc_dir_, 0777); // 创建目录，权限设置为777
        }
    }

private:
    std::string shm_name_;
    std::unique_ptr<managed_mapped_file> shm_; // 改为managed_mapped_file
    std::unique_ptr<file_lock> publisher_lock_;
    bool is_creator_;
    std::string file_path_; // 存储实际文件路径
    std::string lock_file_path_; // 存储锁文件路径
    std::mutex  ipc_dir_setup_mutex_;
    const char* ipc_dir_{BOOST_INTERPROCESS_SHARED_DIR_PATH};

};

} // namespace zero_copy_ipc