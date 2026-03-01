# Console Engine

A lightweight C++17 console-based game engine featuring:

- **ECS (Entity-Component-System)** architecture with a type-indexed `ComponentManager`
- **Console rendering** with dual Windows API / ANSI escape code backends and RGB color support
- **Scene management** with factory registration, persistent scene caching, and transitions
- **Chunked world system** with Perlin noise generation and multi-chunk structure support
- **Cross-platform input** with configurable key bindings (Windows `_kbhit`/`_getch`, Linux `termios`)
- **UI primitives**: panels, text, progress bars, menus, message logs, confirmation dialogs
- **Utilities**: custom JSON parser/serializer, RNG, Perlin noise

## Building

**Linux:** `make`

**Windows:** run `build.bat` (output in `build/Release/`)

## Project Structure

```
engine/
  ecs/          - Entity-Component-System (ComponentManager, Entity, Components)
  input/        - Cross-platform input handling and key bindings
  render/       - Console abstraction, Renderer, UI widgets
  scene/        - Scene base class and SceneManager
  src/          - .cpp implementations (console, ui)
  util/         - JSON parser, RNG
  world/        - World, Chunk, ChunkedWorld, Perlin noise, tile utilities
main.cpp        - Minimal engine demo
```

## Cleanup

**Linux:** `make clean`

**Windows:** delete the `build/` folder manually
