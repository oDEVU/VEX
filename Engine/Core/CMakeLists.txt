cmake_minimum_required(VERSION 3.16)

project(VEX)
set(NAMESPACE_NAME "VEX")

option(BUILD_SHARED_LIBS "Build the shared library" OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(CPP_LIBRARY_VERSION_MAJOR 0)
set(CPP_LIBRARY_VERSION_MINOR 1)
set(CPP_LIBRARY_VERSION_PATCH 0)
set(CPP_LIBRARY_VERSION_STRING ${CPP_LIBRARY_VERSION_MAJOR}.${CPP_LIBRARY_VERSION_MINOR}.${CPP_LIBRARY_VERSION_PATCH})

add_library(${PROJECT_NAME})
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

# Link to consolidated third-party interface
target_link_libraries(${PROJECT_NAME} PUBLIC thirdparty_engine)

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
        src/components/GameObjects/Creators/ModelCreator.cpp
    )

#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    list(APPEND source_files src/components/JoltDebugRenderer.cpp)
#endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    LANGUAGES CXX
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED OFF
    CXX_EXTENSIONS OFF
    VERSION ${CPP_LIBRARY_VERSION_STRING}
    SOVERSION ${CPP_LIBRARY_VERSION_MAJOR}
    PUBLIC_HEADER "${public_header_files}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${public_header_files}
        ${source_files}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#==============================================================================
# INSTALLATION (Only for standalone engine builds)
#==============================================================================
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ConfigPackageLocation "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Library
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Library
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  COMPONENT Library
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME} COMPONENT Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${NAMESPACE_NAME}::
        DESTINATION ${ConfigPackageLocation}
        COMPONENT Development
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm/glm
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    configure_package_config_file(
        ${PROJECT_NAME}Config.cmake.in
        ${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION "${ConfigPackageLocation}"
        PATH_VARS CMAKE_INSTALL_PREFIX
    )

    write_basic_package_version_file(
        ${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${CPP_LIBRARY_VERSION_STRING}
        COMPATIBILITY AnyNewerVersion
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION
            ${ConfigPackageLocation}
        COMPONENT
            Development
    )
endif()

#==============================================================================
# COMPILE SHADERS
#==============================================================================
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB SHADERS
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

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS SHADERS)
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

add_custom_target(compile_shaders ALL DEPENDS ${SPV_SHADERS})
