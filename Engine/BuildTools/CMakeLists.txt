cmake_minimum_required(VERSION 3.14)
project(VEX_BuildTools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Core/thirdparty/zstd/build/cmake zstd SYSTEM)

add_executable(VPAK_Packer
    VpakPacker.cpp
)

target_link_libraries(VPAK_Packer PRIVATE libzstd_static)

add_executable(ProjectBuilder
    ProjectBuilder.cpp
)

add_executable(VexCrashDecoder
    VexCrashDecoder/main.cpp
    VexCrashDecoder/CrashDecoder.cpp
    VexCrashDecoder/CrashDecoder.hpp
)

if(WIN32)
  target_link_libraries(VexCrashDecoder PRIVATE dbghelp)
endif()

target_include_directories(VexCrashDecoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/VexCrashDecoder
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
