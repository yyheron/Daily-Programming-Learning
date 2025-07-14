#include "publisher.hpp"
#include "types.hpp"
#include "topic_types.hpp"
#include "ipc_error_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/chrono.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

// 模板函数，根据消息类型发送消息
template<typename MessageT>
void publish_messages(const char* test_name, int num_samples = 100000) {
    using namespace zero_copy_ipc;
    Publisher<MessageT> publisher(Topic::CameraToRobot);
    std::vector<uint64_t> copy_durations; // 用于存储每次拷贝的时间

    std::cout << test_name << " publishing started." << std::endl;
    for (uint64_t i = 1; i <= num_samples; ++i) {
        auto loanResult = publisher.loan();
        if (loanResult.buffer().has_value()) {
            auto& sample = loanResult.buffer().value();

            sample->id = i;

            // 测量拷贝时间
            auto copy_start = boost::chrono::high_resolution_clock::now();
            // 使用 std::memset 填充数据
            std::memset(sample->data, 'A', MessageT::data_size);
            auto copy_end = boost::chrono::high_resolution_clock::now();

            // 计算拷贝耗时（纳秒）
            auto copy_duration = boost::chrono::duration_cast<boost::chrono::nanoseconds>(copy_end - copy_start).count();
            copy_durations.push_back(copy_duration);

            auto now = boost::chrono::high_resolution_clock::now();
            sample->timestamp_ns = boost::chrono::duration_cast<boost::chrono::nanoseconds>(now.time_since_epoch()).count();
            sample.publish();
        } else if (loanResult.status() == IpcErrorType::LoanBufferFull) {
            std::cout << "Publish failed (queue full)" << std::endl;
        } else if (loanResult.status() == IpcErrorType::LoanNoSubscriber) {
            std::cout << "Publish failed (No Subscriber)" << std::endl;
        } else {
            std::cout << "Publish failed (Unknown Error)" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 计算统计信息
    if (!copy_durations.empty()) {
        uint64_t sum = std::accumulate(copy_durations.begin(), copy_durations.end(), 0ULL);
        double average = static_cast<double>(sum) / copy_durations.size();
        uint64_t min_val = *std::min_element(copy_durations.begin(), copy_durations.end());
        uint64_t max_val = *std::max_element(copy_durations.begin(), copy_durations.end());

        std::cout << test_name << " copy time statistics:" << std::endl;
        std::cout << "  Average: " << average << " ns" << std::endl;
        std::cout << "  Min: " << min_val << " ns" << std::endl;
        std::cout << "  Max: " << max_val << " ns" << std::endl;
    }

    std::cout << test_name << " publishing finished." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <message_type>" << std::endl;
        std::cerr << "Available message types: 1K, 1M, 10M, 20M" << std::endl;
        return 1;
    }

    std::string message_type = argv[1];

    // ./sample_publiser 1K  # 发布 1KB 消息并打印拷贝时间
    // ./sample_publiser 1M  # 发布 1MB 消息并打印拷贝时间
    // ./sample_publiser 10M # 发布 10MB 消息并打印拷贝时间
    // ./sample_publiser 20M # 发布 20MB 消息并打印拷贝时间

    if (message_type == "1K") {
        publish_messages<zero_copy_ipc::ExampleMessage1K>("Test 1KB Message");
    } else if (message_type == "1M") {
        publish_messages<zero_copy_ipc::ExampleMessage1M>("Test 1MB Message");
    } else if (message_type == "10M") {
        publish_messages<zero_copy_ipc::ExampleMessage10M>("Test 10MB Message");
    } else if (message_type == "20M") {
        publish_messages<zero_copy_ipc::ExampleMessage20M>("Test 20MB Message");
    } else {
        std::cerr << "Unknown message type: " << message_type << std::endl;
        std::cerr << "Available message types: 1K, 1M, 10M, 20M" << std::endl;
        return 1;
    }

    return 0;
}