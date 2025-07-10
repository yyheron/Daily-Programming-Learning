// FILEPATH: f:/Repo/Daily-Programming-Learning/example/main.cpp
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
#include <thread> // Added for std::this_thread::sleep_for

template<typename MessageT>
void test_latency(const char* test_name, size_t data_size, int num_samples = 100, bool use_batch = false, int batch_size = 8) {
    using namespace zero_copy_ipc;
    Publisher<MessageT> publisher(Topic::CameraToRobot);
    Subscriber<MessageT> subscriber(Topic::CameraToRobot);

    std::vector<double> pure_latencies;      // 纯传输时间（publish到take）
    std::vector<double> copy_plus_latencies; // 拷贝+传输时间
    std::vector<double> copy_durations;      // 拷贝时间

    for (uint64_t i = 1; i <= num_samples; ++i) {
        auto loanResult = publisher.loan();
        if (loanResult.buffer().has_value()) {
            auto& sample = loanResult.buffer().value();

            sample->id = i;

            // 测量拷贝时间
            auto copy_start = std::chrono::high_resolution_clock::now();
            std::memset(sample->data, 'A', data_size);
            auto copy_end = std::chrono::high_resolution_clock::now();
            auto copy_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(copy_end - copy_start);
            copy_durations.push_back(copy_duration.count());

            // 拷贝+传输开始时间
            auto copy_plus_start = copy_start;

            // 纯传输开始时间（publish后）
            auto start = std::chrono::high_resolution_clock::now();
            sample.publish();

            if (!use_batch) {
                auto receiveResult = subscriber.take();
                if (receiveResult.has_value()) {
                    auto end = std::chrono::high_resolution_clock::now();

                    // 纯传输时间
                    auto pure_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                    pure_latencies.push_back(pure_duration.count());

                    // 拷贝+传输时间
                    auto copy_plus_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - copy_plus_start);
                    copy_plus_latencies.push_back(copy_plus_duration.count());

                    // 可选：验证id
                    auto& receivedSample = receiveResult.value();
                    if (receivedSample->id != i) {
                        std::cerr << test_name << " - Data ID mismatch at sample " << i << std::endl;
                    }
                }
            } else {
                // 批量拉取
                bool found = false;
                std::chrono::high_resolution_clock::time_point end;
                for (int retry = 0; retry < 100 && !found; ++retry) { // 最多等100ms
                    auto batch = subscriber.take_batch(batch_size);
                    for (auto& msg : batch) {
                        if (msg->id == i) {
                            end = std::chrono::high_resolution_clock::now();
                            found = true;
                            break;
                        }
                    }
                    if (!found) std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (found) {
                    // 纯传输时间
                    auto pure_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                    pure_latencies.push_back(pure_duration.count());
                    // 拷贝+传输时间
                    auto copy_plus_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - copy_plus_start);
                    copy_plus_latencies.push_back(copy_plus_duration.count());
                } else {
                    std::cerr << test_name << " - Batch take did not find sample " << i << std::endl;
                }
            }
        } else {
            std::cerr << test_name << " - Unable to loan sample at sample " << i << ". due to " << static_cast<int>(loanResult.status()) << std::endl;
        }
    }

    auto calculate_stats = [](const std::vector<double>& data) {
        double sum = std::accumulate(data.begin(), data.end(), 0.0);
        double mean = sum / data.size();
        double sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / data.size() - mean * mean);
        double min_val = *std::min_element(data.begin(), data.end());
        double max_val = *std::max_element(data.begin(), data.end());
        return std::make_tuple(mean, stdev, min_val, max_val);
    };

    auto [pure_mean, pure_stdev, pure_min, pure_max] = calculate_stats(pure_latencies);
    auto [copy_plus_mean, copy_plus_stdev, copy_plus_min, copy_plus_max] = calculate_stats(copy_plus_latencies);
    auto [copy_mean, copy_stdev, copy_min, copy_max] = calculate_stats(copy_durations);

    std::cout << "Test: " << test_name << std::endl;
    std::cout << "Data size: " << data_size << " bytes" << std::endl;
    std::cout << "Samples: " << num_samples << std::endl;

    std::cout << "纯传输时间 (publish->take):" << std::endl;
    std::cout << "  Min latency: " << pure_min << " ns (" << pure_min / num_samples << " μs)" << std::endl;
    std::cout << "  Max latency: " << pure_max << " ns (" << pure_max / num_samples << " μs)" << std::endl;
    std::cout << "  Average latency: " << pure_mean << " ns (" << pure_mean / num_samples << " μs)" << std::endl;
    std::cout << "  Standard deviation: " << pure_stdev << " ns (" << pure_stdev / num_samples << " μs)" << std::endl;

    std::cout << "拷贝+传输时间 (memset+publish->take):" << std::endl;
    std::cout << "  Min latency: " << copy_plus_min << " ns (" << copy_plus_min / num_samples << " μs)" << std::endl;
    std::cout << "  Max latency: " << copy_plus_max << " ns (" << copy_plus_max / num_samples << " μs)" << std::endl;
    std::cout << "  Average latency: " << copy_plus_mean << " ns (" << copy_plus_mean / num_samples << " μs)" << std::endl;
    std::cout << "  Standard deviation: " << copy_plus_stdev << " ns (" << copy_plus_stdev / num_samples << " μs)" << std::endl;

    std::cout << "纯拷贝时间 (memset):" << std::endl;
    std::cout << "  Min latency: " << copy_min << " ns (" << copy_min / num_samples << " μs)" << std::endl;
    std::cout << "  Max latency: " << copy_max << " ns (" << copy_max / num_samples << " μs)" << std::endl;
    std::cout << "  Average latency: " << copy_mean << " ns (" << copy_mean / num_samples << " μs)" << std::endl;
    std::cout << "  Standard deviation: " << copy_stdev << " ns (" << copy_stdev / num_samples << " μs)" << std::endl;

    std::cout << "----------------------------------------" << std::endl;
}

int main(int argc, char* argv[]) {
    // 用法: ./main [take|batch] [batch_size]
    std::string mode = "take";
    int batch_size = 8;
    if (argc >= 2) {
        mode = argv[1];
    }
    if (argc >= 3) {
        batch_size = std::stoi(argv[2]);
    }
    bool use_batch = (mode == "batch");

    // 测试1KB消息
    test_latency<zero_copy_ipc::ExampleMessage1K>("Test 1KB Message", zero_copy_ipc::ExampleMessage1K::data_size, 100, use_batch, batch_size);

    // 测试1MB消息
    test_latency<zero_copy_ipc::ExampleMessage1M>("Test 1MB Message", zero_copy_ipc::ExampleMessage1M::data_size, 100, use_batch, batch_size);

    // 测试10MB消息
    test_latency<zero_copy_ipc::ExampleMessage10M>("Test 10MB Message", zero_copy_ipc::ExampleMessage10M::data_size, 100, use_batch, batch_size);

    // 测试20MB消息
    test_latency<zero_copy_ipc::ExampleMessage20M>("Test 20MB Message", zero_copy_ipc::ExampleMessage20M::data_size, 100, use_batch, batch_size);

    return 0;
}