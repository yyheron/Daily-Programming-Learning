#include "publisher.hpp"
#include "types.hpp"
#include "topic_types.hpp"
#include "error_types.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/chrono.hpp>

int main() {
    using namespace zero_copy_ipc;
    Publisher<ExampleMessage> publisher(Topic::CameraToRobot);
    const char* msg = "Hello ZeroCopy";

    for (uint64_t i = 1; i <= 1000; ++i) {
        
        auto loanResult = publisher.loan();
        if (loanResult.buffer().has_value())
        {
            auto& sample = loanResult.buffer().value();
            
            // Sample can be held until ready to publish
            sample->id = i;
            size_t len = strlen(msg);
            memcpy(sample->data, msg, len);
            auto now = boost::chrono::high_resolution_clock::now();
            sample->timestamp_ms = boost::chrono::duration_cast<boost::chrono::milliseconds>(now.time_since_epoch()).count();
            sample->timestamp_ns = boost::chrono::duration_cast<boost::chrono::nanoseconds>(now.time_since_epoch()).count();
            sample->data[len] = '\0';
            sample.publish();
            // std::cout << "Published: time (ms): " << sample->timestamp_ms
            //           << ", time (ns): " << sample->timestamp_ns
            //           << ", id=" << sample->id << ", data=" << sample->data << std::endl;
        } else if (loanResult.status() == IpcErrorType::LoanBufferFull){
            std::cout << "Publish failed (queue full)" << std::endl;
        } else if (loanResult.status() == IpcErrorType::LoanNoSubscriber){
            std::cout << "Publish failed (No Subscriber)" << std::endl;
        } else {
            std::cout << "Publish failed (Unknown Error)" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}