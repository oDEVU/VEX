cmake_minimum_required(VERSION 3.16)
project(VexProject)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VexProject.json")
    message(FATAL_ERROR "Project file not found!")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VexProject.json" PROJECT_JSON)
string(JSON PROJECT_NAME ERROR_VARIABLE JSON_ERROR GET "${PROJECT_JSON}" "project_name")

if(JSON_ERROR)
    message(FATAL_ERROR "Failed to parse project file: ${JSON_ERROR}")
endif()

string(JSON ENGINE_PATH ERROR_VARIABLE JSON_ERROR GET "${PROJECT_JSON}" "engine_path")

if(NOT EXISTS "${ENGINE_PATH}")
    message(FATAL_ERROR "Project file has invalid engine path. Please check the path in the project file.")
endif()

if(NOT EXISTS "${ENGINE_PATH}/BuildTools/build/vpak_packer")
    message(FATAL_ERROR "BuildTools are not present. Make sure they are built.")
endif()

project(${PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

# So zed would no longer show false errors you need to copy or link compile_commands.json file to root folder.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME})

# Enables stacktraces for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG=0)
endif()

target_sources(${PROJECT_NAME} PRIVATE main.cpp)
add_subdirectory(${ENGINE_PATH}/Core  ${CMAKE_BINARY_DIR}/Engine)
target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
set(ASSETS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(copy_Assets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_DEST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ASSETS_SOURCE_DIR}"
            "${ASSETS_DEST_DIR}"
        COMMENT "Copying Assets directory"
    )
else()
    make_directory(${ASSETS_DEST_DIR})
    add_custom_target(copy_Assets ALL
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${ENGINE_PATH}/BuildTools/build/vpak_packer
        ${ASSETS_SOURCE_DIR}
        ${ASSETS_DEST_DIR}/assets.vpk
        COMMENT "Packing assets for release build"
    )
endif()

add_dependencies(${PROJECT_NAME} copy_Assets)
