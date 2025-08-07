#include "publisher.hpp"
#include "subscriber.hpp"
#include "types.hpp"
#include "topic_types.hpp"
#include "ipc_error_types.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cstring>
#include <string>
#include <thread> 

// 基于回调的延迟测试函数
auto callback = [](auto msg) {
    auto recv_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    double latency = recv_time - msg->timestamp_ns;
    std::cout << "Received id: " << msg->id << " latency: " << latency << "ns" << std::endl;
    // 统计延迟
    static std::vector<double> latencies;
    latencies.push_back(latency);
    // 计算统计信息
    if (latencies.size() >= 100) {
        double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
        double mean = sum / latencies.size();
        double sq_sum = std::inner_product(latencies.begin(), latencies.end(), latencies.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / latencies.size() - mean * mean);
        std::cout << "Avg latency: " << mean << "ns, Stdev: " << stdev << "ns, Min: " << latencies.front() << "ns, Max: " << latencies.back() << "ns" << std::endl;
        latencies.clear();
    }
};

template<typename MessageT>
void test_callback_latency(const char* test_name, size_t data_size, int num_samples = 100) {
    using namespace zero_copy_ipc;

    Publisher<MessageT> publisher(Topic::CameraToRobot);
    Subscriber<MessageT> subscriber(Topic::CameraToRobot, callback);

    // 等待订阅者初始化
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 发布测试消息
    for (uint64_t i = 1; i <= num_samples; ++i) {
        auto loanResult = publisher.loan();
        if (loanResult.buffer().has_value()) {
            auto& sample = loanResult.buffer().value();
            sample->id = i;
            sample->timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::memset(sample->data, 'A', data_size);
            sample.publish();
            std::cout << test_name << "publish " << i << " message" << std::endl;
        } else {
            std::cerr << test_name << " - Unable to loan sample at sample " << i << ". due to " << static_cast<int>(loanResult.status()) << std::endl;
        }

    }
    // 等待一段时间以确保所有消息都被处理
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

int main() {
    using namespace zero_copy_ipc;
    // 测试不同数据大小的延迟
    std::cout << "Testing 1K messages" << std::endl;
    test_callback_latency<ExampleMessage1K>("Callback Test 1K", 1024, 100);

    std::cout << "Testing 1M messages" << std::endl;
    test_callback_latency<ExampleMessage1M>("Callback Test 1M", 1024 * 1024, 100);

    std::cout << "Testing 10M messages" << std::endl;
    test_callback_latency<ExampleMessage10M>("Callback Test 10M", 10 * 1024 * 1024, 100);

    std::cout << "Testing 20M messages" << std::endl;
    test_callback_latency<ExampleMessage20M>("Callback Test 20M", 20 * 1024 * 1024, 100);
}
