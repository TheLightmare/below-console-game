cmake_minimum_required(VERSION 3.14)
project(ConsoleEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Engine libraries (console_engine, vk_renderer)
# =============================================================================
add_subdirectory(engine)

# =============================================================================
# Helper: add a Vulkan game/demo target with shader copying
# =============================================================================
function(add_vk_target TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE vk_renderer)
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ENGINE_SPV_DIR} $<TARGET_FILE_DIR:${TARGET_NAME}>/shaders
        COMMENT "Copying compiled shaders to output directory (${TARGET_NAME})"
    )
endfunction()

# =============================================================================
# Console-based demos
# =============================================================================

add_executable(engine_demo main.cpp)
target_link_libraries(engine_demo PRIVATE console_engine)
# main.cpp uses #include "engine/..." style, so it needs the repo root
target_include_directories(engine_demo PRIVATE ${CMAKE_SOURCE_DIR})
set_target_properties(engine_demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(zlevel_game demos/zlevel_game.cpp)
target_link_libraries(zlevel_game PRIVATE console_engine)
target_include_directories(zlevel_game PRIVATE ${CMAKE_SOURCE_DIR})
set_target_properties(zlevel_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# Vulkan-based demos
# =============================================================================

if(BUILD_VULKAN_RENDERER)
    add_vk_target(vk_demo    demos/vk_demo.cpp)
    add_vk_target(vk_game    demos/vk_game.cpp)
    add_vk_target(vk_test    demos/vk_test.cpp)
    add_vk_target(vk_ui_demo demos/vk_ui_demo.cpp)
    add_vk_target(hello_world hello_world/hello_world.cpp)
endif()

# Print build info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
