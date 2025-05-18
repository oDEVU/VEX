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

# glm
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(glm)


# Final interface library
add_library(thirdparty INTERFACE)
target_link_libraries(thirdparty INTERFACE
    SDL3::SDL3
    volk
    VulkanMemoryAllocator
    Vulkan::Vulkan
    assimp
    glm::glm
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
