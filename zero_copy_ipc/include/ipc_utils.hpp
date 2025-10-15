#pragma once

#include <type_traits>
#include <utility>

namespace zero_copy_ipc {

// The one and only primary definition of needs_stl_allocator.
// By default, types do not need an allocator.
template <typename T>
struct needs_stl_allocator : std::false_type {};

#define NEED_STL_ALLOCATOR(T) \
template <> \
struct needs_stl_allocator<T> : std::true_type {}

// // 需要为pair特化needs_stl_allocator（当包含容器成员时）
// template <typename T1, typename T2>
// struct needs_stl_allocator<std::pair<T1, T2>> {
//     static constexpr bool value = 
//         needs_stl_allocator<T1>::value || 
//         needs_stl_allocator<T2>::value;
// };

#define EXPAND(x) x

// New macros for generating the initializer list
#define INITIALIZER_LIST(...) \
    EXPAND(INITIALIZER_LIST_IMPL(__VA_ARGS__))
#define INITIALIZER_LIST_IMPL(x, ...) \
    x(alloc) \
    __VA_OPT__(, INITIALIZER_LIST_AGAIN_IMPL PAREN_LEFT (__VA_ARGS__))
#define INITIALIZER_LIST_AGAIN_IMPL() INITIALIZER_LIST_IMPL
#define PAREN_LEFT ()

// 修改主宏定义
#define SHM_STL_TYPE_EXPAND(TypeName, ...) \
template <typename Alloc> \
explicit TypeName(const Alloc& alloc) : INITIALIZER_LIST(__VA_ARGS__) {} \
TypeName() = delete

// // SFINAE工具：创建带分配器的ChunkQueue
// template <typename T, size_t N, typename Shm>
// ChunkQueue<T, N>* create_chunk_queue(Shm& shm, const std::string& name, std::true_type /* needs_allocator */) {
//     const ShmStlAllocator<T> allocator(shm.get_segment_manager());
//     return shm.find_or_construct<ChunkQueue<T, N>>(name.c_str())(allocator);
// }

// // SFINAE工具：创建不带分配器的ChunkQueue
// template <typename T, size_t N, typename Shm>
// ChunkQueue<T, N>* create_chunk_queue(Shm& shm, const std::string& name, std::false_type /* needs_allocator */) {
//     return shm.find_or_construct<ChunkQueue<T, N>>(name.c_str())();
// }

// // 寻找特定名字的ChunkQueue
// template <typename T, size_t N, typename Shm>
// ChunkQueue<T, N>* find_chunk_queue(Shm& shm, const std::string& name) {
//     return shm.find<ChunkQueue<T, N>>(name.c_str());
// }

} // namespace zero_copy_ipc