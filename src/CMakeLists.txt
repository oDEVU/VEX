add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC components)

target_sources(${PROJECT_NAME} PRIVATE main.cpp components/window.cpp components/engine.cpp components/backends/vulkan/interface.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE thirdparty)
