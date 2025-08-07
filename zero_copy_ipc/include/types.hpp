// FILEPATH: f:/Repo/Daily-Programming-Learning/include/types.hpp
#pragma once

#include <cstdint>
#include <cstring>
#include "ipc_utils.hpp"

#include "pubsub_types.hpp"


namespace zero_copy_ipc {

template<size_t N>
struct ExampleMessage {
    uint64_t id;
    uint64_t timestamp_ns;
    static constexpr size_t data_size = N;
    char data[data_size];

    ExampleMessage() : id(0) { std::memset(data, 0, data_size); }
};
template<size_t N>
struct needs_stl_allocator<ExampleMessage<N>> {
    static constexpr bool value = false;  // 显式指定为false
};
using ExampleMessage1K = ExampleMessage<1024>;
using ExampleMessage1M = ExampleMessage<1024 * 1024>;
using ExampleMessage10M = ExampleMessage<10 * 1024 * 1024>;
using ExampleMessage20M = ExampleMessage<20 * 1024 * 1024>;

// 以下给出共享内存中支持或不支持的类型
// 以struct Foo为例
// 当你的数据类型中包括了STL容器时，必须进行显式地指定allocator，并特化need_stl_allocator以便在共享内存中使用
struct Foo {
    vector<int> data;

    template <typename Allocator>
    explicit Foo(const Allocator& alloc) : data(alloc) {}
    Foo() = delete;
};

// 针对 Foo 特化
template<>
struct needs_stl_allocator<Foo> : std::true_type {};

// 支持的数据结构原则：
// 一个对象要能在共享内存中安全存在，它必须是“自包含的”（Self-Contained）。
// 这意味着它的所有数据和状态都必须直接存在于共享内存块中，不能以任何形式依赖于单个进程的特定内存地址或操作系统资源。
// 遵循这个法则的类型通常具有以下特征：
// 只包含可平凡复制（TriviallyCopyable）的成员：如 int, float, char[], 或者不含指针的简单 struct。
// 包含“共享内存安全”的容器：其成员变量是在 pubsub_types.hpp 中定义的 zero_copy_ipc::vector, zero_copy_ipc::string 等。
// 如果类本身需要动态内存（比如内部有 vector），那么它必须提供接收分配器的构造函数，并特化 needs_stl_allocator。

// 不支持共享内存构造的类型（危险区）
// 以下类型的对象绝对不能直接在共享内存中构造，因为它们违反了“自包含”原则，会导致未定义行为、数据损坏或程序崩溃。
// 1. 任何包含裸指针或引用的类
// 指针和引用存储的是虚拟内存地址。进程A中的地址 0x7ffc... 在进程B中是无效的或指向完全不同的东西。
// 2. 任何使用标准库默认分配器的容器的类
// std::vector, std::string, std::map 等标准容器默认从进程的私有堆上分配内存
// 3. 任何包含虚函数的类（即多态类）
// 虚函数表指针（v-pointer）指向该进程内存空间中的一个静态区域（虚函数表 v-table）。这个指针在另一个进程中是无效的。
// 4. 任何包含包含进程特定句柄或资源的类 （如文件描述符、socket 或文件句柄）
// 文件描述符是进程本地的，进程B没有访问权限。
// 5. 包含 static 成员变量的类（需要特别注意）
// 成员变量不存储在类的任何实例中，而是作为全局变量存在于每个进程的独立内存空间里。
// 6. 例如cv::Mat等库中定义的类，内部的 data 指针指向的是进程私有的堆内存。
// 这块内存由标准库的 malloc 或 new 分配，其地址只在创建它的进程中有意义。


} // namespace zero_copy_ipc
