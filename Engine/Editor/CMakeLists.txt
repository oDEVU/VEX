cmake_minimum_required(VERSION 3.16)
project(VexEditor)

set(CMAKE_CXX_STANDARD 23)
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

set(ICON_SOURCE_PNG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/app_icon.png")

if(NOT EXISTS "${ICON_SOURCE_PNG}")
    message(FATAL_ERROR "Missing icon at '${ICON_SOURCE_PNG}'. Please add the file or update the path.")
endif()

if(WIN32)
    set(ICON_OUT_ICO "${CMAKE_CURRENT_BINARY_DIR}/icon.ico")
    set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")

    find_program(MAGICK_EXE magick)

    if(NOT MAGICK_EXE AND NOT EXISTS "${ICON_OUT_ICO}")
        message(FATAL_ERROR "ImageMagick not found and no 'icon.ico' exists. Install ImageMagick to auto-convert PNGs.")
    endif()

    if(MAGICK_EXE)
        add_custom_command(
            OUTPUT ${ICON_OUT_ICO}
            COMMAND ${MAGICK_EXE} convert "${ICON_SOURCE_PNG}" -define icon:auto-resize=256,128,64,48,32,16 "${ICON_OUT_ICO}"
            DEPENDS ${ICON_SOURCE_PNG}
            COMMENT "Generating Windows ICO..."
        )
    endif()

    file(WRITE "${RC_FILE}" "IDI_ICON1 ICON DISCARDABLE \"${ICON_OUT_ICO}\"")
    set(WINDOWS_ICON_RESOURCE ${RC_FILE})
endif()

if(UNIX AND NOT APPLE)
    set(DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/VexEngine.desktop")
    file(WRITE "${DESKTOP_FILE}"
"[Desktop Entry]
Version=1.0
Type=Application
Name=Vex Engine
Comment=Vex Project Selector
Exec=${CMAKE_CURRENT_BINARY_DIR}/VexProjectSelector
Icon=${ICON_SOURCE_PNG}
Terminal=false
Categories=Development;IDE;
")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} "main.cpp" ${WINDOWS_ICON_RESOURCE})

set(source_files
        Editor.cpp
        Editor.hpp
        EditorCamera.hpp
        EditorCamera.cpp
        EditorImGUIWrapper.hpp
        EditorImGUIWrapper.cpp
        DialogWindow.cpp
        DialogWindow.hpp
        Tools/EditorMenuBar.cpp
        Tools/EditorMenuBar.hpp
        Tools/AssetBrowser.cpp
        Tools/AssetBrowser.hpp
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
        "-Wl,/subsystem:console,/ENTRY:mainCRTStartup"
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

add_executable(VexProjectSelector "main_selector.cpp" ${WINDOWS_ICON_RESOURCE})

set(selector_sources
    ProjectSelector/ProjectSelector.cpp
    ProjectSelector/ProjectSelector.hpp
)

target_sources(VexProjectSelector PRIVATE ${selector_sources})

target_link_libraries(VexProjectSelector PRIVATE VEX::VEX)

target_include_directories(VexProjectSelector PRIVATE
    "${ENGINE_PATH}/Core/src"
    "${ENGINE_PATH}/Core/src/components"
    "${ENGINE_PATH}/Core/src/components/backends/vulkan"
    "${ENGINE_PATH}/Core/src/components/UI"
)

if(WIN32)
    target_link_libraries(VexProjectSelector PRIVATE "-Wl,/subsystem:console")
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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:VEX::VEX>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying VEX Shared Library to Editor Binary Directory"
)
