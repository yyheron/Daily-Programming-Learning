cmake_minimum_required(VERSION 3.5)

project(tool)

set(CMAKE_CXX_STANDARD 17)

add_compile_options("-Werror=return-type")

add_subdirectory(recorder)