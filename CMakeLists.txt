cmake_minimum_required(VERSION 3.20)

project(opengl_sandbox VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glad/cmake glad_cmake)
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm EXCLUDE_FROM_ALL)

glad_add_library(glad STATIC REPRODUCIBLE API gl:core=4.6)

add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
        third_party/imgui
        third_party/imgui/backends
)

target_link_libraries(imgui
    PRIVATE
        glfw
)

add_executable(${PROJECT_NAME}
    src/sandbox/assets/image_loader.cpp
    src/sandbox/logging.cpp
    src/main.cpp
    src/sandbox/graphics/mesh_utils.cpp
    src/sandbox/graphics/primitives.cpp
    src/sandbox/graphics/shader_utils.cpp
    src/sandbox/state_manager.cpp
    src/sandbox/states/selector_menu_state.cpp
    src/sandbox/states/fragment_playground_state.cpp
    src/sandbox/states/hello_cube_state.cpp
    src/sandbox/states/voxel_game_state.cpp
    src/sandbox/voxel/runtime.cpp
    src/sandbox/voxel/meshing/controller.cpp
    src/sandbox/voxel/render/material_pack.cpp
    src/sandbox/voxel/streaming/residency.cpp
    src/sandbox/voxel/streaming/terrain_generator.cpp
    src/sandbox/voxel/world/world.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
    third_party/stb
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        SANDBOX_SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glad
        glfw
    glm::glm
        imgui
        spdlog::spdlog
        OpenGL::GL
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++exp)
endif()
