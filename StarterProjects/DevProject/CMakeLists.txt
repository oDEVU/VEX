cmake_minimum_required(VERSION 3.16)
project(DevProject)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

# So zed would no longer show false errors you need to copy or link compile_commands.json file to root folder.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE main.cpp)
add_subdirectory(../../Engine  ${CMAKE_BINARY_DIR}/Engine)
target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
set(ASSETS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets")

add_custom_target(copy_Assets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}"
        "${ASSETS_DEST_DIR}"
    COMMENT "Copying Assets directory"
)

add_dependencies(${PROJECT_NAME} copy_Assets)
