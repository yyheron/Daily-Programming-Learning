#pragma once

#include <type_traits>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "chunk_queue.hpp"
#include "pubsub_types.hpp"

namespace zero_copy_ipc {

// The one and only primary definition of needs_stl_allocator.
// By default, types do not need an allocator.
template <typename T>
struct needs_stl_allocator : std::false_type {};

#define NEED_STL_ALLOCATOR(T) \
template <> \
struct needs_stl_allocator<T> : std::true_type {}

// 需要为pair特化needs_stl_allocator（当包含容器成员时）
template <typename T1, typename T2>
struct needs_stl_allocator<pair<T1, T2>> {
    static constexpr bool value = 
        needs_stl_allocator<T1>::value || 
        needs_stl_allocator<T2>::value;
};

// Trait to check if a type T has an init_members method.
template <typename T, typename Alloc, typename = void>
struct has_init_members : std::false_type {};

template <typename T, typename Alloc>
struct has_init_members<T, Alloc, decltype(void(std::declval<T&>().init_members(std::declval<const Alloc&>(), std::declval<T&>())))> : std::true_type {};

// 基类定义
template <typename Derived>
struct ShmConstructible {
    template <typename Allocator>
    explicit ShmConstructible(const Allocator& alloc) {
        initialize_members(alloc, static_cast<Derived*>(this));
    }

private:
    // SFINAE overload for types that have a custom init_members function.
    template <typename Alloc, typename T>
    auto initialize_members(const Alloc& alloc, T* self) -> std::enable_if_t<has_init_members<T, Alloc>::value> {
        self->Derived::init_members(alloc, *self);
    }

    // SFINAE overload for types that use the automatic for_each_member logic.
    template <typename Alloc, typename T>
    auto initialize_members(const Alloc& alloc, T* self) -> std::enable_if_t<!has_init_members<T, Alloc>::value> {
        // 自动初始化所有public成员
        const auto init = [&](auto& member) {
            using MemberType = typename std::decay<decltype(member)>::type;
            if (needs_stl_allocator<MemberType>::value) {
                using ReboundAlloc = typename Alloc::template rebind<typename MemberType::value_type>::other;
                member = MemberType(ReboundAlloc(alloc.get_segment_manager()));
            }
        };
        
        // 需要用户类型提供for_each_member函数
        self->for_each_member(init);
    }
};

// 支持自动列出成员的宏（需要GCC/Clang的__VA_ARGS__扩展）
// #define FOR_EACH_APPLY(f, ...) FOR_EACH_APPLY_IMPL(f, __VA_ARGS__)
// #define FOR_EACH_APPLY_IMPL(f, x, ...) (f(this->x)), 0 FOR_EACH_APPLY_TAIL(f, __VA_ARGS__)
// #define FOR_EACH_APPLY_TAIL(f, ...) , FOR_EACH_APPLY_IMPL(f, __VA_ARGS__)

// // 修改主宏定义
// #define SHM_STL_TYPE_EXPAND(TypeName, ...) \
// template <typename F> \
// void for_each_member(F&& f) { \
//     using Self = TypeName; \
//     int dummy[] = { 0 FOR_EACH_APPLY(f, __VA_ARGS__) }; \
//     (void)dummy; \
// } \
// template <typename Alloc> \
// explicit TypeName(const Alloc& alloc) : ShmConstructible<TypeName>(alloc) {} \
// TypeName() = delete
#define EXPAND(x) x
#define FOR_EACH_OP(macro, ...) \
    EXPAND(FOR_EACH_OP_IMPL(macro, __VA_ARGS__))
#define FOR_EACH_OP_IMPL(macro, x, ...) \
    macro(x) \
    __VA_OPT__(FOR_EACH_OP_AGAIN_IMPL PAREN_LEFT (macro, __VA_ARGS__))
#define FOR_EACH_OP_AGAIN_IMPL() FOR_EACH_OP_IMPL
#define PAREN_LEFT ()

// 修改主宏定义
#define SHM_STL_TYPE_EXPAND(TypeName, ...) \
template <typename F> \
void for_each_member(F&& f) { \
    using Self = TypeName; \
    int dummy[] = { \
        0 \
        FOR_EACH_OP(FOR_EACH_OP_HANDLER, __VA_ARGS__) \
    }; \
    (void)dummy; \
} \
template <typename Alloc> \
explicit TypeName(const Alloc& alloc) : ShmConstructible<TypeName>(alloc) {} \
TypeName() = delete

#define FOR_EACH_OP_HANDLER(x) , (f(this->x), 0)

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