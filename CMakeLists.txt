cmake_minimum_required(VERSION 3.24)
project(xfly)

set(CMAKE_CXX_STANDARD 20)
include_directories(_include)

add_subdirectory(app)
add_subdirectory(test)
