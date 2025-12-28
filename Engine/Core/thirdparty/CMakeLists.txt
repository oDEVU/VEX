set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)
if(WIN32)
    string(REPLACE "-Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS" CACHE STRING "Modified CXX flags for thirdparty on Windows" FORCE)
endif()

if(WIN32 AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS ">> FORCING DEBUG RUNTIME (msvcrtd) FOR ALL THIRDPARTY LIBS")
        add_compile_definitions(_DEBUG _ITERATOR_DEBUG_LEVEL=2)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -D_MT -D_DLL -Xclang --dependent-lib=msvcrtd" CACHE STRING "Force Debug Runtime" FORCE)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -D_MT -D_DLL -Xclang --dependent-lib=msvcrtd" CACHE STRING "Force Debug Runtime" FORCE)
        add_link_options("-Wl,/nodefaultlib:msvcrt" "-Wl,/defaultlib:msvcrtd")
    endif()
endif()

# i hate google tests
include(FetchContent)

set(GTEST_DUMMY_DIR "${CMAKE_CURRENT_BINARY_DIR}/googletest-dummy")
file(MAKE_DIRECTORY ${GTEST_DUMMY_DIR})

file(WRITE "${GTEST_DUMMY_DIR}/CMakeLists.txt"
    "add_library(gtest INTERFACE)\n"
    "add_library(gtest_main INTERFACE)\n"
    "add_library(gmock INTERFACE)\n"
    "add_library(gmock_main INTERFACE)\n"
    "add_library(GTest::gtest ALIAS gtest)\n"
    "add_library(GTest::gtest_main ALIAS gtest_main)\n"
    "add_library(GTest::gmock ALIAS gmock)\n"
    "add_library(GTest::gmock_main ALIAS gmock_main)\n"
)

FetchContent_Declare(
    googletest
    SOURCE_DIR ${GTEST_DUMMY_DIR}
)
FetchContent_MakeAvailable(googletest)

# SDL3 with Wayland support
set(SDL_TEST OFF CACHE BOOL "Disable SDL tests")
set(SDL_STATIC ON CACHE BOOL "Build shared library")
set(SDL_VIDEO ON CACHE BOOL "Enable video subsystem")
set(SDL_VIDEO_WAYLAND ON CACHE BOOL "Enable Wayland")
set(SDL_VIDEO_X11 ON CACHE BOOL "Enable X11")
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan")
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_subdirectory(SDL SYSTEM)

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
add_subdirectory(volk SYSTEM)

# cr
add_subdirectory(cr SYSTEM)

#yoga
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-array-bounds)
endif()
set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)
set(YOGA_ENABLE_TESTS OFF CACHE BOOL "Disable Yoga tests" FORCE)
set(YOGA_ENABLE_BENCHMARKS OFF CACHE BOOL "Disable Yoga benchmarks" FORCE)
set(YOGA_SKIP_TESTS TRUE CACHE INTERNAL "Skip Yoga tests" FORCE)
set(YOGA_SKIP_BENCHMARKS TRUE CACHE INTERNAL "Skip Yoga benchmarks" FORCE)
set(YOGA_BUILD_TESTS OFF CACHE BOOL "Disable Yoga test builds" FORCE)
add_subdirectory(yoga SYSTEM)

