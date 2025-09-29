cmake_minimum_required(VERSION 3.16)
project(VEX_BuildTools)

set(CMAKE_CXX_STANDARD 20)

# VPAK Packer executable
add_executable(vpak_packer
    vpak_packer.cpp
)
