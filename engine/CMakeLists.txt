# =============================================================================
# ConsoleEngine — Engine Libraries
# =============================================================================
# This file builds the engine as reusable library targets:
#   - console_engine  (text/terminal renderer, ECS, scenes, input, world)
#   - vk_renderer     (Vulkan 2D rendering backend, opt-in)
#
# Include from your game project:
#   set(ENGINE_DIR "/path/to/consoleEngine")
#   set(BUILD_VULKAN_RENDERER ON CACHE BOOL "" FORCE)
#   add_subdirectory("${ENGINE_DIR}/engine" "${CMAKE_BINARY_DIR}/engine")
#
#   add_executable(my_game main.cpp)
#   target_link_libraries(my_game PRIVATE vk_renderer)
# =============================================================================

# Store our source root so paths work regardless of who includes us
set(ENGINE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# =============================================================================
# Console-based engine library
# =============================================================================

add_library(console_engine STATIC
    ${ENGINE_SRC_DIR}/src/console.cpp
    ${ENGINE_SRC_DIR}/src/ui.cpp
)

# PUBLIC so consumers can #include "render/console.hpp", "input/input.hpp", etc.
target_include_directories(console_engine PUBLIC
    ${ENGINE_SRC_DIR}
)

if(MSVC)
    target_compile_options(console_engine PRIVATE /W4 /EHsc /utf-8)
else()
    target_compile_options(console_engine PRIVATE -Wall -Wextra -O2)
endif()

# =============================================================================
# Vulkan 2D Renderer (opt-in via -DBUILD_VULKAN_RENDERER=ON)
# =============================================================================

option(BUILD_VULKAN_RENDERER "Build the Vulkan 2D rendering backend" OFF)

if(BUILD_VULKAN_RENDERER)
    include(FetchContent)

    # ---- Find Vulkan SDK ----
    find_package(Vulkan REQUIRED)

    # ---- Find SDL2 ----
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-2.30.0
            GIT_SHALLOW    TRUE
        )
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON  CACHE BOOL "" FORCE)
        set(SDL_TEST   OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(SDL2)
        set(SDL2_LIBRARIES SDL2::SDL2-static SDL2::SDL2main)
        set(SDL2_INCLUDE_DIRS "")
    else()
        set(SDL2_LIBRARIES SDL2::SDL2)
    endif()

    # ---- Fetch VMA (header-only) ----
    FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG        v3.1.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(VulkanMemoryAllocator)

    # ---- Fetch stb (header-only) ----
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(stb)

    # ---- stb implementation TU ----
    configure_file(
        ${ENGINE_SRC_DIR}/render/vulkan/stb_impl.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp
        COPYONLY
    )

    # ---- VMA implementation TU ----
    configure_file(
        ${ENGINE_SRC_DIR}/render/vulkan/vma_impl.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/vma_impl.cpp
        COPYONLY
    )

    # ---- Vulkan renderer sources ----
    set(VK_RENDERER_SOURCES
        ${ENGINE_SRC_DIR}/render/vulkan/vk_window.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_context.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_buffer.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_pipeline.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_texture.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_sprite_batch.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_text.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_renderer.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_input.cpp
        ${ENGINE_SRC_DIR}/render/vulkan/vk_ui.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/vma_impl.cpp
    )

    # ---- Library target ----
    add_library(vk_renderer STATIC ${VK_RENDERER_SOURCES})

    target_include_directories(vk_renderer PUBLIC
        ${ENGINE_SRC_DIR}
        ${Vulkan_INCLUDE_DIRS}
        ${stb_SOURCE_DIR}
    )

    target_include_directories(vk_renderer PUBLIC
        ${vulkanmemoryallocator_SOURCE_DIR}/include
    )

    target_link_libraries(vk_renderer PUBLIC
        Vulkan::Vulkan
        ${SDL2_LIBRARIES}
    )

    if(MSVC)
        target_compile_options(vk_renderer PRIVATE /W4 /EHsc /utf-8)
    else()
        target_compile_options(vk_renderer PRIVATE -Wall -Wextra -O2)
    endif()

    # ---- Compile GLSL -> SPIR-V at build time ----
    set(ENGINE_SHADER_DIR ${ENGINE_SRC_DIR}/render/vulkan/shaders)
    set(ENGINE_SPV_DIR    ${CMAKE_CURRENT_BINARY_DIR}/shaders CACHE INTERNAL
        "Directory containing compiled SPIR-V shaders (set by engine)")
    file(MAKE_DIRECTORY ${ENGINE_SPV_DIR})

    set(SHADER_SOURCES
        ${ENGINE_SHADER_DIR}/sprite.vert
        ${ENGINE_SHADER_DIR}/sprite.frag
    )

    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_FILE ${ENGINE_SPV_DIR}/${SHADER_NAME}.spv)
        add_custom_command(
            OUTPUT  ${SPV_FILE}
            COMMAND Vulkan::glslc ${SHADER} -o ${SPV_FILE}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} -> SPIR-V"
        )
        list(APPEND SPV_FILES ${SPV_FILE})
    endforeach()

    add_custom_target(compile_shaders ALL DEPENDS ${SPV_FILES})
    add_dependencies(vk_renderer compile_shaders)

    message(STATUS "Vulkan 2D renderer: ENABLED")
    message(STATUS "  SDL2 libs:  ${SDL2_LIBRARIES}")
    message(STATUS "  Vulkan SDK: ${Vulkan_LIBRARY}")
endif()
