cmake_minimum_required(VERSION 3.16)
project(VexProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable all tests" FORCE)
set(CMAKE_DISABLE_TESTING TRUE CACHE BOOL "Force disable all tests" FORCE)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(_DEBUG _ITERATOR_DEBUG_LEVEL=2)
        if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            add_link_options("-Wl,/nodefaultlib:msvcrt" "-Wl,/defaultlib:msvcrtd")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -D_DLL -D_MT -Xclang --dependent-lib=msvcrtd")
        endif()
    endif()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VexProject.json")
    message(FATAL_ERROR "Project file not found!")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VexProject.json" PROJECT_JSON)
string(JSON PROJECT_NAME ERROR_VARIABLE JSON_ERROR GET "${PROJECT_JSON}" "project_name")

if(JSON_ERROR)
    message(FATAL_ERROR "Failed to parse project file: ${JSON_ERROR}")
endif()

string(JSON MAIN_SCENE_PATH ERROR_VARIABLE SCENE_ERROR GET "${PROJECT_JSON}" "main_scene")

if(SCENE_ERROR OR MAIN_SCENE_PATH STREQUAL "")
    message(WARNING "Could not find 'main_scene' in VexProject.json. Defaulting to 'Assets/scenes/main.json'")
    set(MAIN_SCENE_PATH "Assets/scenes/main.json")
endif()

project(${PROJECT_NAME})

set(ENGINE_PATH "${VEX_ENGINE_PATH}" CACHE PATH "Path to Vex Engine Core")

message(STATUS "DEBUG: ENGINE_PATH from Cache is: '${ENGINE_PATH}'")

if(NOT ENGINE_PATH OR ENGINE_PATH STREQUAL "")
    message(FATAL_ERROR "VEX_ENGINE_PATH is missing! Command line argument was not received.")
endif()

if(NOT EXISTS "${ENGINE_PATH}")
    message(FATAL_ERROR "Provided engine path does not exist: ${ENGINE_PATH}")
endif()

if(NOT EXISTS "${ENGINE_PATH}/BuildTools/build/VPAK_Packer${CMAKE_EXECUTABLE_SUFFIX}")
    message(FATAL_ERROR "BuildTools are not present. Make sure VPAK_Packer is built.")
endif()

#add_subdirectory(${ENGINE_PATH}/Core ${CMAKE_BINARY_DIR}/Engine)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VEX_BUILD_DIR "${ENGINE_PATH}/Core/bin/Debug")
else()
    set(VEX_BUILD_DIR "${ENGINE_PATH}/Core/bin/Release")
endif()

set(VEX_TARGETS_FILE "${VEX_BUILD_DIR}/VEXTargets.cmake")

if(EXISTS "${VEX_TARGETS_FILE}")
    message(STATUS "Linking shared VEX Engine from: ${VEX_BUILD_DIR}")
    include("${VEX_TARGETS_FILE}")
else()
    message(FATAL_ERROR "VEX Engine targets not found at: ${VEX_TARGETS_FILE}\nPlease run the ProjectBuilder to compile the engine first.")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CXX_STANDARD 23)

# So zed would no longer show false errors you need to copy or link compile_commands.json file to root folder.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB GAME_SRC CONFIGURE_DEPENDS "Source/*.h" "Source/*.cpp")

add_library(GameModule SHARED ${GAME_SRC})
target_link_libraries(GameModule PUBLIC VEX::VEX)
target_compile_definitions(GameModule PRIVATE
    GAME_MODULE_EXPORTS
    VEX_MAIN_SCENE=\"${MAIN_SCENE_PATH}\"
)

if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(GameModule PRIVATE JPH_ENABLE_ASSERTS=0)
endif()

string(JSON ICON_REL_PATH ERROR_VARIABLE ICON_ERROR GET "${PROJECT_JSON}" "icon_path")

set(PROJECT_ICON_RESOURCE)

if(NOT ICON_ERROR AND NOT ICON_REL_PATH STREQUAL "")
    set(ICON_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${ICON_REL_PATH}")

    if(EXISTS "${ICON_SOURCE_PATH}")
        message(STATUS "Found Project Icon: ${ICON_SOURCE_PATH}")

        if(WIN32)
            set(ICON_OUT_ICO "${CMAKE_CURRENT_BINARY_DIR}/game_icon.ico")
            set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
            get_filename_component(ICON_EXT "${ICON_SOURCE_PATH}" EXT)
            set(HAVE_VALID_ICO FALSE)

            if(ICON_EXT STREQUAL ".png")
                find_program(MAGICK_EXE magick)
                if(MAGICK_EXE)
                    add_custom_command(
                        OUTPUT "${ICON_OUT_ICO}"
                        COMMAND ${MAGICK_EXE} "${ICON_SOURCE_PATH}" -define icon:auto-resize=256,128,64,48,32,16 "${ICON_OUT_ICO}"
                        DEPENDS "${ICON_SOURCE_PATH}"
                        COMMENT "Converting Project Icon to ICO..."
                    )
                    set(HAVE_VALID_ICO TRUE)
                endif()
            elseif(ICON_EXT STREQUAL ".ico")
                configure_file("${ICON_SOURCE_PATH}" "${ICON_OUT_ICO}" COPYONLY)
                set(HAVE_VALID_ICO TRUE)
            endif()

            if(HAVE_VALID_ICO)
                file(WRITE "${RC_FILE}" "IDI_ICON1 ICON DISCARDABLE \"${ICON_OUT_ICO}\"")

                set(PROJECT_ICON_RESOURCE "${RC_FILE}" "${ICON_OUT_ICO}")

                set(COPY_ICON_TO_BIN TRUE)
            endif()

        elseif(UNIX AND NOT APPLE)
            set(COPY_ICON_TO_BIN TRUE)
            get_filename_component(ICON_FILENAME "${ICON_SOURCE_PATH}" NAME)
        endif()
    endif()
endif()

add_executable(${PROJECT_NAME} "main.cpp" ${PROJECT_ICON_RESOURCE})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VEX_PROJECT_TITLE=\"${PROJECT_NAME}\"
)
target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-Wl,/subsystem:windows,/ENTRY:mainCRTStartup"
    )
endif()

# Enables stacktraces for Debug builds
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

#target_sources(${PROJECT_NAME} PRIVATE main.cpp)
#add_subdirectory(${ENGINE_PATH}/Core  ${CMAKE_BINARY_DIR}/Engine)
#target_link_libraries(${PROJECT_NAME} PRIVATE VEX::VEX)

set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
set(ASSETS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(copy_Assets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_DEST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ASSETS_SOURCE_DIR}"
            "${ASSETS_DEST_DIR}"
        COMMENT "Copying Assets directory"
    )
else()
    make_directory(${ASSETS_DEST_DIR})
    add_custom_target(copy_Assets ALL
        COMMAND ${ENGINE_PATH}/BuildTools/build/VPAK_Packer${CMAKE_EXECUTABLE_SUFFIX}
        ${ASSETS_SOURCE_DIR}
        ${ASSETS_DEST_DIR}/assets.vpk
        COMMENT "Packing assets for release build"
    )
endif()

if(COPY_ICON_TO_BIN)
    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ICON_OUT_ICO}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/icon.ico"
            COMMENT "Copying icon.ico to binary directory"
        )
    elseif(UNIX AND NOT APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ICON_SOURCE_PATH}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/icon.png"
            COMMENT "Copying icon image to binary directory as icon.png"
        )
    endif()
endif()

add_dependencies(${PROJECT_NAME} copy_Assets)
