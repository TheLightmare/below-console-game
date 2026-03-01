# Underlight

A game built on the ConsoleEngine's Vulkan 2D renderer.

## Prerequisites

- **CMake** 3.14+
- **C++17** compiler (MSVC, GCC, or Clang)
- **Vulkan SDK** installed ([lunarg.com/vulkan-sdk](https://vulkan.lunarg.com/sdk/home))

## Building

Double-click **`build.bat`** or run manually:

```
cmake -B build -DBUILD_VULKAN_RENDERER=ON
cmake --build build --config Release
```

The executable lands in `build/Release/`.

## Running

```
build\Release\underlight.exe
```

Controls: **WASD** to move, **+/-** to zoom, **ESC** to quit.

## Project structure

```
underlight/
├── CMakeLists.txt      ← Build config (edit ENGINE_DIR here)
├── build.bat           ← One-click build script
├── underlight.cpp      ← Your game code (start here)
└── README.md           ← This file
```

## Adding more source files

Add them to the `add_executable` call in `CMakeLists.txt`:

```cmake
add_executable(underlight
    underlight.cpp
    player.cpp
    world.cpp
)
```
