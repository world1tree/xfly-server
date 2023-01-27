cmake_minimum_required(VERSION 3.24)
project(xfly)

set(CMAKE_CXX_STANDARD 20)
include_directories(_include log)

add_subdirectory(log)
add_subdirectory(proc)
add_subdirectory(net)
add_subdirectory(signal)
add_subdirectory(app)
#add_subdirectory(test)
