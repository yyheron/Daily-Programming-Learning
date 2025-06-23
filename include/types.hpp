#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace zero_copy_ipc {

// 示例：定义一个通用消息类型，可根据实际需求扩展
struct ExampleMessage {
    uint64_t id;
    char data[256];

    ExampleMessage() : id(0) { data[0] = '\0'; }
    ExampleMessage(uint64_t id_, const std::string& str) : id(id_) {
        std::strncpy(data, str.c_str(), sizeof(data) - 1);
        data[sizeof(data) - 1] = '\0';
    }
};

} // namespace