#include "subscriber.hpp"
#include "types.hpp"
#include "topic_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/chrono.hpp>
#include <csignal>
#include <vector>
#include <numeric>
#include <algorithm>
#include <string>
#include <atomic>

std::atomic<bool> running = true;

void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", setting running to false." << std::endl;
    running.store(false);
}

template<typename MessageT>
void test_subscriber_latency(const char* test_name) {
    using namespace zero_copy_ipc;
    Subscriber<MessageT> sub(Topic::CameraToRobot);
    std::cout << "Subscriber B started to deal with " << test_name << std::endl;
    int ctr = 0;
    
    std::vector<uint64_t> latencies_ns;

    std::cout << test_name << " started." << std::endl;
    while (ctr < 99990) {
        auto msg_opt = sub.take();
        if (msg_opt.has_value()) {
            auto& msg = msg_opt.value();
            // 获取当前时间的纳秒数
            auto now = boost::chrono::high_resolution_clock::now();
            auto nanoseconds = boost::chrono::duration_cast<boost::chrono::nanoseconds>(now.time_since_epoch()).count();
            latencies_ns.push_back(nanoseconds - msg->timestamp_ns);
            ctr++;
        } else {
            std::cout << "No message received." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    std::cout << test_name << " finished." << std::endl;

    if (!latencies_ns.empty()) {
        uint64_t min_latency = *std::min_element(latencies_ns.begin(), latencies_ns.end());
        uint64_t max_latency = *std::max_element(latencies_ns.begin(), latencies_ns.end());
        uint64_t sum_latency = std::accumulate(latencies_ns.begin(), latencies_ns.end(), 0ULL);
        double avg_latency = static_cast<double>(sum_latency) / latencies_ns.size();

        std::cout << "[" << test_name << "] Min: " << min_latency << " ns, Max: " << max_latency << " ns, Avg: " << avg_latency << " ns" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    std::cout << "SubscriberB started." << std::endl;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <message_type>" << std::endl;
        std::cerr << "Available message types: 1K, 1M, 10M, 20M" << std::endl;
        return 1;
    }

    std::string message_type = argv[1];

    // ./sample_subscriberB 1K  # 测试 1KB 消息
    // ./sample_subscriberB 1M  # 测试 1MB 消息
    // ./sample_subscriberB 10M # 测试 10MB 消息
    // ./sample_subscriberB 20M # 测试 20MB 消息

    if (message_type == "1K") {
        test_subscriber_latency<zero_copy_ipc::ExampleMessage1K>("Test 1KB Message");
    } else if (message_type == "1M") {
        test_subscriber_latency<zero_copy_ipc::ExampleMessage1M>("Test 1MB Message");
    } else if (message_type == "10M") {
        test_subscriber_latency<zero_copy_ipc::ExampleMessage10M>("Test 10MB Message");
    } else if (message_type == "20M") {
        test_subscriber_latency<zero_copy_ipc::ExampleMessage20M>("Test 20MB Message");
    } else {
        std::cerr << "Unknown message type: " << message_type << std::endl;
        std::cerr << "Available message types: 1K, 1M, 10M, 20M" << std::endl;
        return 1;
    }

    return 0;
}