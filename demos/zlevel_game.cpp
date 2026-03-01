// ============================================================================
// Depths of the Keep — A z-level showcase game
// ============================================================================
// A mini roguelike that demonstrates the engine's 3D z-level system.
//
// 5 z-levels:
//   Z=0  Dragon's Lair    — lava, the dragon, the treasure
//   Z=1  Catacombs         — twisting passages, skeletons
//   Z=2  Dungeon           — cells, rats, locked door, key
//   Z=3  Keep Ground Floor — great hall, merchant, stairs
//   Z=4  Tower Roof        — parapet, overview, banner
//
// Controls:  WASD / arrows  — move        </> — change z-level (stairs)
//            E              — interact     ESC — quit
// ============================================================================

#include "engine/render/console.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/ui.hpp"
#include "engine/input/input.hpp"
#include "engine/ecs/component_manager.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/scene/scene_manager.hpp"
#include "engine/world/world.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

// ── Constants ───────────────────────────────────────────────────────────────
static constexpr int CON_W        = 100;
static constexpr int CON_H        = 40;
static constexpr int PANEL_W      = 24;
static constexpr int MAP_W        = 76;   // CON_W - PANEL_W
static constexpr int MSG_H        = 6;
static constexpr int MAP_H        = CON_H - MSG_H;  // 34
static constexpr int WORLD_W      = 80;
static constexpr int WORLD_H      = 50;
static constexpr int WORLD_DEPTH  = 5;

// ── Level names ─────────────────────────────────────────────────────────────
static const char* LEVEL_NAMES[] = {
    "Dragon's Lair",
    "Catacombs",
    "Dungeon",
    "Keep Ground Floor",
    "Tower Roof"
};

// ── Enemy / item tracking ───────────────────────────────────────────────────
struct EnemyInfo {
    EntityId id       = 0;
    std::string name;
    int hp            = 1;
    int max_hp        = 1;
    int attack        = 1;
    int sight_range   = 6;
    bool alive        = true;
};

struct ItemInfo {
    EntityId id       = 0;
    std::string type; // "potion", "key", "treasure"
    bool collected    = false;
};

// ── Forward declarations ────────────────────────────────────────────────────
class TitleScene;
class GameScene;
class VictoryScene;
class DeathScene;

// ════════════════════════════════════════════════════════════════════════════
//  TITLE SCENE
// ════════════════════════════════════════════════════════════════════════════
class TitleScene : public Scene {
public:
    TitleScene(Console* c, ComponentManager* m, Renderer* r, UI* u) : Scene(c, m, r, u) {}

    void on_enter() override {}

    void handle_input(Key key) override {
        auto& kb = KeyBindings::instance();
        if (kb.is_action(key, GameAction::CONFIRM) || key == Key::ENTER || key == Key::SPACE) {
            change_to_fresh_scene("game");
        } else if (kb.is_action(key, GameAction::CANCEL)) {
            exit_application();
        }
    }

    void update() override {}

