#include <queue>
#include <mutex>
#include <condition_variable>

// 自定义线程安全的单生产者单消费者队列
template<typename T>
class ThreadSafeSPSCQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    size_t capacity_;

public:
    explicit ThreadSafeSPSCQueue(size_t capacity) : capacity_(capacity) {}

    // 尝试入队
    bool push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.size() >= capacity_) {
            return false;
        }
        queue_.push(value);
        cond_.notify_one();
        return true;
    }

    // 尝试出队
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = queue_.front();
        queue_.pop();
        return true;
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // 阻塞出队
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty(); });
        value = queue_.front();
        queue_.pop();
    }
};
