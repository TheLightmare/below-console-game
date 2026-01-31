cmake_minimum_required(VERSION 3.10)
project(ConsoleGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/engine)
include_directories(${CMAKE_SOURCE_DIR})

# Engine source files
set(ENGINE_SOURCES
    engine/src/console.cpp
    engine/src/ui.cpp
    engine/src/component.cpp
    engine/src/ai_system.cpp
    engine/src/combat_system.cpp
    engine/src/inventory_system.cpp
    engine/src/interaction_system.cpp
)

# Game source files
set(GAME_SOURCES
    main.cpp
)

# All sources
set(SOURCES
    ${ENGINE_SOURCES}
    ${GAME_SOURCES}
)

# Create executable
add_executable(game ${SOURCES})

# Compiler-specific options
if(MSVC)
    target_compile_options(game PRIVATE /W4 /EHsc)
    # Enable UTF-8 support
    target_compile_options(game PRIVATE /utf-8)
else()
    target_compile_options(game PRIVATE -Wall -Wextra -O2)
endif()

# Platform-specific libraries
if(WIN32)
    # Link Windows console API
    target_link_libraries(game)
else()
    # Linux doesn't need special linking for termios
endif()

# Set output directory
set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Copy assets folder to build directory if it exists
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Print build info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
