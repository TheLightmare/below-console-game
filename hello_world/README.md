# Hello World — ConsoleEngine Starter Project

A minimal, ready-to-run game project built on the ConsoleEngine's Vulkan 2D renderer. Copy this folder to start a new project — no need to touch the engine's build files.

## Prerequisites

- **CMake** 3.14+
- **C++17** compiler (MSVC, GCC, or Clang)
- **Vulkan SDK** installed ([lunarg.com/vulkan-sdk](https://vulkan.lunarg.com/sdk/home))

## Creating a new project

1. **Copy this entire folder** somewhere on your machine (rename it to your project name).

2. **Update the engine path.** Open `CMakeLists.txt` and edit the `ENGINE_DIR` line to point at the ConsoleEngine root:

   ```cmake
   set(ENGINE_DIR "C:/path/to/consoleEngine" CACHE PATH
       "Path to the ConsoleEngine repository root")
   ```

   Or leave it as-is if your project folder sits right next to the engine (the default `..` will work).

   You can also skip editing the file and pass it at configure time:
   ```
   cmake -B build -DENGINE_DIR="C:/path/to/consoleEngine"
   ```

3. **Rename the target** (optional). In `CMakeLists.txt`, change `HelloWorld`, `hello_world`, and the source filename to match your project.

## Building

Double-click **`build.bat`** or run manually:

```
cmake -B build -DBUILD_VULKAN_RENDERER=ON
cmake --build build --config Release
```

The executable lands in `build/Release/`.

## Running

```
build\Release\hello_world.exe
```

Controls: **WASD** to move, **+/-** to zoom, **ESC** to quit.

## Project structure

```
hello_world/
├── CMakeLists.txt      ← Build config (edit ENGINE_DIR here)
├── build.bat           ← One-click build script
├── hello_world.cpp     ← Your game code (start here)
└── README.md           ← This file
```

## What's included in the starter code

- Window creation and Vulkan init
- Game loop with input polling
- Grid-based player movement
- Camera that follows the player
- Tile map rendering (checkerboard)
- HUD text overlay (screen-space)

All in a single `hello_world.cpp` — a clean starting point to build on.

## Adding more source files

Add them to the `add_executable` call in `CMakeLists.txt`:

```cmake
add_executable(hello_world
    hello_world.cpp
    player.cpp
    world.cpp
)
```

## Engine API at a glance

```cpp
VkRenderer renderer;
renderer.init("My Game", 1280, 720);

while (renderer.is_running()) {
    Key key = renderer.poll_input();

    renderer.begin_frame(0.05f, 0.05f, 0.08f);  // RGB clear color

    renderer.draw_rect({x, y, w, h}, Color4::yellow());
    renderer.draw_text(x, y, "Hello!", Color4::white(), 2.0f);

    renderer.end_frame();
}

renderer.destroy();
```

See the engine's `TUTORIAL.md` and `demos/` folder for more advanced usage.
