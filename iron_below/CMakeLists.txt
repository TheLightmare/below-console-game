# =============================================================================
# Underlight — Standalone Game Project
# =============================================================================
# Built on the ConsoleEngine Vulkan 2D renderer.
#
# To start a new project:
#   1. Copy this entire folder somewhere
#   2. Update ENGINE_DIR below (or pass it via -DENGINE_DIR=...)
#   3. Build:
#        cmake -B build -DBUILD_VULKAN_RENDERER=ON
#        cmake --build build --config Release
#   4. Run:
#        ./build/Release/iron_below
# =============================================================================

cmake_minimum_required(VERSION 3.14)
project(Iron_Below)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Point this at the ConsoleEngine root ----
# By default, assumes the engine is one level up (sibling folder layout).
# Override with: cmake -B build -DENGINE_DIR=/path/to/consoleEngine
set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
    "Path to the ConsoleEngine repository root")

# ---- Pull in the engine (builds vk_renderer, console_engine, etc.) ----
set(BUILD_VULKAN_RENDERER ON CACHE BOOL "" FORCE)
add_subdirectory("${ENGINE_DIR}/engine" "${CMAKE_CURRENT_BINARY_DIR}/engine")

# ---- Game executable ----
add_executable(iron_below iron_below.cpp)
target_link_libraries(iron_below PRIVATE vk_renderer)

set_target_properties(iron_below PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ---- Copy compiled shaders next to the executable ----
add_custom_command(TARGET iron_below POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:iron_below>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ENGINE_SPV_DIR} $<TARGET_FILE_DIR:iron_below>/shaders
    COMMENT "Copying compiled shaders to output directory"
)
