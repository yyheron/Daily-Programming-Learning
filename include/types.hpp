// FILEPATH: f:/Repo/Daily-Programming-Learning/include/types.hpp
#pragma once

#include <cstdint>
#include <cstring>

namespace zero_copy_ipc {

template<size_t N>
struct ExampleMessage {
    uint64_t id;
    uint64_t timestamp_ns;
    static constexpr size_t data_size = N;
    char data[data_size];

    ExampleMessage() : id(0) { std::memset(data, 0, data_size); }
};

using ExampleMessage1K = ExampleMessage<1024>;
using ExampleMessage1M = ExampleMessage<1024 * 1024>;
using ExampleMessage10M = ExampleMessage<10 * 1024 * 1024>;
using ExampleMessage20M = ExampleMessage<20 * 1024 * 1024>;

} // namespace zero_copy_ipc