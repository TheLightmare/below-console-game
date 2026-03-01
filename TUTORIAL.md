# Console Engine — Game Development Tutorial

This step-by-step tutorial walks you through building a complete game using the
Console Engine. By the end you will have a playable top-down adventure with a
tile map, a moving player, enemies, items, a HUD, multiple scenes, and
procedural world generation.

The engine ships with **two rendering backends**:

| Backend | Header | Best for |
|---------|--------|----------|
| **Console** (ASCII terminal) | `engine/render/console.hpp` | Roguelikes, quick prototyping |
| **Vulkan 2D** | `engine/render/vulkan/vk_renderer.hpp` | GPU-accelerated 2D games |

Both backends share the same ECS, input, scene, world, and utility systems.
This tutorial covers both, with separate sections where the rendering code
diverges.

---

## Table of Contents

1. [Prerequisites & Building](#1-prerequisites--building)
2. [Project Skeleton](#2-project-skeleton)
3. [The Game Loop](#3-the-game-loop)
4. [Creating a Tile Map (World)](#4-creating-a-tile-map-world)
   - 4a. Fixed-size maps
   - 4b. Z-Levels (3D Worlds)
5. [Entity-Component-System (ECS)](#5-entity-component-system-ecs)
6. [Input Handling & Key Bindings](#6-input-handling--key-bindings)
7. [Rendering](#7-rendering)
   - 7a. Console Backend
   - 7b. Vulkan Backend
8. [UI Widgets](#8-ui-widgets)
   - 8a. Console Backend
   - 8b. Vulkan Backend — Immediate-Mode UI (`VkUI`)
9. [Scenes & Scene Manager](#9-scenes--scene-manager)
10. [Procedural Generation with Perlin Noise](#10-procedural-generation-with-perlin-noise)
11. [Infinite Worlds with ChunkedWorld](#11-infinite-worlds-with-chunkedworld)
12. [Utilities (JSON, RNG)](#12-utilities-json-rng)
13. [Putting It All Together — A Complete Mini-Game](#13-putting-it-all-together--a-complete-mini-game)
14. [Next Steps](#14-next-steps)

---

## 1. Prerequisites & Building

### Requirements

- **C++17** compiler (MSVC 2019+, GCC 9+, Clang 10+)
- **CMake 3.14+**
- *(Vulkan backend only)* Vulkan SDK with `glslc` shader compiler

### Building — Console backend only

```bash
# Linux / macOS
make

# Windows (from project root)
build.bat
```

The console-only build produces `engine_demo` (or `build/Release/engine_demo.exe`
on Windows). No extra dependencies are required.

### Building — With Vulkan backend

```bash
cmake -B build -DBUILD_VULKAN_RENDERER=ON
cmake --build build --config Release
```

This fetches SDL2, VulkanMemoryAllocator, and stb automatically via CMake
FetchContent, compiles the GLSL shaders to SPIR-V, and produces the `vk_demo`,
`vk_game`, and `vk_test` executables alongside your console build.

### Adding your own target

In `CMakeLists.txt`, add an executable and link the appropriate library:

```cmake
# Console-only game
add_executable(my_game my_game.cpp ${ENGINE_SOURCES})

# Vulkan-based game
add_executable(my_vk_game my_vk_game.cpp)
target_link_libraries(my_vk_game PRIVATE vk_renderer)
```

For Vulkan targets, remember to also copy the compiled shaders to the output
directory (see the existing `vk_game` target in `CMakeLists.txt` for the
post-build copy command).

---

## 2. Project Skeleton

A minimal Console Engine game has only a handful of moving parts:

```
my_game.cpp          ← Your game code (single file is fine to start)
engine/              ← The engine (included via headers)
  ecs/               ← Entity-Component-System
  input/             ← Keyboard input & key bindings
  render/            ← Console renderer, UI widgets, Vulkan backend
  scene/             ← Scene base class and scene manager
  util/              ← JSON parser, RNG
  world/             ← Tile world, chunks, Perlin noise
```

### Minimum includes (console game)

```cpp
#include "engine/render/console.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/ui.hpp"
#include "engine/input/input.hpp"
#include "engine/ecs/component_manager.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/scene/scene_manager.hpp"
#include "engine/world/world.hpp"
```

### Minimum includes (Vulkan game)

```cpp
#include "render/vulkan/vk_renderer.hpp"  // includes everything Vulkan-side
#include "engine/input/input.hpp"         // Key enum shared by both backends
```

---

## 3. The Game Loop

Every game built on this engine follows the same core loop:

```
while (running) {
    1. Read input
    2. Update game state
    3. Render
    4. Sleep / vsync
}
```

### Console backend loop

```cpp
#include "engine/render/console.hpp"
#include "engine/render/renderer.hpp"
#include "engine/input/input.hpp"
#include "engine/ecs/component_manager.hpp"

#ifdef _WIN32
  #include <windows.h>
  #define SLEEP_MS(ms) Sleep(ms)
#else
  #include <unistd.h>
  #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

int main() {
    Console console(80, 30);                  // 80×30 character grid
    ComponentManager manager;
    Renderer renderer(&console, &manager);

    bool running = true;
    while (running) {
        Key key = Input::get_key();           // non-blocking
        if (key == Key::ESCAPE) running = false;

        // ... update game state ...

        console.clear();
        // ... draw things ...
        console.present();                    // flush to terminal

        SLEEP_MS(16);                         // ~60 FPS
    }
    return 0;
}
```

### Vulkan backend loop

```cpp
#include "render/vulkan/vk_renderer.hpp"

int main() {
    VkRenderer renderer;
    renderer.init("My Game", 1280, 720);      // opens an SDL window

    while (renderer.is_running()) {
        Key key = renderer.poll_input();       // returns Key::NONE if idle
        if (key == Key::ESCAPE) break;

        // ... update game state ...

        renderer.begin_frame(0.0f, 0.0f, 0.0f);  // clear color (R,G,B)
        // ... draw things ...
        renderer.end_frame();                      // present (vsync)
    }

    renderer.destroy();
    return 0;
}
```

---

## 4. Creating a Tile Map (World)

### 4a. The `World` class — fixed-size maps

`World` is a fixed-size grid of `Tile` structs, perfect for rooms, dungeons, or
fixed-size levels. The third constructor argument sets the number of z-levels
(defaults to 1 for a flat 2D map).

```cpp
#include "engine/world/world.hpp"

// Create a flat 60×30 world (single z-level)
World world(60, 30);

// Create a 3D world with 5 z-levels
World world_3d(80, 50, 5);

// Fill every tile with grass (flat world)
for (int y = 0; y < world.get_height(); ++y) {
    for (int x = 0; x < world.get_width(); ++x) {
        Tile& tile = world.get_tile(x, y);  // z defaults to 0
        tile.symbol   = '.';
        tile.color    = "green";
        tile.walkable = true;
    }
}

// Add a wall
Tile& wall = world.get_tile(10, 5);
wall.symbol   = '#';
wall.color    = "gray";
wall.walkable = false;
```

### The `Tile` struct

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `walkable` | `bool` | `true` | Can entities walk on it? |
| `symbol` | `char` | `'.'` | ASCII character drawn on screen |
| `color` | `std::string` | `"white"` | Named color (`"red"`, `"green"`, `"dark_cyan"`, …) |
| `movement_cost` | `float` | `1.0` | Speed modifier (0.5 = takes 2 turns) |
| `has_floor` | `bool` | `true` | Solid floor? If `false` + `transparent`, shows the level below |
| `has_ceiling` | `bool` | `false` | Covered from above? |
| `is_stairs_up` | `bool` | `false` | Staircase going up (displayed as `<`) |
| `is_stairs_down` | `bool` | `false` | Staircase going down (displayed as `>`) |
| `is_ramp` | `bool` | `false` | Ramp connecting to the level above |
| `transparent` | `bool` | `true` | Can you see through to levels below? |
| `below_symbol` | `char` | `' '` | Symbol shown when viewed from above (dimmed) |
| `below_color` | `std::string` | `"dark_gray"` | Color when viewed from above |

**Available color names:** `black`, `blue`, `green`, `cyan`, `red`, `magenta`,
`yellow`, `white`, `gray`, `dark_gray`, `brown`, `dark_green`, `dark_blue`,
`dark_cyan`, `dark_red`, `dark_yellow`, `dark_magenta`.

### `World` API reference

| Method | Description |
|--------|-------------|
| `get_tile(x, y, z = 0)` | Access a tile (z defaults to 0 for flat worlds) |
| `set_tile(x, y, tile, z = 0)` | Set a tile |
| `get_width()` / `get_height()` / `get_depth()` | Dimensions |
| `valid_z(z)` | Is this z-level in range? |
| `can_go_up(x, y, z)` | Can an entity ascend from (x,y,z)? (stairs/ramp) |
| `can_go_down(x, y, z)` | Can an entity descend from (x,y,z)? |

### 4b. Z-Levels (3D Worlds)

The engine supports Dwarf Fortress-style z-levels: multiple stacked layers of
tiles that entities can move between via stairs or ramps. This is useful for
multi-floor buildings, underground dungeons, and layered overworlds.

#### Creating a multi-level world

```cpp
// 80×50 world with 5 z-levels (z=0 is the deepest, z=4 is the highest)
World world(80, 50, 5);

// Fill z=0 with lava cavern
for (int y = 0; y < 50; ++y)
    for (int x = 0; x < 80; ++x) {
        Tile& t = world.get_tile(x, y, 0);
        t.symbol = '.';
        t.color = "dark_red";
        t.walkable = true;
    }

// Fill z=1 with stone corridors
for (int y = 0; y < 50; ++y)
    for (int x = 0; x < 80; ++x) {
        Tile& t = world.get_tile(x, y, 1);
        t.symbol = '#';
        t.color = "dark_gray";
        t.walkable = false;  // solid rock by default
    }
// ... then carve rooms and corridors on each level
```

#### Placing stairs

Stairs connect two adjacent z-levels. Mark the lower tile as `is_stairs_up`
and the upper tile as `is_stairs_down` **at the same (x, y) coordinate**:

```cpp
// Connect z=2 to z=3 at position (35, 20)
Tile& lower = world.get_tile(35, 20, 2);
lower.is_stairs_up = true;
lower.symbol = '<';      // conventional: '<' = go up
lower.color  = "white";
lower.walkable = true;

Tile& upper = world.get_tile(35, 20, 3);
upper.is_stairs_down = true;
upper.symbol = '>';      // conventional: '>' = go down
upper.color  = "white";
upper.walkable = true;
upper.has_floor = true;
```

The `World` class provides helpers to check if vertical movement is possible:

```cpp
if (world.can_go_up(player_x, player_y, current_z)) {
    current_z += 1;  // ascend
}
if (world.can_go_down(player_x, player_y, current_z)) {
    current_z -= 1;  // descend
}
```

#### See-through tiles (looking down from above)

When a tile has `has_floor = false` and `transparent = true`, the renderer
shows the level below it (dimmed) instead of the current tile — creating the
effect of open air, grates, chasms, or balconies:

```cpp
// Open-air tile on the tower roof — shows the floor below
Tile& grate = world.get_tile(30, 15, 4);
grate.has_floor    = false;
grate.transparent  = true;
grate.symbol       = '#';        // drawn if nothing below is visible
grate.color        = "dark_cyan";
grate.walkable     = false;      // can't walk over a hole

// The tile below controls what the player sees when looking down
Tile& room_below = world.get_tile(30, 15, 3);
room_below.below_symbol = '.';        // shown when viewed from z=4
room_below.below_color  = "dark_gray"; // dimmed color
```

The renderer's `z_see_through_depth` setting controls how many levels deep
transparent tiles peek through (default: 1):

```cpp
renderer.set_z_see_through_depth(2);  // see up to 2 levels down
```

#### Moving entities between z-levels

`PositionComponent` has a `z` field (defaults to 0). To move an entity
between floors:

```cpp
auto* pos = manager.get_component<PositionComponent>(player_id);

// Player pressed '<' — try to go up
if (world.can_go_up(pos->x, pos->y, pos->z)) {
    pos->z += 1;
    messages.push_back("You ascend the stairs.");
}

// Player pressed '>' — try to go down
if (world.can_go_down(pos->x, pos->y, pos->z)) {
    pos->z -= 1;
    messages.push_back("You descend the stairs.");
}
```

The `Renderer` tracks the camera z-level automatically when you call
`center_camera_on_entity()`, so only the player's current floor (and
transparent peeks below) are rendered:

```cpp
renderer.center_camera_on_entity(player_id);  // follows x, y, AND z
renderer.render_all(&world);                   // draws camera_z level
```

#### Filtering entities by z-level

The renderer automatically skips entities that are not on the current camera
z-level. For game logic (e.g. enemy AI), filter manually:

```cpp
auto* player_pos = manager.get_component<PositionComponent>(player_id);

for (EntityId eid : manager.get_entities_with_component<EnemyTag>()) {
    auto* epos = manager.get_component<PositionComponent>(eid);
    if (!epos || epos->z != player_pos->z) continue;  // skip other floors

    // ... AI logic for enemies on the same floor ...
}

// The built-in query also supports z:
bool blocked = manager.is_position_blocked(10, 5, /*exclude=*/player_id, /*z=*/2);
EntityId enemy = manager.get_entity_at<EnemyTag>(10, 5, /*exclude=*/0, /*z=*/2);
```

#### Example: a 5-level dungeon

Here is a condensed example showing how to build a small multi-floor keep:

```cpp
World world(80, 50, 5);  // 5 floors

// Z=4: Tower roof (open air with parapet)
for (int y = 14; y < 28; ++y)
    for (int x = 25; x < 45; ++x) {
        Tile& t = world.get_tile(x, y, 4);
        if (x == 25 || x == 44 || y == 14 || y == 27) {
            t = Tile(false, '#', "white");  // parapet wall
        } else {
            t = Tile(true, '.', "gray");    // stone floor
        }
    }

// Z=3: Keep ground floor (great hall)
for (int y = 10; y < 40; ++y)
    for (int x = 20; x < 60; ++x) {
        Tile& t = world.get_tile(x, y, 3);
        if (x == 20 || x == 59 || y == 10 || y == 39)
            t = Tile(false, '#', "white");
        else
            t = Tile(true, '.', "dark_yellow");
    }

// Z=2: Dungeon corridors + prison cells
// Z=1: Catacombs with winding passages
// Z=0: Dragon's lair (lava cavern)
// ... (see demos/zlevel_game.cpp for the full implementation)

// Connect floors with stairs
auto place_stairs = [&](int x, int y, int z_low, int z_high) {
    Tile& lo = world.get_tile(x, y, z_low);
    lo.is_stairs_up = true;
    lo.symbol = '<'; lo.color = "white"; lo.walkable = true;

    Tile& hi = world.get_tile(x, y, z_high);
    hi.is_stairs_down = true;
    hi.symbol = '>'; hi.color = "white"; hi.walkable = true;
    hi.has_floor = true;
};

place_stairs(34, 16, 3, 4);  // Keep → Tower roof
place_stairs(35, 38, 2, 3);  // Dungeon → Keep
place_stairs(35, 42, 1, 2);  // Catacombs → Dungeon
place_stairs(35, 10, 0, 1);  // Dragon's lair → Catacombs
```

See [demos/zlevel_game.cpp](demos/zlevel_game.cpp) for a complete 5-floor
roguelike ("Depths of the Keep") that demonstrates all z-level features:
stairs, transparent grates, per-floor enemies and items, locked doors, and
a HUD showing the current floor name.

---

## 5. Entity-Component-System (ECS)

The engine provides a lightweight ECS built around `ComponentManager`. Entities
are plain integer IDs (`uint32_t`). Components are structs that inherit from
`Component`.

### Creating entities and adding components

```cpp
#include "engine/ecs/component_manager.hpp"
#include "engine/ecs/entity.hpp"

ComponentManager manager;

// Create a player entity
EntityId player = manager.create_entity();
manager.add_component<PositionComponent>(player, 5, 5, 0);       // x, y, z
manager.add_component<RenderComponent>(player, '@', "yellow", "", 100);
manager.add_component<VelocityComponent>(player, 0, 0, 1.0f);
manager.add_component<CollisionComponent>(player, true, false);   // blocks movement, not sight
manager.add_component<PlayerControllerComponent>(player, true, true);

// Create a tree (decorative entity)
EntityId tree = manager.create_entity();
manager.add_component<PositionComponent>(tree, 15, 8, 0);
manager.add_component<RenderComponent>(tree, 'T', "green", "", 50);
```

### Built-in component types

| Component | Fields | Purpose |
|-----------|--------|---------|
| `PositionComponent` | `x`, `y`, `z` | World-space position. Has `manhattan_distance()`, `chebyshev_distance()`, arithmetic operators. |
| `RenderComponent` | `ch`, `color`, `bg_color`, `priority` | How the entity is drawn. Higher `priority` draws on top. |
| `VelocityComponent` | `dx`, `dy`, `speed` | Movement direction and speed. |
| `CollisionComponent` | `blocks_movement`, `blocks_sight` | Physics / FOV blocking. |
| `PlayerControllerComponent` | `can_move`, `can_attack` | Marks entity as player-controlled. |
| `PropertiesComponent` | `properties` (string list) | Arbitrary tags/traits (e.g. `"poisoned"`, `"blessed"`). |

### Querying entities

```cpp
// Get a component (returns nullptr if missing)
PositionComponent* pos = manager.get_component<PositionComponent>(player);

// Check if entity has a component
if (manager.has_component<CollisionComponent>(enemy)) { /* ... */ }

// Get all entities that have a specific component
std::vector<EntityId> movers = manager.get_entities_with_component<VelocityComponent>();

// Check if a tile is blocked by any entity
bool blocked = manager.is_position_blocked(10, 5, player); // exclude player from check

// Find an entity of type T at a position
EntityId item = manager.get_entity_at<RenderComponent>(10, 5);
```

### The `Entity` wrapper (optional)

For convenience, the `Entity` class wraps an ID + manager pointer:

```cpp
Entity player_entity(player, &manager);

// Same as manager.get_component<PositionComponent>(player)
auto* pos = player_entity.get_component<PositionComponent>();

// Shorthand for getting (x, y)
auto maybe_pos = player_entity.get_position(); // std::optional<pair<int,int>>
```

### Creating custom components

Define a struct inheriting from `Component` and use the `IMPLEMENT_CLONE` macro:

```cpp
#include "engine/ecs/components/component_base.hpp"

struct HealthComponent : Component {
    int current = 10;
    int maximum = 10;

    HealthComponent() = default;
    HealthComponent(int hp, int max_hp) : current(hp), maximum(max_hp) {}

    bool is_dead() const { return current <= 0; }

    IMPLEMENT_CLONE(HealthComponent)
};

struct NameComponent : Component {
    std::string name;

    NameComponent() = default;
    NameComponent(const std::string& n) : name(n) {}

    IMPLEMENT_CLONE(NameComponent)
};

// Usage:
EntityId goblin = manager.create_entity();
manager.add_component<PositionComponent>(goblin, 20, 10, 0);
manager.add_component<RenderComponent>(goblin, 'g', "green", "", 80);
manager.add_component<HealthComponent>(goblin, 12, 12);
manager.add_component<NameComponent>(goblin, "Goblin");
```

---

## 6. Input Handling & Key Bindings

### Reading keys

```cpp
#include "engine/input/input.hpp"

// Non-blocking — returns Key::NONE if nothing was pressed
Key key = Input::get_key();

// Blocking — waits until a key is pressed
Key key = Input::wait_for_key();
```

On the Vulkan backend, use `renderer.poll_input()` instead (it pumps SDL events
and returns a `Key`).

### The `Key` enum

Common values: `Key::UP`, `Key::DOWN`, `Key::LEFT`, `Key::RIGHT`, `Key::W`,
`Key::A`, `Key::S`, `Key::D`, `Key::ESCAPE`, `Key::ENTER`, `Key::SPACE`,
`Key::E`, `Key::Q`, `Key::I`, `Key::TAB`, `Key::PLUS`, `Key::MINUS`, etc.

### Rebindable key bindings

The `KeyBindings` singleton maps physical keys to abstract game actions:

```cpp
auto& bindings = KeyBindings::instance();

// Check if a key maps to a game action
if (bindings.is_action(key, GameAction::MOVE_UP))    { /* player moves up */ }
if (bindings.is_action(key, GameAction::MOVE_DOWN))  { /* ... */ }
if (bindings.is_action(key, GameAction::ACTION_1))   { /* interact */ }
if (bindings.is_action(key, GameAction::CANCEL))     { /* open menu / quit */ }

// Rebind keys at runtime
bindings.set_primary(GameAction::MOVE_UP, Key::W);
bindings.set_secondary(GameAction::MOVE_UP, Key::UP);

// Save / load bindings to a config file
bindings.save("keybindings.cfg");
bindings.load("keybindings.cfg");

// Get a human-readable hint for display
std::string hint = bindings.get_key_hint(GameAction::MOVE_UP); // e.g. "W / Up"
```

### Available `GameAction` values

`MOVE_UP`, `MOVE_DOWN`, `MOVE_LEFT`, `MOVE_RIGHT`, `ACTION_1`, `ACTION_2`,
`HELP`, `MENU_UP`, `MENU_DOWN`, `MENU_LEFT`, `MENU_RIGHT`, `CONFIRM`,
`CANCEL`, `PAUSE`, `ZOOM_IN`, `ZOOM_OUT`, `SCROLL_MESSAGES_UP`,
`SCROLL_MESSAGES_DOWN`.

---

## 7. Rendering

### 7a. Console Backend

The console backend draws ASCII characters into a cell buffer, then flushes to
the terminal. On Windows it uses the Win32 Console API; on Linux/macOS it uses
ANSI escape codes.

#### Drawing primitives

```cpp
Console console(80, 30);

// Clear the screen
console.clear();

// Draw a single character
console.set_cell(5, 3, '@', Color::YELLOW, Color::BLACK);

// Draw a string
console.draw_string(0, 0, "Hello, World!", Color::WHITE);

// Draw a box
console.draw_box(10, 5, 20, 10, Color::CYAN);

// Fill a rectangle
console.fill_rect(12, 7, 16, 6, '.', Color::DARK_GREEN);

// Flush to terminal
console.present();
```

#### RGB color support

```cpp
RGBColor gold(255, 215, 0);
RGBColor dark_bg(20, 20, 30);

console.set_cell_rgb(5, 5, '@', gold, dark_bg);
console.draw_string_rgb(0, 0, "True color!", gold, dark_bg);
console.draw_box_rgb(0, 2, 30, 5, gold, dark_bg);
```

#### Using the `Renderer` class

`Renderer` wraps `Console` + `ComponentManager` and adds camera support and
automatic entity drawing:

```cpp
Renderer renderer(&console, &manager);

// Camera follows the player
renderer.center_camera_on_entity(player);

// Render the world tiles + all entities with PositionComponent + RenderComponent
renderer.render_all(&world);

// Or step-by-step:
console.clear();
renderer.render_map(&world);        // draw tiles
renderer.render_entities();           // draw entities sorted by priority
console.present();
```

For an infinite `ChunkedWorld`:

```cpp
renderer.render_all_chunked(&chunked_world);
```

Side panels (e.g. a stats sidebar) are supported:

```cpp
renderer.set_side_panel_width(20); // reserve 20 columns on the right
```

### 7b. Vulkan Backend

The Vulkan backend opens an SDL window and draws GPU-accelerated quads and text.

```cpp
#include "render/vulkan/vk_renderer.hpp"

VkRenderer renderer;
renderer.init("My Game", 1280, 720, false, true);
```

#### Drawing

```cpp
renderer.begin_frame(0.02f, 0.02f, 0.04f);     // clear color

// Solid colored rectangle
renderer.draw_rect({100, 100, 64, 64}, Color4::red());

// Textured sprite
VkDescriptorSet tex = renderer.load_texture("assets/player.png");
renderer.draw_sprite(tex, {200, 100, 64, 64});

// Tinted sprite with sub-region UVs
renderer.draw_sprite(tex, {300, 100, 32, 32},
                     {0.0f, 0.0f, 0.5f, 0.5f},   // top-left quarter
                     Color4::yellow());

// Text (built-in bitmap font)
renderer.draw_text(10, 10, "Score: 42", Color4::white(), 2.0f);

renderer.end_frame();
```

#### Camera

```cpp
Camera2D& cam = renderer.camera();

// Centre on a world position
cam.look_at(400.0f, 300.0f);

// Smooth follow (call every frame)
cam.lerp_to(target_x, target_y, 0.15f);

// Zoom
cam.zoom = 2.0f;   // 2× zoom in
cam.zoom *= 0.9f;  // zoom out a little

// Screen ↔ world conversion
float wx, wy;
cam.screen_to_world(mouse_x, mouse_y, wx, wy);

// Visible world bounds
float left   = cam.left();
float right  = cam.right();
float top    = cam.top();
float bottom = cam.bottom();
```

#### Color4 helper

```cpp
Color4 c1 = Color4::white();
Color4 c2 = Color4::from_hex(0xFF6600);
Color4 c3 = Color4::from_rgb(128, 0, 255);
Color4 c4 = { 1.0f, 0.5f, 0.0f, 0.8f };    // RGBA floats
```

---

## 8. UI Widgets

### 8a. Console Backend

The `UI` class provides ready-made interface elements for the console backend.
It wraps the `Console` and automatically handles word wrapping, colors, and
borders.

```cpp
UI ui(&console, &manager);

// Bordered panel
ui.draw_panel(0, 0, 30, 15, "Inventory", Color::WHITE);

// Word-wrapped text block
ui.draw_text(2, 2, 26, "A long description that wraps nicely.");

// Health bar
ui.draw_health_bar(2, 5, 20, /*current=*/7, /*max=*/10,
                   Color::GREEN, Color::RED);

// Scrollable message log
std::vector<std::string> msgs = { "You enter the cave.", "A bat screeches!" };
ui.draw_message_log(0, 20, 80, 5, msgs, /*scroll_offset=*/0);

// Interactive menu (returns selected index)
std::vector<std::string> options = { "New Game", "Load Game", "Quit" };
int selected = ui.draw_menu(10, 5, "Main Menu", options, current_selection);

// Yes/No confirmation
bool yes = ui.confirm_action("Are you sure?");

// Entity selection dialog
EntityId choice = ui.show_entity_selection(entity_list, render_background_fn);
```

### 8b. Vulkan Backend — Immediate-Mode UI (`VkUI`)

The Vulkan backend ships with a full **immediate-mode UI system** built on top
of the sprite batch renderer. There is no retained widget tree — you call
functions each frame to describe your UI, and the system handles layout, input,
and drawing automatically.

```cpp
#include "render/vulkan/vk_renderer.hpp"   // also includes vk_ui.hpp
```

#### Lifecycle

Wrap all UI calls between `begin()` / `end()` each frame, after
`begin_frame()` and before `end_frame()`:

```cpp
VkRenderer renderer;
renderer.init("My Game", 1280, 720);

VkUI ui;

while (renderer.is_running()) {
    Key key = renderer.poll_input();

    renderer.begin_frame(0.05f, 0.05f, 0.1f);

    // ---- UI goes here ----
    ui.begin(renderer);

    if (ui.button("Play"))  { start_game(); }
    if (ui.button("Quit"))  { break; }

    ui.end();
    // ---- End UI ----

    renderer.end_frame();
}
```

`begin()` captures the current mouse state from SDL every frame. `end()` draws
deferred elements (tooltips, modal dim overlay) and finalizes hot/active widget
tracking.

> **Screen-space rendering:** All UI coordinates are in screen pixels
> (top-left = 0,0). The system converts to world-space internally so your UI
> stays fixed on screen even when the camera moves or zooms.

#### Windows

Windows provide a draggable, closable container with automatic vertical layout:

```cpp
// begin_window returns false if the user clicked the close button
if (ui.begin_window("Inventory", { 20, 20, 300, 400 })) {
    ui.label("Gold: 1234");
    ui.separator();

    if (ui.button("Sort Items")) { /* ... */ }

    ui.end_window();
}
```

**Window flags** modify behavior:

| Flag | Effect |
|------|--------|
| `WindowFlags::None` | Default: title bar + close button |
| `WindowFlags::NoTitleBar` | No title bar at all |
| `WindowFlags::NoClose` | Title bar visible but no close button |
| `WindowFlags::Modal` | Dims background, blocks input to other windows |
| `WindowFlags::Centered` | Centers the window on screen |

```cpp
// A modal dialog
if (show_confirm) {
    if (ui.begin_window("Quit?", { 0, 0, 300, 120 },
                        WindowFlags::Modal | WindowFlags::Centered | WindowFlags::NoClose))
    {
        ui.label("Are you sure you want to quit?");
        if (ui.button("Yes"))  { running = false; }
        ui.same_line();
        if (ui.button("No"))   { show_confirm = false; }
        ui.end_window();
    }
}
```

#### Widgets

All widgets work both standalone (with explicit `Rect2D` bounds) and inside
windows (auto-laid-out vertically, full window width).

| Widget | Usage | Returns |
|--------|-------|---------|
| `label(text)` | Static text (auto-layout) | — |
| `label(x, y, text)` | Text at absolute position | — |
| `label(bounds, text)` | Text inside a rect | — |
| `button(text)` | Clickable button (auto-layout) | `true` the frame it is clicked |
| `button(text, bounds)` | Button at explicit bounds | `true` when clicked |
| `selectable(text, selected)` | Highlighted row (auto-layout) | `true` when clicked |
| `progress_bar(fraction)` | Horizontal bar 0.0–1.0 (auto-layout) | — |
| `progress_bar(bounds, fraction)` | Bar at explicit bounds | — |
| `separator()` | Horizontal line | — |
| `same_line(spacing)` | Place next widget to the right, not below | — |
| `tooltip(text)` | Tooltip at mouse position (deferred to `end()`) | — |
| `message_log(bounds, msgs)` | Scrollable message list | — |

**Example — Character stats panel:**

```cpp
if (ui.begin_window("Stats", { 20, 20, 260, 200 })) {
    ui.label("Level 5 Warrior");
    ui.separator();

    ui.label("HP");
    ui.progress_bar(hp / max_hp, Color4::from_hex(0xc02020));
    if (ui.hover()) {
        char tip[64];
        snprintf(tip, sizeof(tip), "%.0f / %.0f HP", hp, max_hp);
        ui.tooltip(tip);
    }

    ui.label("MP");
    ui.progress_bar(mp / max_mp, Color4::from_hex(0x2040c0));

    ui.end_window();
}
```

**Example — Inventory with side-by-side buttons:**

```cpp
if (ui.begin_window("Inventory", { 300, 20, 280, 350 })) {
    for (int i = 0; i < (int)items.size(); ++i) {
        bool clicked = ui.selectable(items[i].name, i == selected);
        if (clicked) selected = i;
        if (ui.hover()) ui.tooltip(items[i].description);
    }

    ui.separator();

    if (selected >= 0) {
        if (ui.button("Use"))  { use_item(selected); }
        ui.same_line();
        if (ui.button("Drop")) { drop_item(selected); }
    }

    ui.end_window();
}
```

#### Querying Widget State

After any widget call, you can check:

```cpp
ui.hover()                 // Was the last widget hovered by the mouse?
ui.clicked()               // Was the last widget clicked this frame?
ui.any_window_hovered()    // Is ANY window currently hovered? (block game input)
ui.mouse()                 // Current UIMouseState (x, y, down, pressed, released)
```

A common pattern for blocking game input when the UI is active:

```cpp
if (!ui.any_window_hovered()) {
    // Handle game-world clicks / movement
}
```

#### Theming with `UIStyle`

`UIStyle` controls every color and dimension in the UI. Push/pop styles to
change the theme at any point:

```cpp
UIStyle ocean;
ocean.window_bg       = Color4::from_hex(0x0c1628);
ocean.window_border   = Color4::from_hex(0x2060a0);
ocean.window_title_bg = Color4::from_hex(0x102040);
ocean.button_bg       = Color4::from_hex(0x152040);
ocean.button_hover    = Color4::from_hex(0x203060);
ocean.button_active   = Color4::from_hex(0x3050a0);
ocean.text_color      = Color4::from_hex(0xc0d8f0);
ocean.progress_fill   = Color4::from_hex(0x2070c0);
// ... (all fields are public, set as many as you need)

// Apply globally
ui.push_style(ocean);
// ... all subsequent widgets use the ocean theme ...
ui.pop_style();
```

**`UIStyle` fields:**

| Category | Fields |
|----------|--------|
| Window | `window_bg`, `window_border`, `window_title_bg`, `window_title_fg` |
| Button | `button_bg`, `button_hover`, `button_active`, `button_fg` |
| Selectable | `selectable_bg`, `selectable_hover`, `selectable_selected`, `selectable_fg` |
| Progress | `progress_bg`, `progress_fill` |
| General | `text_color`, `separator_color`, `tooltip_bg`, `tooltip_fg`, `modal_dim`, `close_btn_fg`, `close_btn_hover` |
| Sizing | `padding`, `item_spacing`, `border_width`, `title_bar_height`, `text_scale`, `button_height`, `scrollbar_width` |

#### Textured Skins with `UISkin` (9-Slice)

For a fully custom look, load a spritesheet and define 9-slice patches.
Wherever a `NineSlice` is set, it replaces the flat color:

```cpp
UISkin skin;

VkDescriptorSet atlas = renderer.load_texture("assets/ui_atlas.png");

// Define 9-slice for the window frame
skin.window_frame.texture = atlas;
skin.window_frame.uv      = { 0.0f, 0.0f, 0.25f, 0.25f };  // region in atlas
skin.window_frame.bl       = 8;   // border left (pixels in source)
skin.window_frame.bt       = 8;   // border top
skin.window_frame.br       = 8;   // border right
skin.window_frame.bb       = 8;   // border bottom

// Similar for buttons, tooltips, etc.
skin.button_normal.texture = atlas;
skin.button_normal.uv      = { 0.25f, 0.0f, 0.5f, 0.125f };
skin.button_normal.bl = skin.button_normal.bt = 4;
skin.button_normal.br = skin.button_normal.bb = 4;

ui.set_skin(skin);
// ... all widgets now use 9-slice textures ...
ui.clear_skin();   // revert to flat colors
```

The 9-slice system stretches the center of the patch to fit any size while
keeping corners and edges pixel-perfect — ideal for window frames, buttons,
and panels.

**Skinnable elements in `UISkin`:**

| Field | Used by |
|-------|---------|
| `window_frame` | Window background |
| `window_title` | Window title bar |
| `button_normal` / `button_hover` / `button_pressed` | Button states |
| `selectable_hover` / `selectable_selected` | Selectable states |
| `progress_bg` / `progress_fill` | Progress bar |
| `tooltip` | Tooltip background |
| `checkbox_on` / `checkbox_off` | Checkboxes (Sprite) |
| `slider_track` / `slider_thumb` | Sliders (Sprite) |
| `close_button` | Window close button (Sprite) |

#### Custom Draw Callbacks

For full control over how a widget looks, register a draw callback. Your
function receives the renderer, the widget bounds (in world-space, ready for
`draw_rect`), and the widget state:

```cpp
// Animated pulsing button background
ui.set_custom_draw("button", [&](VkRenderer& r, Rect2D bounds, WidgetState state) {
    float pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
    Color4 bg = (state == WidgetState::Hovered)
        ? Color4{ 0.2f + pulse * 0.3f, 0.1f, 0.4f, 1.0f }
        : Color4{ 0.15f, 0.1f, 0.25f, 1.0f };
    r.draw_rect(bounds, bg);
});

// Valid callback names: "window_bg", "button", "selectable", "progress", "tooltip"

// Clear a specific callback
ui.set_custom_draw("button", nullptr);
```

The three skinning layers are checked in order: **custom draw callback** →
**9-slice texture** → **flat color**. The first one found is used.

#### Message Log

Display a scrollable list of messages in a fixed region:

```cpp
std::vector<std::string> messages = {
    "You enter the dungeon.",
    "A goblin appears!",
    "You strike the goblin for 5 damage.",
};

// Show up to 6 visible lines, most recent at the bottom
ui.message_log({ 20, 550, 400, 120 }, messages, /*max_visible=*/6);
```

#### Complete Vulkan UI Example

Here is a condensed game loop showing typical UI usage:

```cpp
#include "render/vulkan/vk_renderer.hpp"
#include <cstdio>

int main() {
    VkRenderer renderer;
    renderer.init("UI Example", 1280, 720);

    VkUI ui;
    bool show_inventory = true;
    float hp = 75, max_hp = 100;
    int selected = -1;
    std::vector<std::string> items = { "Sword", "Shield", "Potion" };
    std::vector<std::string> log = { "Welcome!" };

    while (renderer.is_running()) {
        Key key = renderer.poll_input();
        if (key == Key::ESCAPE) break;
        if (key == Key::I) show_inventory = !show_inventory;

        renderer.begin_frame(0.05f, 0.05f, 0.1f);

        ui.begin(renderer);

        // --- HUD bar ---
        ui.label(10, 10, "HP:");
        ui.progress_bar({ 50, 8, 200, 20 }, hp / max_hp,
                        Color4::from_hex(0xc02020));

        // --- Inventory window ---
        if (show_inventory) {
            if (ui.begin_window("Inventory", { 20, 50, 260, 300 })) {
                for (int i = 0; i < (int)items.size(); ++i) {
                    if (ui.selectable(items[i], i == selected))
                        selected = i;
                }
                ui.separator();
                if (selected >= 0 && ui.button("Use")) {
                    log.push_back("Used " + items[selected]);
                }
                ui.end_window();
            } else {
                show_inventory = false;  // user closed it
            }
        }

        // --- Message log ---
        ui.message_log({ 20, 600, 400, 100 }, log, 5);

        // --- Block game input when hovering UI ---
        if (!ui.any_window_hovered()) {
            // handle world clicks, movement, etc.
        }

        ui.end();

        renderer.end_frame();
    }

    renderer.destroy();
    return 0;
}
```

See [demos/vk_ui_demo.cpp](demos/vk_ui_demo.cpp) for a comprehensive showcase
with multiple themes, animated custom draws, modal dialogs, and a full
inventory system.

---

## 9. Scenes & Scene Manager

Scenes let you split your game into logical states: title screen, gameplay,
inventory, game over, etc.

### Defining a scene

Subclass `Scene` and override `on_enter()`, `handle_input()`, `update()`, and
`render()`:

```cpp
#include "engine/scene/scene.hpp"

class GameplayScene : public Scene {
private:
    World world;
    EntityId player_id;

public:
    GameplayScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : Scene(con, mgr, rend, ui_sys), world(60, 30) {}

    void on_enter() override {
        // Called when the scene becomes active.
        // Set up the world, spawn entities, etc.
        manager->clear_all_entities();

        player_id = manager->create_entity();
        manager->add_component<PositionComponent>(player_id, 5, 5, 0);
        manager->add_component<RenderComponent>(player_id, '@', "yellow", "", 100);

        // ... populate world tiles ...
    }

    void handle_input(Key key) override {
        auto& bindings = KeyBindings::instance();
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);

        int dx = 0, dy = 0;
        if (bindings.is_action(key, GameAction::MOVE_UP))    dy = -1;
        if (bindings.is_action(key, GameAction::MOVE_DOWN))  dy =  1;
        if (bindings.is_action(key, GameAction::MOVE_LEFT))  dx = -1;
        if (bindings.is_action(key, GameAction::MOVE_RIGHT)) dx =  1;

        if (dx || dy) {
            int nx = pos->x + dx, ny = pos->y + dy;
            if (world.get_tile(nx, ny).walkable) {
                pos->x = nx;
                pos->y = ny;
            }
        }

        if (bindings.is_action(key, GameAction::CANCEL)) {
            change_scene("title");       // transition to a different scene
        }
    }

    void update() override {
        // Called each frame. AI ticks, physics, timers, etc.
    }

    void render() override {
        renderer->center_camera_on_entity(player_id);
        renderer->render_all(&world);
        ui->draw_message_log(0, 25, 80, 5, messages);
        renderer->present();
    }
};
```

### Registering and running scenes

```cpp
Console console(80, 30);
ComponentManager manager;
Renderer renderer(&console, &manager);
UI ui(&console, &manager);
SceneManager scene_manager;

// Register scene factories (lazy creation)
scene_manager.register_scene<GameplayScene>("gameplay",
    [&]() -> std::unique_ptr<Scene> {
        return std::make_unique<GameplayScene>(&console, &manager, &renderer, &ui);
    });

scene_manager.register_scene<TitleScene>("title",
    [&]() -> std::unique_ptr<Scene> {
        return std::make_unique<TitleScene>(&console, &manager, &renderer, &ui);
    });

// Start with the title screen
scene_manager.load_scene("title");

// Main loop
while (scene_manager.has_active_scene()) {
    Key key = Input::get_key();
    scene_manager.handle_input(key);
    scene_manager.update();            // calls current_scene->update()
    scene_manager.render();            // calls current_scene->render()
    SLEEP_MS(16);
}
```

### Scene transitions

From inside a scene:

```cpp
change_scene("gameplay");           // transition (cached if persistent)
change_to_fresh_scene("gameplay");  // transition, destroying cached state
exit_application();                 // stop the game loop
```

### Persistent scenes

By default, every scene is recreated each time it is loaded. To keep a scene
alive (preserving its state between visits):

```cpp
scene_manager.set_persistent("gameplay");
```

### Sharing data between scenes

Use `user_data` to pass a shared game state pointer:

```cpp
struct GameData {
    int score = 0;
    int level = 1;
};

GameData game_data;
scene_manager.set_user_data(&game_data);

// Inside any scene:
GameData* gd = static_cast<GameData*>(user_data);
gd->score += 10;
```

---

## 10. Procedural Generation with Perlin Noise

The engine includes a 2D Perlin noise generator for terrain, biomes, caves, etc.

```cpp
#include "engine/world/perlin_noise.hpp"

PerlinNoise perlin(42);   // seed

// Single sample
double val = perlin.noise(3.5, 7.2);              // raw noise
double fbm = perlin.octave_noise(3.5, 7.2, 4);    // 4-octave fractal

// Generate a full map (values roughly in [-1, 1])
auto height_map = perlin.generate_map(60, 30, /*scale=*/0.1, /*octaves=*/4);

// Or normalized to [0, 1]
auto norm_map = perlin.generate_normalized_map(60, 30, 0.1, 4, 0.5);
```

### Example: terrain generation

```cpp
PerlinNoise elevation(seed);
PerlinNoise moisture(seed + 1);
World world(60, 30);

for (int y = 0; y < 30; ++y) {
    for (int x = 0; x < 60; ++x) {
        double e = elevation.octave_noise(x * 0.08, y * 0.08, 4);
        double m = moisture.octave_noise(x * 0.05, y * 0.05, 3);

        Tile& tile = world.get_tile(x, y);

        if (e < -0.2) {
            tile = Tile(false, '~', "blue");          // water
        } else if (e < 0.0) {
            tile = Tile(true, '.', "yellow");          // sand
        } else if (e < 0.4) {
            tile = Tile(true, m > 0 ? '"' : '.', "green"); // grass / forest
        } else {
            tile = Tile(false, '^', "gray");           // mountain
        }
    }
}
```

---

## 11. Infinite Worlds with ChunkedWorld

`ChunkedWorld` divides the world into fixed-size chunks loaded on demand,
enabling infinite exploration.

```cpp
#include "engine/world/chunk.hpp"

ChunkedWorld chunked_world(/*seed=*/12345);

// Access a tile at any world coordinate — the chunk is created automatically
Tile& tile = chunked_world.get_tile(500, -200);

// Check walkability
if (chunked_world.is_walkable(10, 20)) { /* move there */ }

// Ensure chunks around the player are loaded
chunked_world.ensure_chunks_loaded(player_x, player_y, /*radius=*/3);

// Unload distant chunks to save memory
chunked_world.unload_distant_chunks(player_x, player_y, /*max_distance=*/10);

// Render with the console Renderer
renderer.render_all_chunked(&chunked_world);
```

### Multi-chunk structures

Plan large structures (dungeons, towns) that span multiple chunks:

```cpp
int struct_id = chunked_world.plan_structure(
    /*world_x=*/100, /*world_y=*/50,
    /*width=*/32, /*height=*/24,
    /*type=*/1,
    /*seed=*/9999,
    /*structure_id=*/"town_01"
);

// During chunk generation, query planned structures:
auto* planned = chunked_world.get_structures_for_chunk(chunk_x, chunk_y);
for (auto* ps : planned) {
    if (!chunked_world.is_structure_generated(ps->id)) {
        // ... build the structure tiles ...
        chunked_world.mark_structure_generated(ps->id);
    }
}
```

---

## 12. Utilities (JSON, RNG)

### JSON

A lightweight recursive-descent JSON parser/serializer with `//` comment
support.

```cpp
#include "engine/util/json.hpp"

// Parse from file
json::Value config = json::parse_file("config.json");

// Access fields
std::string name = config["name"].as_string();
int level = config["level"].as_int();
bool hard_mode = config["difficulty"].get_bool(false);  // default if missing

// Iterate arrays
for (auto& item : config["inventory"].as_array()) {
    std::string item_name = item["name"].as_string();
    int count = item["count"].as_int();
}

// Build JSON
json::Object save_data;
save_data["player_x"] = 42;
save_data["player_y"] = 17;
save_data["health"]   = 80.5;
save_data["name"]     = "Hero";
save_data["items"]    = json::Array{ "sword", "shield" };

// Write to file (pretty-printed)
json::write_file("save.json", json::Value(save_data), /*indent=*/2);

// Or convert to string
std::string json_str = json::stringify(json::Value(save_data), 2);
```

### RNG

`GameRNG` is a singleton Mersenne Twister with convenience functions:

```cpp
#include "engine/util/rng.hpp"

GameRNG::instance().seed(42);

int damage = random_int(5, 15);
float spawn_chance = random_float(0.0f, 1.0f);
bool crit = chance(0.15f);                   // 15% chance
int attack_roll = roll_dice(2, 6);            // 2d6
size_t idx = random_index(items.size());      // random valid index
```

---

## 13. Putting It All Together — A Complete Mini-Game

Below is a full, self-contained console game you can compile and run. It
includes a title screen, gameplay with enemies and items, and a game-over
screen — all wired through the scene manager.

```cpp
// my_game.cpp — A minimal roguelike built with Console Engine

#include "engine/render/console.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/ui.hpp"
#include "engine/input/input.hpp"
#include "engine/ecs/component_manager.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/scene/scene_manager.hpp"
#include "engine/world/world.hpp"
#include "engine/world/perlin_noise.hpp"
#include "engine/util/rng.hpp"
#include <memory>
#include <vector>
#include <string>

#ifdef _WIN32
  #include <windows.h>
  #define SLEEP_MS(ms) Sleep(ms)
#else
  #include <unistd.h>
  #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

// ---- Custom Components ----

struct HealthComponent : Component {
    int current, maximum;
    HealthComponent(int hp = 10, int max_hp = 10) : current(hp), maximum(max_hp) {}
    bool is_dead() const { return current <= 0; }
    IMPLEMENT_CLONE(HealthComponent)
};

struct NameComponent : Component {
    std::string name;
    NameComponent(const std::string& n = "") : name(n) {}
    IMPLEMENT_CLONE(NameComponent)
};

struct EnemyTag : Component {
    IMPLEMENT_CLONE(EnemyTag)
};

// ---- Shared game state ----

struct GameState {
    int score = 0;
    std::vector<std::string> log;

    void add_message(const std::string& msg) {
        log.push_back(msg);
        if (log.size() > 50) log.erase(log.begin());
    }
};

// ============================================================================
// Title Screen
// ============================================================================

class TitleScene : public Scene {
public:
    using Scene::Scene;

    void on_enter() override {}

    void handle_input(Key key) override {
        auto& b = KeyBindings::instance();
        if (b.is_action(key, GameAction::CONFIRM) || key == Key::ENTER) {
            change_to_fresh_scene("gameplay");
        }
        if (b.is_action(key, GameAction::CANCEL)) {
            exit_application();
        }
    }

    void update() override {}

    void render() override {
        console->clear();
        ui->draw_panel(15, 5, 50, 15, "DUNGEON CRAWL", Color::YELLOW);
        ui->draw_text(20, 9, 40, "A tiny roguelike made with Console Engine.");
        ui->draw_text(20, 12, 40, "Press ENTER to start");
        ui->draw_text(20, 14, 40, "Press ESC to quit");
        console->present();
    }
};

// ============================================================================
// Gameplay Scene
// ============================================================================

class GameplayScene : public Scene {
private:
    World world;
    EntityId player_id = 0;
    GameState* gs = nullptr;

    void generate_world() {
        PerlinNoise noise(random_int(0, 99999));
        for (int y = 0; y < world.get_height(); ++y) {
            for (int x = 0; x < world.get_width(); ++x) {
                double v = noise.octave_noise(x * 0.1, y * 0.1, 4);
                Tile& t = world.get_tile(x, y);
                if (v < -0.2) {
                    t = Tile(false, '~', "blue");
                } else if (v > 0.5) {
                    t = Tile(false, '#', "gray");
                } else {
                    t = Tile(true, '.', "green");
                }
            }
        }
    }

    void spawn_enemies(int count) {
        for (int i = 0; i < count; ++i) {
            int ex, ey;
            do {
                ex = random_int(1, world.get_width() - 2);
                ey = random_int(1, world.get_height() - 2);
            } while (!world.get_tile(ex, ey).walkable);

            EntityId e = manager->create_entity();
            manager->add_component<PositionComponent>(e, ex, ey, 0);
            manager->add_component<RenderComponent>(e, 'g', "red", "", 80);
            manager->add_component<HealthComponent>(e, 5, 5);
            manager->add_component<NameComponent>(e, "Goblin");
            manager->add_component<CollisionComponent>(e, true, false);
            manager->add_component<EnemyTag>(e);
        }
    }

public:
    GameplayScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* u)
        : Scene(con, mgr, rend, u), world(80, 25) {}

    void on_enter() override {
        gs = static_cast<GameState*>(user_data);
        gs->score = 0;
        gs->log.clear();
        manager->clear_all_entities();

        generate_world();

        // Spawn player
        player_id = manager->create_entity();
        manager->add_component<PositionComponent>(player_id, 5, 5, 0);
        manager->add_component<RenderComponent>(player_id, '@', "yellow", "", 100);
        manager->add_component<HealthComponent>(player_id, 20, 20);
        manager->add_component<NameComponent>(player_id, "Hero");
        manager->add_component<PlayerControllerComponent>(player_id, true, true);

        spawn_enemies(8);

        gs->add_message("You enter the dungeon. Good luck!");
    }

    void handle_input(Key key) override {
        auto& bindings = KeyBindings::instance();
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;

        int dx = 0, dy = 0;
        if (bindings.is_action(key, GameAction::MOVE_UP))    dy = -1;
        if (bindings.is_action(key, GameAction::MOVE_DOWN))  dy = 1;
        if (bindings.is_action(key, GameAction::MOVE_LEFT))  dx = -1;
        if (bindings.is_action(key, GameAction::MOVE_RIGHT)) dx = 1;

        if (dx != 0 || dy != 0) {
            int nx = pos->x + dx, ny = pos->y + dy;
            if (nx < 0 || ny < 0 || nx >= world.get_width() || ny >= world.get_height()) return;
            if (!world.get_tile(nx, ny).walkable) return;

            // Check for enemy at destination (bump-to-attack)
            EntityId enemy = manager->get_entity_at<EnemyTag>(nx, ny, player_id);
            if (enemy) {
                auto* hp = manager->get_component<HealthComponent>(enemy);
                auto* name = manager->get_component<NameComponent>(enemy);
                int dmg = random_int(2, 5);
                hp->current -= dmg;
                gs->add_message("You hit " + name->name + " for " + std::to_string(dmg) + "!");

                if (hp->is_dead()) {
                    gs->score += 10;
                    gs->add_message(name->name + " is slain! (+10 pts)");
                    manager->destroy_entity(enemy);
                }
            } else {
                pos->x = nx;
                pos->y = ny;
            }
        }

        if (bindings.is_action(key, GameAction::CANCEL)) {
            change_scene("title");
        }
    }

    void update() override {
        // Simple enemy AI — chase the player
        auto* player_pos = manager->get_component<PositionComponent>(player_id);
        auto enemies = manager->get_entities_with_component<EnemyTag>();

        for (EntityId eid : enemies) {
            auto* epos = manager->get_component<PositionComponent>(eid);
            if (!epos) continue;

            int dist = epos->manhattan_distance(*player_pos);
            if (dist > 8) continue; // only chase if nearby

            int dx = 0, dy = 0;
            if (player_pos->x > epos->x) dx = 1;
            else if (player_pos->x < epos->x) dx = -1;
            if (player_pos->y > epos->y) dy = 1;
            else if (player_pos->y < epos->y) dy = -1;

            // Prefer the axis with greater distance
            int adx = std::abs(player_pos->x - epos->x);
            int ady = std::abs(player_pos->y - epos->y);

            int nx, ny;
            if (adx >= ady) { nx = epos->x + dx; ny = epos->y; }
            else             { nx = epos->x;       ny = epos->y + dy; }

            // Adjacent to player? Attack instead of moving
            if (std::abs(nx - player_pos->x) + std::abs(ny - player_pos->y) == 0) {
                auto* php = manager->get_component<HealthComponent>(player_id);
                int dmg = random_int(1, 3);
                php->current -= dmg;
                gs->add_message("Goblin hits you for " + std::to_string(dmg) + "!");

                if (php->is_dead()) {
                    change_scene("gameover");
                    return;
                }
                continue;
            }

            if (world.get_tile(nx, ny).walkable &&
                !manager->is_position_blocked(nx, ny, eid)) {
                epos->x = nx;
                epos->y = ny;
            }
        }
    }

    void render() override {
        renderer->center_camera_on_entity(player_id);
        renderer->render_all(&world);

        // HUD
        auto* hp = manager->get_component<HealthComponent>(player_id);
        console->draw_string(0, world.get_height(),
            "HP: " + std::to_string(hp->current) + "/" + std::to_string(hp->maximum) +
            "  Score: " + std::to_string(gs->score), Color::WHITE);

        // Message log
        ui->draw_message_log(0, world.get_height() + 1, console->get_width(), 4, gs->log);

        renderer->present();
    }
};

// ============================================================================
// Game Over Scene
// ============================================================================

class GameOverScene : public Scene {
public:
    using Scene::Scene;

    void on_enter() override {}

    void handle_input(Key key) override {
        if (key == Key::ENTER) change_to_fresh_scene("gameplay");
        if (key == Key::ESCAPE) exit_application();
    }

    void update() override {}

    void render() override {
        GameState* gs = static_cast<GameState*>(user_data);
        console->clear();
        ui->draw_panel(20, 8, 40, 10, "GAME OVER", Color::RED);
        ui->draw_text(25, 11, 30,
            "Final score: " + std::to_string(gs->score));
        ui->draw_text(25, 14, 30, "ENTER = retry   ESC = quit");
        console->present();
    }
};

// ============================================================================
// Main
// ============================================================================

int main() {
    GameRNG::instance().seed(42);

    Console console(80, 30);
    ComponentManager manager;
    Renderer renderer(&console, &manager);
    UI ui(&console, &manager);
    SceneManager scenes;

    GameState game_state;
    scenes.set_user_data(&game_state);

    scenes.register_scene<TitleScene>("title",
        [&]() { return std::make_unique<TitleScene>(&console, &manager, &renderer, &ui); });
    scenes.register_scene<GameplayScene>("gameplay",
        [&]() { return std::make_unique<GameplayScene>(&console, &manager, &renderer, &ui); });
    scenes.register_scene<GameOverScene>("gameover",
        [&]() { return std::make_unique<GameOverScene>(&console, &manager, &renderer, &ui); });

    scenes.load_scene("title");

    while (scenes.has_active_scene()) {
        Key key = Input::get_key();
        scenes.handle_input(key);
        scenes.update();
        scenes.render();
        SLEEP_MS(16);
    }

    return 0;
}
```

### Building the example

Add to `CMakeLists.txt`:

```cmake
add_executable(my_game my_game.cpp ${ENGINE_SOURCES})
target_include_directories(my_game PRIVATE ${CMAKE_SOURCE_DIR})
if(MSVC)
    target_compile_options(my_game PRIVATE /W4 /EHsc /utf-8)
endif()
```

Then build:

```bash
cmake -B build
cmake --build build --config Release
```

---

## 14. Next Steps

Now that you know the fundamentals, here are ideas to explore:

- **Add more component types** — `InventoryComponent`, `AIComponent`,
  `StatusEffectComponent` — and write systems that query them.
- **Save/load games** using the `json` utility to serialize entity state.
- **Use `ChunkedWorld`** with custom chunk generators and Perlin noise for
  infinite open worlds.
- **Switch to the Vulkan backend** for textures, smooth camera, and GPU
  rendering. See [demos/vk_game.cpp](demos/vk_game.cpp) for a complete
  roguelike built on `VkRenderer`.
- **Build a custom UI** using `VkUI` — create themed windows, health bars,
  inventory screens, and modal dialogs. Load a spritesheet with `UISkin`
  for a fully textured look. See [demos/vk_ui_demo.cpp](demos/vk_ui_demo.cpp)
  for a full showcase.
- **Add sound** by integrating SDL_mixer (SDL2 is already a dependency when
  building with Vulkan).
- **Build multi-floor dungeons with z-levels** — stack 5, 10, or more
  floors with stairs, ramps, transparent tiles, and per-floor enemies.
  See [demos/zlevel_game.cpp](demos/zlevel_game.cpp) for a complete example.
- **Implement A\* pathfinding** using the tile `walkable` and `movement_cost`
  fields.
- **Customize key bindings** — ship a `keybindings.cfg` and let players
  remap controls at runtime.

Happy building!
