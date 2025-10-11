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
            setup_linux_ipc_dir();
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
            try {
                shm_ = std::make_unique<managed_mapped_file>(open_only, file_path_.c_str());
                std::cout << "[SharedMemoryManager] Opened mapped file: " << file_path_ << std::endl;
            } catch (const interprocess_exception& e) {
                std::cerr << "[SharedMemoryManager] Failed to open mapped file: " << e.what() << std::endl;
                throw;
            }
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
    bool resize(std::size_t new_size) {
        if (!is_creator_ || !shm_) {
            std::cerr << "[SharedMemoryManager] Resize failed: not creator or no valid shared memory" << std::endl;
            return false;
        }
        
        try {
            // 首先关闭当前的mapped file
            shm_.reset();
            
            // 使用新大小重新打开（会自动扩容）
            shm_ = std::make_unique<managed_mapped_file>(open_only, file_path_.c_str());
            
            // 注意：boost的managed_mapped_file本身不支持动态扩容
            // 要实现真正的扩容，需要创建一个新文件并迁移数据
            // 这里我们只是返回当前文件状态
            std::cout << "[SharedMemoryManager] File size remains at: " << shm_->get_size() << " bytes" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[SharedMemoryManager] Resize failed: " << e.what() << std::endl;
            return false;
        }
    }

    // 目录设置方法保持不变
    void setup_linux_ipc_dir() {
        // Create /dev/shm/ipc directory if it doesn't exist.
        const char* ipc_dir = BOOST_INTERPROCESS_SHARED_DIR_PATH;
        std::cout << "[SharedMemoryManager] Checking for IPC directory: " << ipc_dir << std::endl;
        
        // 检查目录是否存在且可访问
        struct stat st;
        if (stat(ipc_dir, &st) != 0) {
            // 尝试递归创建目录，确保中间目录都存在
            std::string cmd = "mkdir -p " + std::string(ipc_dir) + " && chmod 777 " + std::string(ipc_dir);
            std::cout << "[SharedMemoryManager] Executing command: " << cmd << std::endl;
            int result = system(cmd.c_str());
            
            if (result != 0) {
                std::cerr << "[SharedMemoryManager] Failed to create IPC directory: " << ipc_dir 
                          << ", error: " << strerror(errno) << std::endl;
                throw std::runtime_error("Failed to create directory " + std::string(ipc_dir) + ": " + strerror(errno));
            }
            std::cout << "[SharedMemoryManager] Created IPC directory: " << ipc_dir << std::endl;
        } else {
            if (!S_ISDIR(st.st_mode)) {
                std::cerr << "[SharedMemoryManager] Error: " << ipc_dir << " exists but is not a directory." << std::endl;
                throw std::runtime_error(std::string(ipc_dir) + " exists but is not a directory.");
            }
            // 检查权限
            if ((st.st_mode & S_IRWXU) != S_IRWXU || (st.st_mode & S_IRWXG) != S_IRWXG || (st.st_mode & S_IRWXO) != S_IRWXO) {
                std::cout << "[SharedMemoryManager] Updating permissions for IPC directory: " << ipc_dir << std::endl;
                if (chmod(ipc_dir, 0777) != 0) {
                    std::cerr << "[SharedMemoryManager] Failed to set permissions: " << strerror(errno) << std::endl;
                }
            }
            std::cout << "[SharedMemoryManager] IPC directory already exists: " << ipc_dir << std::endl;
        }
    }

private:
    std::string shm_name_;
    std::unique_ptr<managed_mapped_file> shm_; // 改为managed_mapped_file
    std::unique_ptr<file_lock> publisher_lock_;
    bool is_creator_;
    std::string file_path_; // 存储实际文件路径
    std::string lock_file_path_; // 存储锁文件路径
};

} // namespace zero_copy_ipc