cmake_minimum_required(VERSION 3.5)
project(BinToText)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# # 查找 Boost 库
# find_package(Boost REQUIRED)
# include_directories(${Boost_INCLUDE_DIRS})

add_executable(BinToText recorder.cpp MapFileReader.cpp BinToText.cpp)
target_link_libraries(BinToText pthread) #${Boost_LIBRARIES})

install(TARGETS BinToText DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