    void render() override {
        console->clear();

        // ── ASCII banner ────────────────────────────────────────────────────
        const std::vector<std::string> title = {
            R"( ____   _____ ____ _____ _   _ ____     ___  _____   _____ _   _ _____   _  _______ _____ ____  )",
            R"(|  _ \ | ____|  _ \_   _| | | / ___|   / _ \|  ___| |_   _| | | | ____| | |/ / ____| ____|  _ \ )",
            R"(| | | ||  _| | |_) || | | |_| \___ \  | | | | |_      | | | |_| |  _|   | ' /|  _| |  _| | |_) |)",
            R"(| |_| || |___|  __/ | | |  _  |___) | | |_| |  _|     | | |  _  | |___  | . \| |___| |___|  __/ )",
            R"(|____/ |_____|_|    |_| |_| |_|____/   \___/|_|       |_| |_| |_|_____| |_|\_\_____|_____|_|    )",
        };

        int ty = 4;
        for (auto& line : title) {
            int tx = (CON_W - (int)line.size()) / 2;
            if (tx < 0) tx = 0;
            console->draw_string(tx, ty++, line, Color::RED, Color::BLACK);
        }

        // ── Subtitle & instructions ─────────────────────────────────────────
        auto center = [&](int y, const std::string& s, Color fg) {
            int x = (CON_W - (int)s.size()) / 2;
            console->draw_string(x, y, s, fg, Color::BLACK);
        };

        center(12, "A  Z - L E V E L   S H O W C A S E", Color::YELLOW);
        center(15, "Descend five floors beneath an ancient keep.", Color::GRAY);
        center(16, "Slay monsters. Find the key. Claim the dragon's treasure.", Color::GRAY);
        center(17, "Return alive to the tower roof to win.", Color::GRAY);

        center(20, "CONTROLS", Color::WHITE);
        center(22, "WASD / Arrows ... Move          < / > ... Ascend / Descend", Color::DARK_CYAN);
        center(23, "E ............... Interact       ESC ..... Quit", Color::DARK_CYAN);

        center(26, "Each floor has its own dangers and rewards.", Color::DARK_YELLOW);
        center(27, "Bump into enemies to attack. Walk over items to pick up.", Color::DARK_YELLOW);

        center(31, "[ PRESS  ENTER  TO  BEGIN ]", Color::WHITE);
        center(33, "Press ESC to quit", Color::DARK_GRAY);

        console->present();
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  VICTORY SCENE
// ════════════════════════════════════════════════════════════════════════════
class VictoryScene : public Scene {
public:
    VictoryScene(Console* c, ComponentManager* m, Renderer* r, UI* u) : Scene(c, m, r, u) {}
    void on_enter() override {}
    void handle_input(Key key) override {
        if (key != Key::NONE) exit_application();
    }
    void update() override {}
    void render() override {
        console->clear();
        auto center = [&](int y, const std::string& s, Color fg) {
            console->draw_string((CON_W - (int)s.size()) / 2, y, s, fg, Color::BLACK);
        };
        center(10, "========================================", Color::YELLOW);
        center(12, "   V I C T O R Y !   ", Color::YELLOW);
        center(14, "========================================", Color::YELLOW);
        center(17, "You escaped the keep with the dragon's treasure!", Color::WHITE);
        center(18, "The kingdom hails you as a hero.", Color::GREEN);
        center(22, "Thanks for playing Depths of the Keep", Color::GRAY);
        center(23, "A z-level showcase for ConsoleEngine", Color::DARK_GRAY);
        center(27, "[ Press any key to exit ]", Color::DARK_GRAY);
        console->present();
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  DEATH SCENE
// ════════════════════════════════════════════════════════════════════════════
class DeathScene : public Scene {
public:
    DeathScene(Console* c, ComponentManager* m, Renderer* r, UI* u) : Scene(c, m, r, u) {}
    void on_enter() override {}
    void handle_input(Key key) override {
        if (key != Key::NONE) change_to_fresh_scene("title");
    }
    void update() override {}
    void render() override {
        console->clear();
        auto center = [&](int y, const std::string& s, Color fg) {
            console->draw_string((CON_W - (int)s.size()) / 2, y, s, fg, Color::BLACK);
        };
        center(12, "========================================", Color::DARK_RED);
        center(14, "   Y O U   D I E D   ", Color::RED);
        center(16, "========================================", Color::DARK_RED);
        center(19, "The depths of the keep have claimed another soul...", Color::GRAY);
        center(23, "[ Press any key to return to title ]", Color::DARK_GRAY);
        console->present();
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  GAME SCENE — the main z-level showcase
// ════════════════════════════════════════════════════════════════════════════
class GameScene : public Scene {
private:
    World world;

    // Player state
    EntityId player_id = 0;
    int player_hp      = 20;
    int player_max_hp  = 20;
    int player_atk     = 4;
    bool has_key       = false;
    bool has_treasure  = false;
    int potions_used   = 0;
    int kills          = 0;

    // Enemies & items
    std::vector<EnemyInfo>  enemies;
    std::vector<ItemInfo>   items;

    // Messages
    std::vector<std::string> messages;
    int msg_scroll = 0;

    // Turn counter (enemies act every player move)
    bool player_acted = false;

    // ── Helpers ─────────────────────────────────────────────────────────────
    void msg(const std::string& s) {
        messages.push_back(s);
        msg_scroll = 0; // auto-scroll to latest
    }

    EnemyInfo* find_enemy_at(int x, int y, int z) {
        for (auto& e : enemies) {
            if (!e.alive) continue;
            auto* pos = manager->get_component<PositionComponent>(e.id);
            if (pos && pos->x == x && pos->y == y && pos->z == z) return &e;
        }
        return nullptr;
    }

    ItemInfo* find_item_at(int x, int y, int z) {
        for (auto& it : items) {
            if (it.collected) continue;
            auto* pos = manager->get_component<PositionComponent>(it.id);
            if (pos && pos->x == x && pos->y == y && pos->z == z) return &it;
        }
        return nullptr;
    }

    // ── World Building ──────────────────────────────────────────────────────
    void build_world() {
        // Fill everything as solid rock first
        for (int z = 0; z < WORLD_DEPTH; ++z) {
            for (int y = 0; y < WORLD_H; ++y) {
                for (int x = 0; x < WORLD_W; ++x) {
                    Tile& t = world.get_tile(x, y, z);
                    t.symbol    = '#';
                    t.color     = "dark_gray";
                    t.walkable  = false;
                    t.has_floor = true;
                    t.transparent = false;
                    t.below_symbol = '#';
                    t.below_color  = "dark_gray";
                }
            }
        }

        build_z4_tower_roof();
        build_z3_keep();
        build_z2_dungeon();
        build_z1_catacombs();
        build_z0_dragons_lair();
        place_stairs();
    }

    // -- helper: carve a rectangular room
    void carve_room(int z, int x1, int y1, int x2, int y2,
                    char floor_ch, const std::string& floor_col,
                    char wall_ch = '#', const std::string& wall_col = "gray") {
        for (int y = y1; y <= y2; ++y) {
            for (int x = x1; x <= x2; ++x) {
                Tile& t = world.get_tile(x, y, z);
                if (x == x1 || x == x2 || y == y1 || y == y2) {
                    t.symbol   = wall_ch;
                    t.color    = wall_col;
                    t.walkable = false;
                    t.below_symbol = wall_ch;
                    t.below_color  = wall_col;
                } else {
                    t.symbol   = floor_ch;
                    t.color    = floor_col;
                    t.walkable = true;
                    t.below_symbol = floor_ch;
                    t.below_color  = "dark_gray";
                }
            }
        }
    }

    // helper: carve horizontal corridor
    void carve_hcorridor(int z, int x1, int x2, int y,
                         char ch = '.', const std::string& col = "gray") {
        int lo = std::min(x1, x2), hi = std::max(x1, x2);
        for (int x = lo; x <= hi; ++x) {
            Tile& t = world.get_tile(x, y, z);
            t.symbol   = ch; t.color = col;
            t.walkable = true;
            t.below_symbol = ch; t.below_color = "dark_gray";
        }
    }

    // helper: carve vertical corridor
    void carve_vcorridor(int z, int x, int y1, int y2,
                         char ch = '.', const std::string& col = "gray") {
        int lo = std::min(y1, y2), hi = std::max(y1, y2);
        for (int y = lo; y <= hi; ++y) {
            Tile& t = world.get_tile(x, y, z);
            t.symbol   = ch; t.color = col;
            t.walkable = true;
            t.below_symbol = ch; t.below_color = "dark_gray";
        }
    }

    // helper: set a single tile
    void set_floor(int x, int y, int z, char ch, const std::string& col, bool walk = true) {
        Tile& t = world.get_tile(x, y, z);
        t.symbol = ch; t.color = col; t.walkable = walk;
        t.has_floor = true; t.transparent = false;
        t.below_symbol = ch; t.below_color = "dark_gray";
    }

    void set_wall(int x, int y, int z, char ch = '#', const std::string& col = "gray") {
        Tile& t = world.get_tile(x, y, z);
        t.symbol = ch; t.color = col; t.walkable = false;
        t.has_floor = true; t.transparent = false;
        t.below_symbol = ch; t.below_color = col;
    }

    void set_open_air(int x, int y, int z) {
        Tile& t = world.get_tile(x, y, z);
        t.symbol = ' '; t.color = "white"; t.walkable = false;
        t.has_floor = false; t.transparent = true;
    }

    // ── Z=4  Tower Roof ─────────────────────────────────────────────────────
    void build_z4_tower_roof() {
        int z = 4;
        // Open air everywhere first
        for (int y = 0; y < WORLD_H; ++y)
            for (int x = 0; x < WORLD_W; ++x)
                set_open_air(x, y, z);

        // Small stone platform 20x16 centered at (30,18)
        int px = 25, py = 14, pw = 20, ph = 14;
        for (int y = py; y < py + ph; ++y) {
            for (int x = px; x < px + pw; ++x) {
                Tile& t = world.get_tile(x, y, z);
                if (x == px || x == px + pw - 1 || y == py || y == py + ph - 1) {
                    // Parapet with crenellations
                    bool crenel = ((x + y) % 3 == 0);
                    if (crenel) {
                        // Gap in parapet — open air, let you see below
                        t.symbol = ' '; t.color = "white"; t.walkable = false;
                        t.has_floor = true; t.transparent = true;
                        t.below_symbol = '.'; t.below_color = "dark_gray";
                    } else {
                        t.symbol = '#'; t.color = "white"; t.walkable = false;
                        t.has_floor = true; t.transparent = false;
                        t.below_symbol = '#'; t.below_color = "gray";
                    }
                } else {
                    t.symbol = '.'; t.color = "gray"; t.walkable = true;
                    t.has_floor = true; t.transparent = false;
                    t.below_symbol = '.'; t.below_color = "dark_gray";
                }
            }
        }

        // Flagpole
        set_floor(35, 17, z, 'P', "red", false);

        // Observation spots (transparent grates showing Z=3 below)
        for (int gx = px + 3; gx < px + pw - 3; gx += 4) {
            for (int gy = py + 3; gy < py + ph - 3; gy += 4) {
                Tile& t = world.get_tile(gx, gy, z);
                t.symbol = '#'; t.color = "dark_cyan"; t.walkable = true;
                t.has_floor = false; t.transparent = true;
                t.below_symbol = '.'; t.below_color = "dark_cyan";
            }
        }
    }

    // ── Z=3  Keep Ground Floor ──────────────────────────────────────────────
    void build_z3_keep() {
        int z = 3;
        // Outdoor grass around the keep
        for (int y = 0; y < WORLD_H; ++y)
            for (int x = 0; x < WORLD_W; ++x)
                set_floor(x, y, z, '.', "dark_green", true);

        // Keep outer walls   (20,10) to (60,40)
        carve_room(z, 20, 10, 60, 40, '.', "dark_yellow", '#', "white");

        // Great hall (center)
        carve_room(z, 30, 15, 50, 30, '.', "dark_yellow", '#', "gray");
        // Hall entrance (south)
        set_floor(40, 30, z, '+', "yellow");
        // Hall north door
        set_floor(40, 15, z, '+', "yellow");

        // Storage room (west)
        carve_room(z, 22, 15, 29, 25, '.', "dark_yellow", '#', "gray");
        set_floor(29, 20, z, '+', "yellow");

        // Guard room (east)
        carve_room(z, 51, 15, 58, 25, '.', "dark_yellow", '#', "gray");
        set_floor(51, 20, z, '+', "yellow");

        // Corridors connecting rooms
        carve_hcorridor(z, 21, 59, 20, '.', "dark_yellow");
        carve_vcorridor(z, 40, 11, 39, '.', "dark_yellow");

        // Courtyard entrance (south wall gap)
        set_floor(40, 40, z, '.', "dark_yellow");
        set_floor(41, 40, z, '.', "dark_yellow");

        // Outdoor path leading south
        carve_vcorridor(z, 40, 40, 48, '.', "dark_green");

        // Some decoration: pillars in great hall
        for (int px : {33, 37, 43, 47}) {
            for (int py : {18, 22, 26}) {
                set_wall(px, py, z, 'O', "dark_yellow");
            }
        }

        // Torches on walls
        for (int tx : {31, 35, 39, 43, 47}) {
            set_wall(tx, 15, z, '*', "yellow");
        }

        // Tables in great hall
        for (int tx = 38; tx <= 42; ++tx) {
            set_floor(tx, 20, z, '=', "dark_yellow", false);
        }

        // Barrels in storage
        set_floor(24, 17, z, 'o', "dark_yellow", false);
        set_floor(25, 17, z, 'o', "dark_yellow", false);
        set_floor(24, 18, z, 'o', "dark_yellow", false);

        // Trees outside
        int trees[][2] = {{10,5},{15,8},{65,12},{70,20},{12,42},{68,38},{8,30},{72,44}};
        for (auto& tr : trees) {
            if (tr[0] < WORLD_W && tr[1] < WORLD_H)
                set_floor(tr[0], tr[1], z, 'T', "green", false);
        }

        // Well in courtyard
        set_floor(40, 35, z, 'O', "cyan", false);
        set_floor(41, 35, z, 'O', "cyan", false);
    }

    // ── Z=2  Dungeon ────────────────────────────────────────────────────────
    void build_z2_dungeon() {
        int z = 2;

        // Main corridor running east-west
        carve_hcorridor(z, 10, 65, 25, '.', "dark_cyan");
        // North-south corridor
        carve_vcorridor(z, 35, 10, 40, '.', "dark_cyan");

        // Prison cells along north side
        for (int i = 0; i < 5; ++i) {
            int cx = 14 + i * 10;
            carve_room(z, cx, 15, cx + 6, 24, '.', "dark_cyan", '#', "dark_gray");
            // Cell door
            set_floor(cx + 3, 24, z, '+', "dark_yellow");
            // Cell details — chains on back wall
            set_wall(cx + 2, 16, z, '&', "dark_gray");
            set_wall(cx + 4, 16, z, '&', "dark_gray");
        }

        // Torture chamber (south)
        carve_room(z, 25, 28, 45, 38, '.', "dark_red", '#', "dark_gray");
        set_floor(35, 28, z, '+', "dark_yellow");
        // Torture devices
        set_floor(30, 32, z, 'X', "dark_red", false);
        set_floor(35, 33, z, 'X', "dark_red", false);
        set_floor(40, 32, z, 'X', "dark_red", false);

        // Storage alcove with the dungeon key
        carve_room(z, 55, 15, 63, 23, '.', "dark_cyan", '#', "dark_gray");
        set_floor(55, 19, z, '+', "dark_yellow");

        // Locked door blocking passage to deeper levels
        // This door at the stairway down requires the key
        set_floor(35, 40, z, 'X', "yellow", false);  // Locked gate marker

        // Make Z=3 tiles above the dungeon corridor transparent (grates)
        for (int gx = 33; gx <= 37; ++gx) {
            Tile& t3 = world.get_tile(gx, 25, 3);
            t3.has_floor = false;
            t3.transparent = true;
            t3.symbol = '#';
            t3.color = "dark_cyan";
            t3.walkable = false;
            // Below shows dungeon
        }
    }

    // ── Z=1  Catacombs ──────────────────────────────────────────────────────
    void build_z1_catacombs() {
        int z = 1;

        // Winding passages
        carve_hcorridor(z, 10, 40, 15, '.', "dark_gray");
        carve_vcorridor(z, 40, 15, 35, '.', "dark_gray");
        carve_hcorridor(z, 40, 65, 35, '.', "dark_gray");
        carve_vcorridor(z, 65, 20, 35, '.', "dark_gray");
        carve_hcorridor(z, 50, 65, 20, '.', "dark_gray");
        carve_vcorridor(z, 50, 10, 20, '.', "dark_gray");
        carve_hcorridor(z, 30, 50, 10, '.', "dark_gray");
        carve_vcorridor(z, 10, 15, 35, '.', "dark_gray");
        carve_hcorridor(z, 10, 30, 35, '.', "dark_gray");

        // Crypt rooms
        carve_room(z, 12, 17, 22, 25, '.', "dark_gray", '#', "dark_magenta");
        set_floor(17, 25, z, '+', "dark_magenta");
        // Coffins
        set_floor(15, 20, z, '=', "dark_magenta", false);
        set_floor(19, 20, z, '=', "dark_magenta", false);
        set_floor(15, 22, z, '=', "dark_magenta", false);
        set_floor(19, 22, z, '=', "dark_magenta", false);

        // Ossuary
        carve_room(z, 55, 22, 63, 33, '.', "dark_gray", '#', "dark_magenta");
        set_floor(55, 28, z, '+', "dark_magenta");
        // Bone piles
        for (int by = 24; by <= 31; by += 2)
            for (int bx = 57; bx <= 61; bx += 2)
                set_floor(bx, by, z, '%', "white", false);

        // Large tomb room at end
        carve_room(z, 15, 28, 28, 34, '.', "dark_gray", '#', "dark_magenta");
        set_floor(22, 28, z, '+', "dark_magenta");
        // Sarcophagus
        set_floor(20, 31, z, '=', "dark_yellow", false);
        set_floor(21, 31, z, '=', "dark_yellow", false);
        set_floor(22, 31, z, '=', "dark_yellow", false);

        // Underground stream
        for (int sx = 30; sx <= 48; ++sx) {
            int sy = 25 + (int)(2.0 * sin(sx * 0.4));
            if (sy >= 0 && sy < WORLD_H) {
                set_floor(sx, sy, z, '~', "blue", false);
                // Also adjacent
                if (sy + 1 < WORLD_H)
                    set_floor(sx, sy + 1, z, '~', "dark_blue", false);
            }
        }
    }

    // ── Z=0  Dragon's Lair ──────────────────────────────────────────────────
    void build_z0_dragons_lair() {
        int z = 0;

        // Large cavern  (15,10) to (65,40)
        for (int y = 12; y <= 42; ++y) {
            // Irregular cave shape
            int indent_left  = 15 + (int)(3.0 * sin(y * 0.5));
            int indent_right = 65 - (int)(3.0 * sin(y * 0.7 + 1.0));
            for (int x = indent_left; x <= indent_right; ++x) {
                set_floor(x, y, z, '.', "dark_red");
            }
        }

        // Entry corridor from stairs
        carve_vcorridor(z, 35, 8, 14, '.', "dark_red");
        carve_hcorridor(z, 33, 37, 8, '.', "dark_red");

        // Lava pools
        auto lava_pool = [&](int cx, int cy, int r) {
            for (int dy = -r; dy <= r; ++dy) {
                for (int dx = -r; dx <= r; ++dx) {
                    if (dx * dx + dy * dy <= r * r) {
                        int px = cx + dx, py = cy + dy;
                        if (px >= 0 && px < WORLD_W && py >= 0 && py < WORLD_H) {
                            set_floor(px, py, z, '~', "red", false);
                        }
                    }
                }
            }
        };
        lava_pool(25, 25, 3);
        lava_pool(50, 30, 4);
        lava_pool(30, 38, 2);
        lava_pool(55, 20, 2);

        // Treasure hoard area (center)
        for (int ty = 28; ty <= 32; ++ty)
            for (int tx = 38; tx <= 44; ++tx)
                set_floor(tx, ty, z, '*', "yellow");

        // Stalactites / rock pillars
        int pillars[][2] = {{22,18},{30,16},{45,17},{55,25},{20,35},{48,38}};
        for (auto& p : pillars)
            set_wall(p[0], p[1], z, '^', "dark_gray");
    }

    // ── Stairs ──────────────────────────────────────────────────────────────
    void place_stairs() {
        auto make_stairs = [&](int x, int y, int z_low, int z_high) {
            Tile& lo = world.get_tile(x, y, z_low);
            lo.is_stairs_up = true;
            lo.symbol = '<';
            lo.color  = "white";
            lo.walkable = true;

            Tile& hi = world.get_tile(x, y, z_high);
            hi.is_stairs_down = true;
            hi.symbol = '>';
            hi.color  = "white";
            hi.walkable = true;
            hi.has_floor = true;
        };

        // Z3 <-> Z4 : inside the keep great hall, up to the tower roof  (34, 16)
        make_stairs(34, 16, 3, 4);

        // Z2 <-> Z3 : in the keep corridor          (35, 38)
        make_stairs(35, 38, 2, 3);

        // Z1 <-> Z2 : dungeon south end             (35, 40)
        // (the locked gate is here — we replace it with stairs but keep it locked)
        // We'll place stairs at (35, 42) instead and keep the gate at (35, 40)
        make_stairs(35, 42, 1, 2);
        // Make Z=2 tile at (35,42) walkable corridor
        set_floor(35, 41, 2, '.', "dark_cyan");
        set_floor(35, 42, 2, '.', "dark_cyan");
        // Override — the stair set_floor above is overwritten by make_stairs:
        {
            Tile& t = world.get_tile(35, 42, 2);
            t.is_stairs_down = true;
            t.symbol = '>'; t.color = "white";
            t.walkable = true; t.has_floor = true;
        }

        // Z0 <-> Z1 : catacombs connecting to lair  (35, 10)
        make_stairs(35, 10, 0, 1);
        // make sure the area around the stairs in Z=1 is open
        set_floor(34, 10, 1, '.', "dark_gray");
        set_floor(36, 10, 1, '.', "dark_gray");
        // Override Z1 stair tile
        {
            Tile& t = world.get_tile(35, 10, 1);
            t.is_stairs_down = true;
            t.symbol = '>'; t.color = "white";
            t.walkable = true; t.has_floor = true;
        }
    }

    // ── Entity Spawning ─────────────────────────────────────────────────────
    void spawn_entities() {
        // ── Player on Z=3, in the courtyard ──
        player_id = manager->create_entity();
        manager->add_component<PositionComponent>(player_id, 40, 35, 3);
        manager->add_component<RenderComponent>(player_id, '@', "yellow", "", 100);

        // ── Enemies ─────────────────────────────────────────────────────────
        auto spawn_enemy = [&](int x, int y, int z, char ch, const std::string& color,
                               const std::string& name, int hp, int atk, int sight) {
            EntityId id = manager->create_entity();
            manager->add_component<PositionComponent>(id, x, y, z);
            manager->add_component<RenderComponent>(id, ch, color, "", 50);
            manager->add_component<CollisionComponent>(id, true, false);
            enemies.push_back({id, name, hp, hp, atk, sight, true});
        };

        // Z=2: Dungeon rats
        spawn_enemy(16, 22, 2, 'r', "dark_yellow",  "Rat",      4, 1, 5);
        spawn_enemy(26, 22, 2, 'r', "dark_yellow",  "Rat",      4, 1, 5);
        spawn_enemy(36, 22, 2, 'r', "dark_yellow",  "Rat",      3, 1, 5);
        spawn_enemy(46, 22, 2, 'r', "dark_yellow",  "Rat",      4, 1, 5);
        spawn_enemy(32, 33, 2, 'r', "dark_yellow",  "Rat",      5, 2, 5);

        // Z=1: Catacombs skeletons
        spawn_enemy(17, 21, 1, 's', "white",        "Skeleton",  8, 3, 7);
        spawn_enemy(42, 35, 1, 's', "white",        "Skeleton",  8, 3, 7);
        spawn_enemy(60, 28, 1, 's', "white",        "Skeleton", 10, 3, 7);
        spawn_enemy(50, 15, 1, 's', "white",        "Skeleton",  8, 3, 7);
        // Wraith (stronger)
        spawn_enemy(22, 31, 1, 'W', "dark_magenta", "Wraith",   12, 4, 8);

        // Z=0: Dragon
        spawn_enemy(41, 25, 0, 'D', "red",          "Dragon",   30, 6, 10);
        // Fire imps
        spawn_enemy(25, 20, 0, 'i', "red",          "Fire Imp",  6, 3, 6);
        spawn_enemy(50, 35, 0, 'i', "red",          "Fire Imp",  6, 3, 6);

        // ── Items ───────────────────────────────────────────────────────────
        auto spawn_item = [&](int x, int y, int z, char ch, const std::string& color,
                              const std::string& type) {
            EntityId id = manager->create_entity();
            manager->add_component<PositionComponent>(id, x, y, z);
            manager->add_component<RenderComponent>(id, ch, color, "", 30);
            items.push_back({id, type, false});
        };

        // Health potions
        spawn_item(25, 18, 3, '!', "magenta", "potion");   // In storage room
        spawn_item(55, 20, 2, '!', "magenta", "potion");   // In dungeon alcove
        spawn_item(17, 15, 1, '!', "magenta", "potion");   // Catacombs entrance area
        spawn_item(60, 25, 1, '!', "magenta", "potion");   // Near ossuary
        spawn_item(35, 12, 0, '!', "magenta", "potion");   // Near dragon's lair entrance

        // Key (in the dungeon alcove)
        spawn_item(59, 19, 2, 'k', "yellow", "key");

        // Treasure (in dragon's lair)
        spawn_item(41, 30, 0, '$', "yellow", "treasure");

        // ── Friendly NPCs (no collision, just decoration + messages) ────────
        EntityId merchant = manager->create_entity();
        manager->add_component<PositionComponent>(merchant, 26, 20, 3);
        manager->add_component<RenderComponent>(merchant, 'M', "cyan", "", 50);

        EntityId guard = manager->create_entity();
        manager->add_component<PositionComponent>(guard, 53, 20, 3);
        manager->add_component<RenderComponent>(guard, 'G', "green", "", 50);
    }

    // ── Enemy AI ────────────────────────────────────────────────────────────
    void enemy_turn() {
        auto* ppos = manager->get_component<PositionComponent>(player_id);
        if (!ppos) return;

        for (auto& e : enemies) {
            if (!e.alive) continue;
            auto* epos = manager->get_component<PositionComponent>(e.id);
            if (!epos || epos->z != ppos->z) continue;

            int dx = ppos->x - epos->x;
            int dy = ppos->y - epos->y;
            int dist = std::abs(dx) + std::abs(dy);

            if (dist > e.sight_range || dist == 0) continue;

            // Move one step toward player
            int mx = 0, my = 0;
            if (std::abs(dx) >= std::abs(dy)) mx = (dx > 0) ? 1 : -1;
            else                               my = (dy > 0) ? 1 : -1;

            int nx = epos->x + mx;
            int ny = epos->y + my;

            // Attack player if adjacent
            if (nx == ppos->x && ny == ppos->y) {
                int dmg = e.attack;
                player_hp -= dmg;
                msg(e.name + " hits you for " + std::to_string(dmg) + " damage!");
                if (player_hp <= 0) {
                    player_hp = 0;
                    msg("You have been slain...");
                }
                continue;
            }

            // Don't walk into walls or other enemies
            if (nx >= 0 && nx < WORLD_W && ny >= 0 && ny < WORLD_H) {
                if (world.get_tile(nx, ny, epos->z).walkable && !find_enemy_at(nx, ny, epos->z)) {
                    epos->x = nx;
                    epos->y = ny;
                }
            }
        }
    }

    // ── Player combat ───────────────────────────────────────────────────────
    bool try_attack(int x, int y, int z) {
        EnemyInfo* e = find_enemy_at(x, y, z);
        if (!e) return false;

        int dmg = player_atk;
        e->hp -= dmg;
        msg("You hit the " + e->name + " for " + std::to_string(dmg) + "! (" +
            std::to_string(std::max(0, e->hp)) + "/" + std::to_string(e->max_hp) + ")");

        if (e->hp <= 0) {
            e->alive = false;
            manager->destroy_entity(e->id);
            msg("The " + e->name + " is dead!");
            kills++;
        }
        return true;
    }

    // ── Item pickup ─────────────────────────────────────────────────────────
    void check_pickup(int x, int y, int z) {
        ItemInfo* it = find_item_at(x, y, z);
        if (!it) return;

        if (it->type == "potion") {
            int heal = 8;
            player_hp = std::min(player_max_hp, player_hp + heal);
            msg("You drink a health potion. +" + std::to_string(heal) + " HP!");
            potions_used++;
        } else if (it->type == "key") {
            has_key = true;
            msg("You found the DUNGEON KEY!");
        } else if (it->type == "treasure") {
            has_treasure = true;
            msg("You seized the DRAGON'S TREASURE! Return to the tower roof!");
        }

        it->collected = true;
        manager->destroy_entity(it->id);
    }

    // ── Locked gate check ───────────────────────────────────────────────────
    bool try_unlock_gate(int x, int y, int z) {
        Tile& t = world.get_tile(x, y, z);
        if (t.symbol == 'X' && !t.walkable && z == 2 && x == 35 && y == 40) {
            if (has_key) {
                t.symbol = '.';
                t.color = "dark_cyan";
                t.walkable = true;
                msg("You unlock the gate with the dungeon key!");
                return true;
            } else {
                msg("The gate is locked. You need a key.");
                return false;
            }
        }
        return false;
    }

    // ── Victory check ───────────────────────────────────────────────────────
    bool check_victory() {
        auto* pos = manager->get_component<PositionComponent>(player_id);
        return has_treasure && pos && pos->z == 4;
    }

    // ── Draw HUD ────────────────────────────────────────────────────────────
    void draw_hud() {
        int px = MAP_W;  // Panel starts here
        int pw = PANEL_W;

        // Panel background
        console->fill_rect(px, 0, pw, MAP_H, ' ', Color::WHITE, Color::BLACK);
        console->draw_box(px, 0, pw, MAP_H, Color::DARK_GRAY, Color::BLACK);

        // Title
        console->draw_string(px + 2, 1, "DEPTHS OF THE KEEP", Color::RED, Color::BLACK);

        // Divider
        for (int x = px + 1; x < px + pw - 1; ++x)
            console->set_cell(x, 2, '-', Color::DARK_GRAY);

        auto* pos = manager->get_component<PositionComponent>(player_id);
        int cur_z = pos ? pos->z : 0;

        // Level info
        console->draw_string(px + 2, 4, "FLOOR:", Color::GRAY);
        Color lvl_col = Color::WHITE;
        if (cur_z == 0) lvl_col = Color::RED;
        else if (cur_z == 1) lvl_col = Color::DARK_MAGENTA;
        else if (cur_z == 2) lvl_col = Color::DARK_CYAN;
        else if (cur_z == 3) lvl_col = Color::DARK_YELLOW;
        else if (cur_z == 4) lvl_col = Color::WHITE;
        console->draw_string(px + 2, 5, LEVEL_NAMES[cur_z], lvl_col);
        console->draw_string(px + 2, 6, "Z = " + std::to_string(cur_z), Color::GRAY);

        // Divider
        for (int x = px + 1; x < px + pw - 1; ++x)
            console->set_cell(x, 7, '-', Color::DARK_GRAY);

        // Health bar
        console->draw_string(px + 2, 9, "HP:", Color::GRAY);
        std::string hp_str = std::to_string(player_hp) + "/" + std::to_string(player_max_hp);
        console->draw_string(px + 6, 9, hp_str, player_hp > 10 ? Color::GREEN : (player_hp > 5 ? Color::YELLOW : Color::RED));

        // HP bar visual
        int bar_w = pw - 4;
        int filled = (player_hp * bar_w) / player_max_hp;
        for (int i = 0; i < bar_w; ++i) {
            Color c = (i < filled) ? (player_hp > 10 ? Color::GREEN : (player_hp > 5 ? Color::YELLOW : Color::RED))
                                   : Color::DARK_GRAY;
            console->set_cell(px + 2 + i, 10, '#', c);
        }

        // Attack
        console->draw_string(px + 2, 12, "ATK: " + std::to_string(player_atk), Color::GRAY);

        // Divider
        for (int x = px + 1; x < px + pw - 1; ++x)
            console->set_cell(x, 13, '-', Color::DARK_GRAY);

        // Inventory
        console->draw_string(px + 2, 15, "INVENTORY:", Color::GRAY);
        int iy = 16;
        if (has_key)      console->draw_string(px + 3, iy++, "* Dungeon Key", Color::YELLOW);
        if (has_treasure)  console->draw_string(px + 3, iy++, "* Dragon Treasure", Color::YELLOW);
        if (!has_key && !has_treasure)
            console->draw_string(px + 3, iy++, "(empty)", Color::DARK_GRAY);

        // Divider
        for (int x = px + 1; x < px + pw - 1; ++x)
            console->set_cell(x, 19, '-', Color::DARK_GRAY);

        // Stats
        console->draw_string(px + 2, 21, "Kills: " + std::to_string(kills), Color::GRAY);
        console->draw_string(px + 2, 22, "Potions: " + std::to_string(potions_used), Color::GRAY);

        // Divider
        for (int x = px + 1; x < px + pw - 1; ++x)
            console->set_cell(x, 24, '-', Color::DARK_GRAY);

        // Z-level map (vertical cross-section)
        console->draw_string(px + 2, 26, "LEVEL MAP:", Color::GRAY);
        for (int z = WORLD_DEPTH - 1; z >= 0; --z) {
            int row = 27 + (WORLD_DEPTH - 1 - z);
            char marker = (z == cur_z) ? '@' : '.';
            Color mc = (z == cur_z) ? Color::YELLOW : Color::DARK_GRAY;
            std::string line = " Z" + std::to_string(z) + " [" + marker + "] ";
            // Truncate level name to fit
            std::string short_name(LEVEL_NAMES[z]);
            if ((int)short_name.size() > pw - 12)
                short_name = short_name.substr(0, pw - 12);
            line += short_name;
            console->draw_string(px + 1, row, line, mc);
        }
    }

    // ── Draw message log ────────────────────────────────────────────────────
    void draw_messages() {
        int my = MAP_H;
        console->draw_box(0, my, CON_W, MSG_H, Color::DARK_GRAY, Color::BLACK);
        console->draw_string(2, my, " LOG ", Color::DARK_GRAY);

        int visible = MSG_H - 2;
        int total = (int)messages.size();
        int start = std::max(0, total - visible - msg_scroll);
        for (int i = 0; i < visible && (start + i) < total; ++i) {
            Color c = (start + i == total - 1) ? Color::WHITE : Color::GRAY;
            std::string line = messages[start + i];
            if ((int)line.size() > CON_W - 4) line = line.substr(0, CON_W - 4);
            console->draw_string(2, my + 1 + i, line, c);
        }
    }

public:
    GameScene(Console* c, ComponentManager* m, Renderer* r, UI* u)
        : Scene(c, m, r, u), world(WORLD_W, WORLD_H, WORLD_DEPTH) {}

    void on_enter() override {
        manager->clear_all_entities();
        enemies.clear();
        items.clear();
        messages.clear();
        msg_scroll  = 0;
        player_hp   = 20;
        player_max_hp = 20;
        player_atk  = 4;
        has_key     = false;
        has_treasure = false;
        potions_used = 0;
        kills       = 0;

        renderer->set_side_panel_width(PANEL_W);
        renderer->set_z_see_through_depth(2);

        build_world();
        spawn_entities();

        msg("You stand in the courtyard of the ruined keep.");
        msg("Legends say a dragon guards a treasure deep below.");
        msg("Find the key, slay the beast, claim the prize.");
        msg("Return to the tower roof to win!");
        msg("WASD: move  </>: stairs  E: interact  ESC: quit");
    }

    void on_exit() override {
        renderer->set_side_panel_width(0);
    }

    void handle_input(Key key) override {
        if (player_hp <= 0) return; // dead

        auto& kb = KeyBindings::instance();
        auto* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;

        player_acted = false;

        int dx = 0, dy = 0;
        if      (kb.is_action(key, GameAction::MOVE_UP))    dy = -1;
        else if (kb.is_action(key, GameAction::MOVE_DOWN))  dy =  1;
        else if (kb.is_action(key, GameAction::MOVE_LEFT))  dx = -1;
        else if (kb.is_action(key, GameAction::MOVE_RIGHT)) dx =  1;
        else if (kb.is_action(key, GameAction::MOVE_UP_LAYER)) {
            if (world.can_go_up(pos->x, pos->y, pos->z)) {
                pos->z += 1;
                msg("You ascend to " + std::string(LEVEL_NAMES[pos->z]) + ".");
                player_acted = true;
            } else {
                msg("No way up here. Look for stairs (<).");
            }
        }
        else if (kb.is_action(key, GameAction::MOVE_DOWN_LAYER)) {
            if (world.can_go_down(pos->x, pos->y, pos->z)) {
                pos->z -= 1;
                msg("You descend to " + std::string(LEVEL_NAMES[pos->z]) + ".");
                player_acted = true;
            } else {
                msg("No way down here. Look for stairs (>).");
            }
        }
        else if (kb.is_action(key, GameAction::ACTION_1)) {
            // Interact — check adjacent tiles for NPCs, locked doors
            int dirs[][2] = {{0,-1},{0,1},{-1,0},{1,0}};
            bool interacted = false;
            for (auto& d : dirs) {
                int ix = pos->x + d[0], iy = pos->y + d[1];
                // Check for locked gate
                if (try_unlock_gate(ix, iy, pos->z)) { interacted = true; break; }
            }
            // Check for NPCs at adjacent positions
            if (!interacted) {
                // Merchant
                for (auto& d : dirs) {
                    int ix = pos->x + d[0], iy = pos->y + d[1];
                    // Check for M (merchant) or G (guard) at that position
                    auto ents = manager->get_entities_with_component<RenderComponent>();
                    for (auto eid : ents) {
                        auto* ep = manager->get_component<PositionComponent>(eid);
                        auto* er = manager->get_component<RenderComponent>(eid);
                        if (ep && er && ep->x == ix && ep->y == iy && ep->z == pos->z) {
                            if (er->ch == 'M') {
                                msg("Merchant: \"Be careful below! The dungeon key is");
                                msg("  hidden in the eastern alcove of Z2.\"");
                                interacted = true;
                            } else if (er->ch == 'G') {
                                msg("Guard: \"A dragon lurks in the deepest level.");
                                msg("  You'll need a key to get past the dungeon gate.\"");
                                interacted = true;
                            }
                        }
                    }
                    if (interacted) break;
                }
            }
            if (!interacted) {
                msg("Nothing to interact with here.");
            }
            player_acted = true;
        }
        else if (kb.is_action(key, GameAction::SCROLL_MESSAGES_UP)) {
            msg_scroll = std::min(msg_scroll + 1, std::max(0, (int)messages.size() - (MSG_H - 2)));
        }
        else if (kb.is_action(key, GameAction::SCROLL_MESSAGES_DOWN)) {
            msg_scroll = std::max(0, msg_scroll - 1);
        }
        else if (kb.is_action(key, GameAction::CANCEL)) {
            exit_application();
            return;
        }

        // Movement
        if (dx != 0 || dy != 0) {
            int nx = pos->x + dx;
            int ny = pos->y + dy;
            if (nx >= 0 && nx < WORLD_W && ny >= 0 && ny < WORLD_H) {
                // Try attack first
                if (try_attack(nx, ny, pos->z)) {
                    player_acted = true;
                }
                // Try unlock gate
                else if (world.get_tile(nx, ny, pos->z).symbol == 'X' &&
                         !world.get_tile(nx, ny, pos->z).walkable) {
                    try_unlock_gate(nx, ny, pos->z);
                    player_acted = true;
                }
                // Move
                else if (world.get_tile(nx, ny, pos->z).walkable) {
                    pos->x = nx;
                    pos->y = ny;
                    check_pickup(nx, ny, pos->z);
                    player_acted = true;
                }
            }
        }
    }

    void update() override {
        // Enemies act after each player move (roguelike turns)
        if (player_acted) {
            enemy_turn();
            player_acted = false;
        }

        // Death check
        if (player_hp <= 0) {
            change_to_fresh_scene("death");
            return;
        }

        // Victory check
        if (check_victory()) {
            change_to_fresh_scene("victory");
        }
    }

    void render() override {
        console->clear();

        auto* pos = manager->get_component<PositionComponent>(player_id);
        if (pos) {
            renderer->center_camera_on_entity(player_id);
        }

        renderer->render_all(&world);
        draw_hud();
        draw_messages();
        renderer->present();
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  MAIN — wire up scenes and run game loop
// ════════════════════════════════════════════════════════════════════════════
class DepthsOfTheKeep {
private:
    Console         console;
    ComponentManager manager;
    Renderer        renderer;
    UI              ui;
    SceneManager    scene_manager;

public:
    DepthsOfTheKeep()
        : console(CON_W, CON_H)
        , renderer(&console, &manager)
        , ui(&console, &manager)
    {
        auto make = [&](auto* self) {
            (void)self; // unused
            return [this]<typename T>() {
                return std::make_unique<T>(&console, &manager, &renderer, &ui);
            };
        };

        scene_manager.register_scene<TitleScene>("title", [this]() -> std::unique_ptr<Scene> {
            return std::make_unique<TitleScene>(&console, &manager, &renderer, &ui);
        });
        scene_manager.register_scene<GameScene>("game", [this]() -> std::unique_ptr<Scene> {
            return std::make_unique<GameScene>(&console, &manager, &renderer, &ui);
        });
        scene_manager.register_scene<VictoryScene>("victory", [this]() -> std::unique_ptr<Scene> {
            return std::make_unique<VictoryScene>(&console, &manager, &renderer, &ui);
        });
        scene_manager.register_scene<DeathScene>("death", [this]() -> std::unique_ptr<Scene> {
            return std::make_unique<DeathScene>(&console, &manager, &renderer, &ui);
        });

        scene_manager.load_scene("title");
    }

    void run() {
        while (scene_manager.has_active_scene()) {
            Key key = Input::get_key();
            scene_manager.handle_input(key);
            scene_manager.update();
            scene_manager.render();
            SLEEP_MS(16);
        }
    }
};

int main() {
    DepthsOfTheKeep game;
    game.run();
    return 0;
}
