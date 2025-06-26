#include "publisher.hpp"
#include "types.hpp"
#include "topic_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace zero_copy_ipc;
    Publisher<ExampleMessage> publisher(Topic::CameraToRobot);
    const char* msg = "Hello ZeroCopy"

    for (uint64_t i = 1; i <= 10; ++i) {
        
        auto loanResult = publisher.loan();
        if (loanResult.has_value())
        {
            auto& sample = loanResult.value();
            // Sample can be held until ready to publish
            sample.id = i;
            size_t len = strlen(msg);
            memcpy(sample.data, msg, len);
            sample.data[len] = '\0';
            sample.publish();
            std::cout << "Published: id=" << msg.id << ", data=" << msg.data << std::endl;
        } else {
            std::cout << "Publish failed (queue full)" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}