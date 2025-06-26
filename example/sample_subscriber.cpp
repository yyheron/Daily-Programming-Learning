#include "subscriber.hpp"
#include "types.hpp"
#include "topic_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace zero_copy_ipc;
    Subscriber<ExampleMessage> sub(Topic::CameraToRobot);

    while (true) {
        auto msg_opt = sub.take();
        if (msg_opt.has_value()) {
            auto& msg = msg_opt.value();
            std::cout << "Received: id=" << msg->id << ", data=" << msg->data << std::endl;
        } else {
            std::cout << "No message received." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    return 0;
}