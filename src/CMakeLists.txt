add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC components)

target_sources(${PROJECT_NAME} PRIVATE main.cpp components/window.cpp components/resolution_manager.cpp components/mesh.cpp components/engine.cpp components/backends/vulkan/interface.cpp components/backends/vulkan/swapchain_manager.cpp components/backends/vulkan/pipeline.cpp components/backends/vulkan/resources.cpp components/backends/vulkan/vulkan_mesh.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE thirdparty)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
