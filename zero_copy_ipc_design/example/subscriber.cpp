#include "middleware.hpp"
#include "types.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace zero_copy_ipc;
    Subscriber<ExampleMessage> sub("demo_topic");

    ExampleMessage msg;
    while (true) {
        if (sub.take(msg)) {
            std::cout << "Received: id=" << msg.id << ", data=" << msg.data << std::endl;
        } else {
            std::cout << "No message received." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    return 0;
}