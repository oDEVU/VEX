# SDL3 with Wayland support
set(SDL_TEST OFF CACHE BOOL "Disable SDL tests")
set(SDL_STATIC ON CACHE BOOL "Build shared library")
set(SDL_VIDEO ON CACHE BOOL "Enable video subsystem")
set(SDL_VIDEO_WAYLAND ON CACHE BOOL "Enable Wayland")
set(SDL_VIDEO_X11 ON CACHE BOOL "Enable X11")
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan")
add_subdirectory(SDL)

# volk with multi-platform support
if(WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor xkbcommon)
    pkg_check_modules(X11 REQUIRED x11 xcb)

    set(VOLK_STATIC_DEFINES
        VK_USE_PLATFORM_WAYLAND_KHR
        VK_USE_PLATFORM_XLIB_KHR
        VK_USE_PLATFORM_XCB_KHR
    )

    add_compile_definitions(
        VK_USE_PLATFORM_WAYLAND_KHR
        VK_USE_PLATFORM_XLIB_KHR
        VK_USE_PLATFORM_XCB_KHR
    )
endif()
add_subdirectory(volk)

# VMA
set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON)
find_package(Vulkan REQUIRED)
add_subdirectory(VulkanMemoryAllocator)

# assimp
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp)

# dirty fix for assimp not compiling in realese
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(assimp PRIVATE -Wno-array-bounds)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # cpptrace
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_STATIC_DEFINE ON CACHE BOOL "" FORCE)
    set(CPPTRACE_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_COLOR_OUTPUT ON CACHE BOOL "" FORCE)
    add_subdirectory(cpptrace)

    # imgui
    set(IMGUI_PATH  "imgui")
    file(GLOB IMGUI_SOURCES
        ${IMGUI_PATH}/*.cpp
        ${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
    )
    add_library("ImGui" STATIC ${IMGUI_SOURCES})
    target_include_directories("ImGui" PUBLIC
        ${IMGUI_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include
    )

    target_link_libraries("ImGui" PRIVATE SDL3-static volk)
    target_compile_definitions(ImGui PRIVATE
        IMGUI_IMPL_VULKAN_USE_VOLK
    )
endif()

# glm
add_library(thirdparty_glm INTERFACE)
target_include_directories(thirdparty_glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

# Linux-specific dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(linux_deps INTERFACE)
    target_include_directories(linux_deps INTERFACE
        ${WAYLAND_INCLUDE_DIRS}
        ${X11_INCLUDE_DIRS}
    )
    target_link_libraries(linux_deps INTERFACE
        ${WAYLAND_LIBRARIES}
        ${X11_LIBRARIES}
    )
endif()

add_library(thirdparty_engine INTERFACE)

target_link_libraries(thirdparty_engine INTERFACE
    SDL3-static
    volk
    VulkanMemoryAllocator
    assimp
    thirdparty_glm
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(thirdparty_engine INTERFACE
        cpptrace::cpptrace
        ImGui
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(thirdparty_engine INTERFACE linux_deps)
endif()

# Prevent exporting this internal target
set_target_properties(thirdparty_engine PROPERTIES EXPORT_PROPERTIES "EXCLUDE_FROM_ALL")
