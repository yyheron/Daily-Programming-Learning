#pragma once

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

namespace zero_copy_ipc {

// 进程间同步原语封装（可直接用boost的类型，也可做别名）
using InterprocessMutex = boost::interprocess::interprocess_mutex;
using InterprocessCondition = boost::interprocess::interprocess_condition;

} // namespace zero_copy_ipc