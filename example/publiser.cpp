#include "middleware.hpp"
#include "types.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace zero_copy_ipc;
    Publisher<ExampleMessage> pub("demo_topic");

    for (uint64_t i = 1; i <= 10; ++i) {
        ExampleMessage msg(i, "Hello, zero-copy world!");
        if (pub.publish(msg)) {
            std::cout << "Published: id=" << msg.id << ", data=" << msg.data << std::endl;
        } else {
            std::cout << "Publish failed (queue full)" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}