foreach(BAD_TARGET yogatest yogatests tests yogabench yogabenchmarks)
    if(TARGET ${BAD_TARGET})
        message(STATUS "Target '${BAD_TARGET}' detected. Forcefully excluding from build.")
        set_target_properties(${BAD_TARGET} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endforeach()

# Force disable yoga test targets if they were created
if(TARGET yogatest)
    set_target_properties(yogatest PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()
if(TARGET yogabench)
    set_target_properties(yogabench PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Check what Yoga target is available
if(TARGET yoga)
    message(STATUS "Yoga target found: yoga")
    set(YOGA_TARGET yoga)
elseif(TARGET Yoga)
    message(STATUS "Yoga target found: Yoga")
    set(YOGA_TARGET Yoga)
elseif(TARGET yogacore)
    message(STATUS "Yoga target found: yogacore")
    set(YOGA_TARGET yogacore)
else()
    message(FATAL_ERROR "Could not find Yoga target")
endif()

# VMA
set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON)
find_package(Vulkan REQUIRED)
add_subdirectory(VulkanMemoryAllocator SYSTEM)

# assimp
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_TEST_IMPORT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "Force Assimp to use internal zlib" FORCE)
set(BUILD_GTEST OFF CACHE BOOL "Disable GoogleTest build" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Disable GoogleMock build" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest install" FORCE)
add_subdirectory(assimp SYSTEM)

# Force disable Assimp's GoogleTest targets if they were created
foreach(BAD_TARGET gtest gtest_main unit tests)
    if(TARGET ${BAD_TARGET})
        set_target_properties(${BAD_TARGET} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endif()
endforeach()

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-Wno-character-conversion" COMPILER_HAS_CHAR_CONV_WARNING)

#if(COMPILER_HAS_CHAR_CONV_WARNING)
#    if(TARGET gtest)
#        target_compile_options(gtest PRIVATE -Wno-character-conversion)
#    endif()
#    if(TARGET gtest_main)
#        target_compile_options(gtest_main PRIVATE -Wno-character-conversion)
#    endif()
#endif()

#entt
add_subdirectory(entt SYSTEM)

target_compile_definitions(EnTT INTERFACE
        ENTT_NOEXCEPTION
    )

#jolt
# set(JPH_SHARED_LIBRARY ON "")
set(DOUBLE_PRECISION OFF)
set(INTERPROCEDURAL_OPTIMIZATION OFF)
set(JPH_NO_X11_MACROS ON)
set(CPP_RTTI_ENABLED ON CACHE BOOL "Enable RTTI" FORCE)
add_compile_definitions(JPH_NO_X11_MACROS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(GENERATE_DEBUG_SYMBOLS ON)
    if(WIN32)
        set(JPH_ENABLE_ASSERTS OFF)
    else()
        set(JPH_ENABLE_ASSERTS ON)
    endif()
    set(JPH_DEBUG_RENDERER ON)
endif()

add_subdirectory(JoltPhysics/Build SYSTEM)

#nlohmann::json
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json SYSTEM)

# dirty fix for assimp not compiling in realese
# Note: Modified to use COMPILER_HAS_CHAR_CONV_WARNING to prevent Clang 20 crash
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(WIN32)
        target_compile_options(assimp PRIVATE -Wno-array-bounds)
        if(COMPILER_HAS_CHAR_CONV_WARNING)
            target_compile_options(assimp PRIVATE -Wno-character-conversion)
        endif()
    else()
        target_compile_options(assimp PRIVATE -Wno-array-bounds)
    endif()
else()
    if(WIN32)
        target_compile_options(assimp PRIVATE -Wno-error)
        if(COMPILER_HAS_CHAR_CONV_WARNING)
            target_compile_options(assimp PRIVATE -Wno-character-conversion)
        endif()
    else()
        target_compile_options(assimp PRIVATE -Wno-error)
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF CACHE BOOL "Disable C++ module scanning" FORCE)

    add_definitions(-DCPPTRACE_STATIC_DEFINE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_STATIC_DEFINE ON CACHE BOOL "" FORCE)
    set(CPPTRACE_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_COLOR_OUTPUT ON CACHE BOOL "" FORCE)
    add_subdirectory(cpptrace) # Add module flags for cpptrace-lib

    # imgui
    set(IMGUI_PATH  "imgui")
    file(GLOB IMGUI_SOURCES
        ${IMGUI_PATH}/*.cpp
        ${IMGUI_PATH}/misc/cpp/*.cpp
        ${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
    )
    add_library("ImGui" STATIC ${IMGUI_SOURCES})
    target_include_directories("ImGui" PUBLIC
        ${IMGUI_PATH}
        ${IMGUI_PATH}/misc/cpp/
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include
    )

    target_link_libraries("ImGui" PRIVATE SDL3-static volk)
    target_compile_definitions(ImGui PRIVATE
        IMGUI_IMPL_VULKAN_USE_VOLK
    )

    if(WIN32)
        target_compile_definitions(ImGui PRIVATE "IMGUI_API=__declspec(dllexport)")
    endif()

    #imguizmo
    set(IMGUIZMO_PATH  "ImGuizmo")
    file(GLOB IMGUIZMO_SOURCES
        ${IMGUIZMO_PATH}/*.cpp
    )

    add_library("ImGuizmo" STATIC ${IMGUIZMO_SOURCES})
    target_include_directories("ImGuizmo" PUBLIC
        ${IMGUIZMO_PATH}
    )

    target_link_libraries("ImGuizmo" PRIVATE ImGui)

    if(WIN32)
        target_compile_definitions(ImGuizmo PRIVATE "IMGUI_API=__declspec(dllexport)")
    endif()

    #imreflect
    set(IMREFLECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ImReflect")

    add_library(ImReflect INTERFACE)
    target_include_directories(ImReflect INTERFACE ${IMREFLECT_PATH})
    target_link_libraries(ImGui INTERFACE ImReflect)
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
    cr
    VulkanMemoryAllocator
    assimp
    thirdparty_glm
    EnTT::EnTT
    nlohmann_json
    Jolt
    ${YOGA_TARGET}
)

target_include_directories(thirdparty_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/yoga)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    target_link_libraries(thirdparty_engine INTERFACE
        cpptrace::cpptrace
        ImGui
        ImGuizmo
        ImReflect
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(thirdparty_engine INTERFACE linux_deps)
endif()

# Prevent exporting this internal target
set_target_properties(thirdparty_engine PROPERTIES EXPORT_PROPERTIES "EXCLUDE_FROM_ALL")
