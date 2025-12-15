cmake_minimum_required(VERSION 3.20)

project(VEX)
set(NAMESPACE_NAME "VEX")

option(BUILD_SHARED_LIBS "Build the shared library" ON)
set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32 AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(_DEBUG _ITERATOR_DEBUG_LEVEL=2)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -D_MT -D_DLL -Xclang --dependent-lib=msvcrtd")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -D_MT -D_DLL -Xclang --dependent-lib=msvcrtd")
        add_link_options("-Wl,/nodefaultlib:msvcrt" "-Wl,/defaultlib:msvcrtd")
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(CPP_LIBRARY_VERSION_MAJOR 0)
set(CPP_LIBRARY_VERSION_MINOR 1)
set(CPP_LIBRARY_VERSION_PATCH 0)
set(CPP_LIBRARY_VERSION_STRING ${CPP_LIBRARY_VERSION_MAJOR}.${CPP_LIBRARY_VERSION_MINOR}.${CPP_LIBRARY_VERSION_PATCH})

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(${PROJECT_NAME} SHARED)
add_library(${NAMESPACE_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

message(STATUS "VEX build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(VEX PUBLIC DEBUG=1)
else()
    target_compile_definitions(VEX PUBLIC DEBUG=0)
endif()

get_target_property(defs VEX COMPILE_DEFINITIONS)
message(STATUS "Compile definitions for VEX: ${defs}")

add_subdirectory(thirdparty)

generate_export_header(${PROJECT_NAME}
    EXPORT_FILE_NAME ${PROJECT_NAME}/${PROJECT_NAME}_export.h
)
set(public_header_files
    include/Engine.hpp
    include/components/Mesh.hpp
    include/components/ResolutionManager.hpp
    include/components/Scene.hpp
    include/components/SceneManager.hpp
    include/components/errorUtils.hpp
    include/components/pathUtils.hpp
    include/components/ImGUIWrapper.hpp
    include/components/InputSystem.hpp
    include/components/PhysicsSystem.hpp
    include/components/JoltSafe.hpp
    include/components/types.hpp
    include/components/UI/VexUI.hpp
    include/components/GameComponents/BasicComponents.hpp
    include/components/GameComponents/UiComponent.hpp
    include/components/GameComponents/ComponentFactory.hpp
    include/components/GameComponents/InputComponent.hpp
    include/components/GameObjects/GameObject.hpp
    include/components/GameObjects/ModelObject.hpp
    include/components/GameObjects/CameraObject.hpp
    include/components/GameObjects/Creators/ModelCreator.hpp
    include/components/GameObjects/GameObjectFactory.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}_export.h
)

#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    list(APPEND public_header_files include/components/JoltDebugRenderer.hpp)
#endif()

set(source_files
        src/Engine.cpp
        src/components/Mesh.cpp
        src/components/ResolutionManager.cpp
        src/components/Scene.cpp
        src/components/SceneManager.cpp
        src/components/errorUtils.cpp
        src/components/pathUtils.cpp
        src/components/Window.cpp
        src/components/Window.hpp
        src/components/VirtualFileSystem.cpp
        src/components/VirtualFileSystem.hpp
        src/components/InputSystem.cpp
        src/components/PhysicsSystem.cpp
        src/components/UI/VexUI.cpp
        src/components/backends/vulkan/context.hpp
        src/components/backends/vulkan/Interface.cpp
        src/components/backends/vulkan/Interface.hpp
        src/components/backends/vulkan/Pipeline.cpp
        src/components/backends/vulkan/Pipeline.hpp
        src/components/backends/vulkan/Resources.cpp
        src/components/backends/vulkan/Resources.hpp
        src/components/backends/vulkan/SwapchainManager.cpp
        src/components/backends/vulkan/SwapchainManager.hpp
        src/components/backends/vulkan/uniforms.hpp
        src/components/backends/vulkan/VulkanMesh.cpp
        src/components/backends/vulkan/VulkanMesh.hpp
        src/components/backends/vulkan/VulkanImGUIWrapper.cpp
        src/components/backends/vulkan/VulkanImGUIWrapper.hpp
        src/components/backends/vulkan/Renderer.cpp
        src/components/backends/vulkan/Renderer.hpp
        src/components/backends/vulkan/MeshManager.cpp
        src/components/backends/vulkan/MeshManager.hpp
        src/components/backends/vulkan/PhysicsDebug.hpp
        src/components/GameObjects/Creators/ModelCreator.cpp
        src/components/GameObjects/GameObject.cpp
    )

#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    list(APPEND source_files src/components/JoltDebugRenderer.cpp)
#endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    LANGUAGES CXX
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${CPP_LIBRARY_VERSION_STRING}
    SOVERSION ${CPP_LIBRARY_VERSION_MAJOR}
    PUBLIC_HEADER "${public_header_files}"
    CXX_VISIBILITY_PRESET default  # fix it with exports later
    VISIBILITY_INLINES_HIDDEN 0
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${public_header_files}
        ${source_files}
)

#==============================================================================
# COMPILE SHADERS
#==============================================================================
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# Find slangc (prefer Vulkan SDK, fallback to PATH or env var)
find_program(SLANGC slangc HINTS $ENV{VULKAN_SDK}/Bin $ENV{SLANGC} ENV PATH)
if(NOT SLANGC)
    message(FATAL_ERROR "slangc not found, make sure you have installed it.")
endif()

file(GLOB GLSL_SHADERS
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
  ${SHADER_SOURCE_DIR}/*.tesc
  ${SHADER_SOURCE_DIR}/*.tese
  ${SHADER_SOURCE_DIR}/*.mesh
  ${SHADER_SOURCE_DIR}/*.task
  ${SHADER_SOURCE_DIR}/*.rgen
  ${SHADER_SOURCE_DIR}/*.rchit
  ${SHADER_SOURCE_DIR}/*.rmiss)

file(GLOB SLANG_SHADERS
    ${SHADER_SOURCE_DIR}/*.slang
)

file(GLOB SLANG_DEPENDENCIES
    ${SHADER_SOURCE_DIR}/depends/*.slang
    ${SHADER_SOURCE_DIR}/effects/*.slang
)

set(SPV_SHADERS "")

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS GLSL_SHADERS)
    get_filename_component(FILENAME ${source} NAME)
    add_custom_command(
    COMMAND
      ${glslc_executable}
      -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
    list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

foreach(source IN LISTS SLANG_SHADERS)
    get_filename_component(FILENAME ${source} NAME_WE)

    set(SLANG_FLAGS "-I${SHADER_SOURCE_DIR}")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND SLANG_FLAGS "-O3")
        #list(APPEND SLANG_FLAGS "-O2") Rewert to this if O3 is too aggresive C:
    endif()

    # Case 1: ends with Vert → only vertex shader
    if(FILENAME MATCHES ".*Vert$")
        set(OUTPUT_FILE ${SHADER_BINARY_DIR}/${FILENAME}.spv)
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${SLANGC} ${SLANG_FLAGS} -target spirv -profile vs_6_0 -o ${OUTPUT_FILE} ${source}
            DEPENDS ${source} ${SLANG_DEPENDENCIES}
            COMMENT "Compiling vertex shader ${FILENAME}"
        )
        list(APPEND SPV_SHADERS ${OUTPUT_FILE})

        # Case 2: ends with Frag → only fragment shader
    elseif(FILENAME MATCHES ".*Frag$")
        set(OUTPUT_FILE ${SHADER_BINARY_DIR}/${FILENAME}.spv)
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${SLANGC} ${SLANG_FLAGS} -target spirv -profile ps_6_0 -o ${OUTPUT_FILE} ${source}
            DEPENDS ${source} ${SLANG_DEPENDENCIES}
            COMMENT "Compiling fragment shader ${FILENAME}"
        )
        list(APPEND SPV_SHADERS ${OUTPUT_FILE})

        # Case 3: unified shader → compile both vertMain + fragMain
    else()
        set(VERT_OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}Vert.spv)
        set(FRAG_OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}Frag.spv)

        add_custom_command(
            OUTPUT ${VERT_OUTPUT}
            COMMAND ${SLANGC} ${SLANG_FLAGS} -target spirv -profile vs_6_0 -entry vertMain -o ${VERT_OUTPUT} ${source}
            DEPENDS ${source} ${SLANG_DEPENDENCIES}
            COMMENT "Compiling combined shader vertex entry 'vertMain' from ${FILENAME}.slang"
        )

        add_custom_command(
            OUTPUT ${FRAG_OUTPUT}
            COMMAND ${SLANGC} ${SLANG_FLAGS} -target spirv -profile ps_6_0 -entry fragMain -o ${FRAG_OUTPUT} ${source}
            DEPENDS ${source} ${SLANG_DEPENDENCIES}
            COMMENT "Compiling combined shader fragment entry 'fragMain' from ${FILENAME}.slang"
        )

        list(APPEND SPV_SHADERS ${VERT_OUTPUT} ${FRAG_OUTPUT})
    endif()
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SPV_SHADERS})

target_link_libraries(${PROJECT_NAME} PUBLIC thirdparty_engine)

if(TARGET ImGui)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE -Wl,--whole-archive ImGui -Wl,--no-whole-archive)
    elseif(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE "/WHOLEARCHIVE:ImGui")
        target_link_options(${PROJECT_NAME} PRIVATE "/WHOLEARCHIVE:SDL3-static")
    elseif(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,/WHOLEARCHIVE:$<TARGET_FILE:ImGui>")
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,/WHOLEARCHIVE:$<TARGET_FILE:SDL3-static>")

        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,/EXPORT:SDL_SetWindowRelativeMouseMode")
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,/EXPORT:SDL_LogError")
    endif()
endif()

set(TP_RESOLVED_INCLUDES "")
set(TP_RESOLVED_DEFINITIONS "")

macro(extract_properties INPUT_TARGET)
    if(TARGET ${INPUT_TARGET})
        set(_REAL_TARGET ${INPUT_TARGET})

        get_target_property(_ALIASED ${INPUT_TARGET} ALIASED_TARGET)
        if(_ALIASED)
            set(_REAL_TARGET ${_ALIASED})
        endif()

        get_target_property(_INCS ${_REAL_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
        if(_INCS AND NOT "${_INCS}" MATCHES "NOTFOUND")
            list(APPEND TP_RESOLVED_INCLUDES ${_INCS})
        endif()

        get_target_property(_SYS_INCS ${_REAL_TARGET} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
        if(_SYS_INCS AND NOT "${_SYS_INCS}" MATCHES "NOTFOUND")
            list(APPEND TP_RESOLVED_INCLUDES ${_SYS_INCS})
        endif()

        get_target_property(_DEFS ${_REAL_TARGET} INTERFACE_COMPILE_DEFINITIONS)
        if(_DEFS AND NOT "${_DEFS}" MATCHES "NOTFOUND")
            list(APPEND TP_RESOLVED_DEFINITIONS ${_DEFS})
        endif()
    endif()
endmacro()

extract_properties(thirdparty_engine)

get_target_property(TP_DEPS thirdparty_engine INTERFACE_LINK_LIBRARIES)
if(TP_DEPS)
    foreach(LIB ${TP_DEPS})
        extract_properties(${LIB})
    endforeach()
endif()

extract_properties(SDL3::SDL3)
extract_properties(SDL3-static)
extract_properties(SDL3-shared)

if(DEFINED SDL3_SOURCE_DIR)
    list(APPEND TP_RESOLVED_INCLUDES "${SDL3_SOURCE_DIR}/include")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL/include")
    list(APPEND TP_RESOLVED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL/include")
endif()

list(REMOVE_DUPLICATES TP_RESOLVED_INCLUDES)
list(REMOVE_DUPLICATES TP_RESOLVED_DEFINITIONS)

message(STATUS ">> VEX Exporting Definitions: ${TP_RESOLVED_DEFINITIONS}")

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

foreach(DIR ${TP_RESOLVED_INCLUDES})
    if(DIR)
        target_include_directories(${PROJECT_NAME} PUBLIC
            $<BUILD_INTERFACE:${DIR}>
        )
    endif()
endforeach()

target_compile_definitions(${PROJECT_NAME} PUBLIC
    ${TP_RESOLVED_DEFINITIONS}
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} INTERFACE "IMGUI_API=__declspec(dllimport)")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_LINK_LIBRARIES "")

export(TARGETS VEX
    FILE "${CMAKE_BINARY_DIR}/VEXTargets.cmake"
    NAMESPACE VEX::
)
