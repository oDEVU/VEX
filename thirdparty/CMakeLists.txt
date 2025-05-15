# SDL3 with Wayland support
set(SDL_TEST OFF CACHE BOOL "Disable SDL tests")
set(SDL_STATIC ON CACHE BOOL "Build shared library")
set(SDL_VIDEO ON CACHE BOOL "Enable video subsystem")
set(SDL_VIDEO_WAYLAND ON CACHE BOOL "Enable Wayland")
set(SDL_VIDEO_X11 ON CACHE BOOL "Enable X11")
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan")
add_subdirectory(SDL)

# After SDL is configured
#message(STATUS "SDL Video Drivers: ${SDL_VIDEO_DRIVERS}")
#file(READ "${SDL_BINARY_DIR}/SDL_config.h" SDL_CONFIG_H)
#string(FIND "${SDL_CONFIG_H}" "#define SDL_VIDEO_DRIVER_WAYLAND 1" HAVE_WAYLAND)
#if(HAVE_WAYLAND EQUAL -1)
#    message(FATAL_ERROR "SDL was not built with Wayland support!")
#endif()

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

# Final interface library
add_library(thirdparty INTERFACE)
target_link_libraries(thirdparty INTERFACE
    SDL3::SDL3
    volk
    VulkanMemoryAllocator
    Vulkan::Vulkan
)

# Linux-specific linking
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(thirdparty INTERFACE
        ${WAYLAND_LIBRARIES}
        ${X11_LIBRARIES}
    )
    target_include_directories(thirdparty INTERFACE
        ${WAYLAND_INCLUDE_DIRS}
        ${X11_INCLUDE_DIRS}
    )
endif()
