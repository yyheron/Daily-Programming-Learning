#pragma once

#include <type_traits>
#include <boost/interprocess/managed_shared_memory.hpp>
#include "chunk_queue.hpp"
#include "pubsub_types.hpp"

namespace zero_copy_ipc {

// The one and only primary definition of needs_stl_allocator.
// By default, types do not need an allocator.
template <typename T>
struct needs_stl_allocator : std::false_type {};


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
