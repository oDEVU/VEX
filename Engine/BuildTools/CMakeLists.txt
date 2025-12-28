cmake_minimum_required(VERSION 3.14)
project(VEX_BuildTools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# VPAK Packer executable
add_executable(VPAK_Packer
    vpakPacker.cpp
)

# Project Builder executable
add_executable(ProjectBuilder
    projectBuilder.cpp
)

set(BUILD_FILES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/BuildFiles")
set(BUILD_FILES_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/BuildFiles")

add_custom_target(copy_files ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_FILES_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${BUILD_FILES_SOURCE_DIR}"
        "${BUILD_FILES_DEST_DIR}"
    COMMENT "Copying BuildFiles directory"
)
