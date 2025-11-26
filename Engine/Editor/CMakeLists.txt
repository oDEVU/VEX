cmake_minimum_required(VERSION 3.16)
project(VexEditor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)
set(CMAKE_DISABLE_TESTING TRUE CACHE BOOL "Force disable all tests" FORCE)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Forcing build type to Debug to match Game Module DLL")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type forced to Debug" FORCE)
endif()

if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Limiting configurations to Debug only")
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "Configs forced to Debug" FORCE)
endif()

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(_DEBUG _ITERATOR_DEBUG_LEVEL=2)
        if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            add_link_options("-Wl,/nodefaultlib:msvcrt" "-Wl,/defaultlib:msvcrtd")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -D_DLL -D_MT -Xclang --dependent-lib=msvcrtd")
        endif()
    endif()
endif()

set(ENGINE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../")

if(NOT EXISTS "${ENGINE_PATH}")
    message(FATAL_ERROR "Could not find engine core. Expected path: ${ENGINE_PATH}")
endif()

set(VEX_BUILD_DIR "${ENGINE_PATH}/Core/bin/Debug")

set(VEX_TARGETS_FILE "${VEX_BUILD_DIR}/VEXTargets.cmake")

if(EXISTS "${VEX_TARGETS_FILE}")
    message(STATUS "Linking shared VEX Engine from: ${VEX_BUILD_DIR}")
    include("${VEX_TARGETS_FILE}")
else()
    message(FATAL_ERROR "VEX Engine targets not found at: ${VEX_TARGETS_FILE}\nPlease run the ProjectBuilder to compile the engine first.")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} "main.cpp")

set(source_files
        Editor.cpp
        Editor.hpp
        EditorCamera.hpp
        EditorCamera.cpp
        EditorImGUIWrapper.hpp
        EditorImGUIWrapper.cpp
    )

target_sources(${PROJECT_NAME}
    PRIVATE
        ${source_files}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VEX_PROJECT_TITLE=\"${PROJECT_NAME}\"
)
target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${ENGINE_PATH}/Core/src"
    "${ENGINE_PATH}/Core/src/components"
    "${ENGINE_PATH}/Core/src/components/backends/vulkan"
    "${ENGINE_PATH}/Core/src/components/GameObjects"
    "${ENGINE_PATH}/Core/src/components/GameObjects/Creators"
    "${ENGINE_PATH}/Core/src/components/UI"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-Wl,/subsystem:windows,/ENTRY:mainCRTStartup"
    )
endif()

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

set(SHADERS_SOURCE_DIR "${VEX_BUILD_DIR}/shaders")
set(SHADERS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Engine/shaders")

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADERS_SOURCE_DIR}"
        "${SHADERS_DEST_DIR}"
    COMMENT "Copying Shaders directory"
)

add_dependencies(${PROJECT_NAME} copy_shaders)
