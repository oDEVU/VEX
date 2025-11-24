cmake_minimum_required(VERSION 3.16)
project(VexProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)
set(CMAKE_DISABLE_TESTING TRUE CACHE BOOL "Force disable all tests" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VexProject.json")
    message(FATAL_ERROR "Project file not found!")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VexProject.json" PROJECT_JSON)
string(JSON PROJECT_NAME ERROR_VARIABLE JSON_ERROR GET "${PROJECT_JSON}" "project_name")

if(JSON_ERROR)
    message(FATAL_ERROR "Failed to parse project file: ${JSON_ERROR}")
endif()

string(JSON ENGINE_PATH ERROR_VARIABLE JSON_ERROR GET "${PROJECT_JSON}" "engine_path")

set(ENGINE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../${ENGINE_PATH})

if(NOT EXISTS "${ENGINE_PATH}")
    message(FATAL_ERROR "Project file has invalid engine path. Please check the path in the project file. Loaded path: ${ENGINE_PATH}")
endif()

if(NOT EXISTS "${ENGINE_PATH}/BuildTools/build/VPAK_Packer${CMAKE_EXECUTABLE_SUFFIX}")
    message(FATAL_ERROR "BuildTools are not present. Make sure VPAK_Packer is built.")
endif()

project(${PROJECT_NAME})

#add_subdirectory(${ENGINE_PATH}/Core ${CMAKE_BINARY_DIR}/Engine)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VEX_BUILD_DIR "${ENGINE_PATH}/Core/bin/Debug")
else()
    set(VEX_BUILD_DIR "${ENGINE_PATH}/Core/bin/Release")
endif()

set(VEX_TARGETS_FILE "${VEX_BUILD_DIR}/VEXTargets.cmake")

if(EXISTS "${VEX_TARGETS_FILE}")
    message(STATUS "Linking shared VEX Engine from: ${VEX_BUILD_DIR}")
    include("${VEX_TARGETS_FILE}")
else()
    message(FATAL_ERROR "VEX Engine targets not found at: ${VEX_TARGETS_FILE}\nPlease run the ProjectBuilder to compile the engine first.")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

# So zed would no longer show false errors you need to copy or link compile_commands.json file to root folder.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB GAME_SRC CONFIGURE_DEPENDS "Source/*.h" "Source/*.cpp")

add_library(GameModule SHARED ${GAME_SRC})
target_link_libraries(GameModule PUBLIC VEX::VEX)
target_compile_definitions(GameModule PRIVATE GAME_MODULE_EXPORTS)

add_executable(${PROJECT_NAME} "main.cpp")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VEX_PROJECT_TITLE=\"${PROJECT_NAME}\"
)
target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-Wl,/subsystem:windows,/ENTRY:mainCRTStartup"
    )
endif()

# Enables stacktraces for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG=1)
    if(WIN32)
        set(VULKAN_VALIDATION_DLL "${Vulkan_LIBRARY_DIR}/../Bin/VkLayer_khronos_validation.dll")
        if(EXISTS "${VULKAN_VALIDATION_DLL}")
            add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${VULKAN_VALIDATION_DLL}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying Vulkan validation layer DLL to Debug build directory"
            CONFIGURATIONS Debug
        )
        else()
            message(WARNING "Vulkan validation layer DLL not found at ${VULKAN_VALIDATION_DLL}")
        endif()
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG=0)
endif()

#target_sources(${PROJECT_NAME} PRIVATE main.cpp)
#add_subdirectory(${ENGINE_PATH}/Core  ${CMAKE_BINARY_DIR}/Engine)
#target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

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
        COMMAND ${ENGINE_PATH}/BuildTools/build/VPAK_Packer${CMAKE_EXECUTABLE_SUFFIX}
        ${ASSETS_SOURCE_DIR}
        ${ASSETS_DEST_DIR}/assets.vpk
        COMMENT "Packing assets for release build"
    )
endif()

add_dependencies(${PROJECT_NAME} copy_Assets)
