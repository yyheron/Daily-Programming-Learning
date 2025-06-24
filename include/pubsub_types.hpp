#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> // Needed for ptime

namespace zero_copy_ipc {

// The information stored for each subscriber in the registry
struct SubscriberInfo {
    std::size_t head;
    boost::posix_time::ptime last_heartbeat;
};

// The allocator for the map, which uses the shared memory segment manager
using ShmAllocator = boost::interprocess::allocator<
    std::pair<const uint64_t, SubscriberInfo>,
    boost::interprocess::managed_shared_memory::segment_manager
>;

// The map itself, which stores subscriber IDs and their info
using SubscriberRegistryMap = boost::interprocess::map<
    uint64_t,
    SubscriberInfo,
    std::less<uint64_t>,
    ShmAllocator
>;

} // namespace zero_copy_ipc
