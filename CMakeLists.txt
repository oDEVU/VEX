cmake_minimum_required(VERSION 3.24)
project(VEX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(thirdparty)
add_subdirectory(src)
