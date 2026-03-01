// =============================================================================
// "Below" - Vulkan 2D Roguelike Demo
// =============================================================================
// A small but playable dungeon crawler built on top of the Vulkan 2D renderer.
// Features:
//   - Procedural dungeon generation (BSP rooms + corridors)
//   - Turn-based tile movement (bump-to-attack)
//   - Field of view / fog of war
//   - Enemies with chase AI
//   - Health potions, gold, weapon pickups
//   - Stairs to descend deeper
//   - HUD: health bar, stats, message log
//   - Camera follows player with smooth lerp
//
// Controls:
//   WASD / Arrows  - Move / attack
//   E              - Pick up item / use stairs
//   I              - Toggle inventory
//   Q              - Quaff (drink) health potion
//   ESCAPE         - Quit
//   +/-            - Zoom in/out
// =============================================================================

#include "render/vulkan/vk_renderer.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <vector>
#include <string>
#include <deque>
#include <random>
#include <functional>
#include <sstream>

// =============================================================================
// Constants
// =============================================================================

static constexpr int   TILE_SIZE    = 16;
static constexpr int   MAP_W        = 80;
static constexpr int   MAP_H        = 50;
static constexpr int   MAX_ROOMS    = 15;
static constexpr int   MIN_ROOM_W   = 5;
static constexpr int   MAX_ROOM_W   = 14;
static constexpr int   MIN_ROOM_H   = 4;
static constexpr int   MAX_ROOM_H   = 10;
static constexpr int   FOV_RADIUS   = 10;
static constexpr int   MAX_ENEMIES  = 20;
static constexpr int   MAX_ITEMS    = 30;
static constexpr int   MAX_LOG_LINES = 8;
static constexpr float LOG_FADE_TIME = 8.0f;

// =============================================================================
// Random
// =============================================================================

static std::mt19937 g_rng;

static int rng_int(int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(g_rng);
}

static float rng_float(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(g_rng);
}

static bool rng_chance(float pct) {
    return rng_float(0.0f, 1.0f) < pct;
}

// =============================================================================
// Tile types
// =============================================================================

enum class Tile : uint8_t {
    Void,         // unexplored / out of bounds
    Wall,
    Floor,
    StairsDown,
    Water,
    Door,
};

static bool tile_walkable(Tile t) {
    return t == Tile::Floor || t == Tile::StairsDown || t == Tile::Door;
}

static bool tile_transparent(Tile t) {
    return t != Tile::Wall && t != Tile::Void;
}

// =============================================================================
// Colors
// =============================================================================

namespace Colors {
    static Color4 bg_void()      { return Color4::from_hex(0x0a0a12); }
    static Color4 bg_wall()      { return Color4::from_hex(0x2c2c3a); }
    static Color4 bg_floor()     { return Color4::from_hex(0x1a1a24); }
    static Color4 bg_stairs()    { return Color4::from_hex(0x1a2430); }
    static Color4 bg_water()     { return Color4::from_hex(0x0a1530); }
    static Color4 bg_door()      { return Color4::from_hex(0x2a2018); }

    static Color4 wall_fg()      { return Color4::from_hex(0x606080); }
    static Color4 floor_fg()     { return Color4::from_hex(0x35354a); }
    static Color4 stairs_fg()    { return Color4::from_hex(0x40a0c0); }
    static Color4 water_fg()     { return Color4::from_hex(0x2060c0); }
    static Color4 door_fg()      { return Color4::from_hex(0x806030); }

    static Color4 player()       { return Color4::from_hex(0xf0d040); }
    static Color4 goblin()       { return Color4::from_hex(0x40c040); }
    static Color4 skeleton()     { return Color4::from_hex(0xd0d0d0); }
    static Color4 orc()          { return Color4::from_hex(0x80a040); }
    static Color4 spider()       { return Color4::from_hex(0x804080); }
    static Color4 demon()        { return Color4::from_hex(0xc02020); }
    static Color4 dragon()       { return Color4::from_hex(0xff6010); }

    static Color4 potion_hp()    { return Color4::from_hex(0xff3060); }
    static Color4 gold()         { return Color4::from_hex(0xffd700); }
    static Color4 weapon()       { return Color4::from_hex(0x60c0ff); }

    static Color4 fog()          { return { 0.2f, 0.2f, 0.3f, 1.0f }; }
    static Color4 hud_bg()       { return { 0.05f, 0.05f, 0.08f, 0.9f }; }
    static Color4 hp_bar()       { return Color4::from_hex(0xc02020); }
    static Color4 hp_bar_bg()    { return Color4::from_hex(0x401010); }
    static Color4 xp_bar()       { return Color4::from_hex(0x20a0c0); }
    static Color4 xp_bar_bg()    { return Color4::from_hex(0x103040); }
}

// =============================================================================
// Dungeon Map
// =============================================================================

struct Room {
    int x, y, w, h;
    int cx() const { return x + w / 2; }
    int cy() const { return y + h / 2; }
    bool intersects(const Room& o, int pad = 1) const {
        return !(x - pad >= o.x + o.w || o.x - pad >= x + w ||
                 y - pad >= o.y + o.h || o.y - pad >= y + h);
    }
};

struct DungeonMap {
    Tile tiles[MAP_W][MAP_H];
    bool visible[MAP_W][MAP_H];   // currently in FOV
    bool revealed[MAP_W][MAP_H];  // ever seen
    std::vector<Room> rooms;

    void clear() {
        for (int x = 0; x < MAP_W; x++)
            for (int y = 0; y < MAP_H; y++) {
                tiles[x][y] = Tile::Void;
                visible[x][y] = false;
                revealed[x][y] = false;
            }
        rooms.clear();
    }

    Tile at(int x, int y) const {
        if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return Tile::Void;
        return tiles[x][y];
    }

    void set(int x, int y, Tile t) {
        if (x >= 0 && y >= 0 && x < MAP_W && y < MAP_H)
            tiles[x][y] = t;
    }

    bool is_walkable(int x, int y) const {
        return tile_walkable(at(x, y));
    }
};

static void carve_room(DungeonMap& map, const Room& r) {
    for (int x = r.x; x < r.x + r.w; x++)
        for (int y = r.y; y < r.y + r.h; y++)
            map.set(x, y, Tile::Floor);
    // Walls around
    for (int x = r.x - 1; x <= r.x + r.w; x++) {
        if (map.at(x, r.y - 1) == Tile::Void) map.set(x, r.y - 1, Tile::Wall);
        if (map.at(x, r.y + r.h) == Tile::Void) map.set(x, r.y + r.h, Tile::Wall);
    }
    for (int y = r.y - 1; y <= r.y + r.h; y++) {
        if (map.at(r.x - 1, y) == Tile::Void) map.set(r.x - 1, y, Tile::Wall);
        if (map.at(r.x + r.w, y) == Tile::Void) map.set(r.x + r.w, y, Tile::Wall);
    }
}

static void carve_h_corridor(DungeonMap& map, int x1, int x2, int y) {
    int lo = std::min(x1, x2), hi = std::max(x1, x2);
    for (int x = lo; x <= hi; x++) {
        map.set(x, y, Tile::Floor);
        if (map.at(x, y - 1) == Tile::Void) map.set(x, y - 1, Tile::Wall);
        if (map.at(x, y + 1) == Tile::Void) map.set(x, y + 1, Tile::Wall);
    }
}

static void carve_v_corridor(DungeonMap& map, int y1, int y2, int x) {
    int lo = std::min(y1, y2), hi = std::max(y1, y2);
    for (int y = lo; y <= hi; y++) {
        map.set(x, y, Tile::Floor);
        if (map.at(x - 1, y) == Tile::Void) map.set(x - 1, y, Tile::Wall);
        if (map.at(x + 1, y) == Tile::Void) map.set(x + 1, y, Tile::Wall);
    }
}

static void generate_dungeon(DungeonMap& map, int floor_num) {
    map.clear();

    int num_rooms = std::min(MAX_ROOMS, 6 + floor_num * 2);

    for (int attempt = 0; attempt < 200 && (int)map.rooms.size() < num_rooms; attempt++) {
        Room r;
        r.w = rng_int(MIN_ROOM_W, MAX_ROOM_W);
        r.h = rng_int(MIN_ROOM_H, MAX_ROOM_H);
        r.x = rng_int(2, MAP_W - r.w - 2);
        r.y = rng_int(2, MAP_H - r.h - 2);

        bool ok = true;
        for (auto& existing : map.rooms) {
            if (r.intersects(existing, 2)) { ok = false; break; }
        }
        if (!ok) continue;

        carve_room(map, r);

        if (!map.rooms.empty()) {
            auto& prev = map.rooms.back();
            if (rng_chance(0.5f)) {
                carve_h_corridor(map, prev.cx(), r.cx(), prev.cy());
                carve_v_corridor(map, prev.cy(), r.cy(), r.cx());
            } else {
                carve_v_corridor(map, prev.cy(), r.cy(), prev.cx());
                carve_h_corridor(map, prev.cx(), r.cx(), r.cy());
            }
        }

        map.rooms.push_back(r);
    }

    // Add some doors at corridor-room junctions
    for (int x = 1; x < MAP_W - 1; x++) {
        for (int y = 1; y < MAP_H - 1; y++) {
            if (map.at(x, y) != Tile::Floor) continue;
            // Detect narrow corridor openings (floor with walls on 2 opposite sides)
            bool h_walls = (map.at(x-1, y) == Tile::Wall && map.at(x+1, y) == Tile::Wall);
            bool v_walls = (map.at(x, y-1) == Tile::Wall && map.at(x, y+1) == Tile::Wall);
            if ((h_walls || v_walls) && rng_chance(0.15f)) {
                map.set(x, y, Tile::Door);
            }
        }
    }

    // Place stairs in the last room
    if (!map.rooms.empty()) {
        auto& last = map.rooms.back();
        map.set(last.cx(), last.cy(), Tile::StairsDown);
    }

    // Sprinkle water pools in some rooms
    for (auto& room : map.rooms) {
        if (rng_chance(0.2f)) {
            int wx = rng_int(room.x + 1, room.x + room.w - 2);
            int wy = rng_int(room.y + 1, room.y + room.h - 2);
            for (int dx = 0; dx < rng_int(1, 3); dx++)
                for (int dy = 0; dy < rng_int(1, 2); dy++)
                    if (map.at(wx+dx, wy+dy) == Tile::Floor)
                        map.set(wx+dx, wy+dy, Tile::Water);
        }
    }
}

// =============================================================================
// Field of View (simple shadow-casting)
// =============================================================================

static void compute_fov(DungeonMap& map, int px, int py, int radius) {
    // Clear visibility
    for (int x = 0; x < MAP_W; x++)
        for (int y = 0; y < MAP_H; y++)
            map.visible[x][y] = false;

    // Simple raycasting FOV
    for (int i = 0; i < 360; i++) {
        float angle = (float)i * 3.14159265f / 180.0f;
        float dx = cosf(angle);
        float dy = sinf(angle);
        float fx = (float)px + 0.5f;
        float fy = (float)py + 0.5f;

        for (int j = 0; j < radius; j++) {
            int tx = (int)fx;
            int ty = (int)fy;
            if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) break;

            map.visible[tx][ty] = true;
            map.revealed[tx][ty] = true;

            if (!tile_transparent(map.tiles[tx][ty])) break;

            fx += dx;
            fy += dy;
        }
    }
}

// =============================================================================
// Entity types
// =============================================================================

enum class EnemyType {
    Goblin, Skeleton, Orc, Spider, Demon, Dragon
};

struct EnemyTemplate {
    const char* name;
    const char* glyph;
    int hp, atk, def, xp_reward;
    int min_floor;         // earliest floor this enemy spawns
    Color4 color;
};

static EnemyTemplate ENEMY_TEMPLATES[] = {
    { "Goblin",   "g", 12,  5,  2,  15, 0, Colors::goblin()   },
    { "Skeleton", "s", 10,  6,  1,  18, 0, Colors::skeleton() },
    { "Orc",      "O", 22,  9,  4,  35, 2, Colors::orc()      },
    { "Spider",   "S", 16,  7,  3,  25, 1, Colors::spider()   },
    { "Demon",    "D", 35, 14,  7,  80, 4, Colors::demon()    },
    { "Dragon",   "W", 60, 20, 12, 200, 6, Colors::dragon()   },
};
static constexpr int NUM_ENEMY_TYPES = sizeof(ENEMY_TEMPLATES) / sizeof(ENEMY_TEMPLATES[0]);

enum class ItemType {
    HealthPotion,
    GreaterHealthPotion,
    Gold,
    Weapon,
};

struct ItemTemplate {
    const char* name;
    const char* glyph;
    ItemType type;
    int value;           // heal amount, gold amount, or atk bonus
    Color4 color;
};

static ItemTemplate ITEM_TEMPLATES[] = {
    { "Health Potion",         "!",  ItemType::HealthPotion,        20, Colors::potion_hp() },
    { "Greater Health Potion", "!",  ItemType::GreaterHealthPotion, 50, Colors::potion_hp() },
    { "Gold Coins",            "$",  ItemType::Gold,                 0, Colors::gold()      },
    { "Iron Sword",            "/",  ItemType::Weapon,               3, Colors::weapon()    },
    { "Battle Axe",            "P",  ItemType::Weapon,               5, Colors::weapon()    },
};
static constexpr int NUM_ITEM_TYPES = sizeof(ITEM_TEMPLATES) / sizeof(ITEM_TEMPLATES[0]);

// =============================================================================
// Game entities
// =============================================================================

struct Enemy {
    bool alive;
    int  x, y;
    int  hp, max_hp;
    int  atk, def;
    int  xp_reward;
    int  template_idx;
    bool alerted;         // has seen the player
    int  last_seen_px, last_seen_py;
};

struct Item {
    bool exists;
    int  x, y;
    int  template_idx;
    int  value;           // randomised (e.g. gold = rng(5,30))
};

struct Player {
    int  x, y;
    int  hp, max_hp;
    int  atk, def;
    int  level;
    int  xp, xp_next;
    int  gold;
    int  potions;
    int  floor;
    int  kills;
    bool dead;
};

// =============================================================================
// Message Log
// =============================================================================

struct LogEntry {
    std::string text;
    Color4 color;
    float timer;
};

static std::deque<LogEntry> g_log;

static void log_msg(const std::string& text, Color4 color = Color4::white()) {
    g_log.push_front({ text, color, LOG_FADE_TIME });
    if ((int)g_log.size() > MAX_LOG_LINES * 2) g_log.pop_back();
}

// =============================================================================
// Game State
// =============================================================================

struct GameState {
    DungeonMap  map;
    Player      player;
    Enemy       enemies[MAX_ENEMIES];
    Item        items[MAX_ITEMS];
    int         num_enemies = 0;
    int         num_items   = 0;
    bool        game_over   = false;
    bool        show_inventory = false;
    float       time        = 0.0f;
    float       hit_flash   = 0.0f;  // screen flash on player damage
    float       kill_flash  = 0.0f;  // flash on kill
    int         turn_count  = 0;
};

// =============================================================================
// Spawn helpers
// =============================================================================

static bool position_occupied(GameState& gs, int x, int y) {
    if (gs.player.x == x && gs.player.y == y) return true;
    for (int i = 0; i < gs.num_enemies; i++)
        if (gs.enemies[i].alive && gs.enemies[i].x == x && gs.enemies[i].y == y) return true;
    return false;
}

static void spawn_enemies(GameState& gs) {
    gs.num_enemies = 0;
    int to_spawn = std::min(MAX_ENEMIES, 3 + gs.player.floor * 2);

    for (int i = 0; i < to_spawn; i++) {
        // Pick a random room (skip the first room where player spawns)
        if (gs.map.rooms.size() < 2) break;
        int ri = rng_int(1, (int)gs.map.rooms.size() - 1);
        auto& room = gs.map.rooms[ri];

        int ex = rng_int(room.x + 1, room.x + room.w - 2);
        int ey = rng_int(room.y + 1, room.y + room.h - 2);
        if (!gs.map.is_walkable(ex, ey) || position_occupied(gs, ex, ey)) continue;

        // Pick enemy type based on floor
        std::vector<int> valid;
        for (int t = 0; t < NUM_ENEMY_TYPES; t++) {
            if (ENEMY_TEMPLATES[t].min_floor <= gs.player.floor)
                valid.push_back(t);
        }
        if (valid.empty()) continue;
        int ti = valid[rng_int(0, (int)valid.size() - 1)];
        auto& tmpl = ENEMY_TEMPLATES[ti];

        // Scale with floor
        float scale = 1.0f + gs.player.floor * 0.15f;

        Enemy e{};
        e.alive = true;
        e.x = ex; e.y = ey;
        e.max_hp = (int)(tmpl.hp * scale);
        e.hp = e.max_hp;
        e.atk = (int)(tmpl.atk * scale);
        e.def = (int)(tmpl.def * scale);
        e.xp_reward = (int)(tmpl.xp_reward * scale);
        e.template_idx = ti;
        e.alerted = false;

        gs.enemies[gs.num_enemies++] = e;
    }
}

static void spawn_items(GameState& gs) {
    gs.num_items = 0;
    int to_spawn = std::min(MAX_ITEMS, 4 + gs.player.floor);

    for (int i = 0; i < to_spawn; i++) {
        if (gs.map.rooms.size() < 2) break;
        int ri = rng_int(1, (int)gs.map.rooms.size() - 1);
        auto& room = gs.map.rooms[ri];

        int ix = rng_int(room.x, room.x + room.w - 1);
        int iy = rng_int(room.y, room.y + room.h - 1);
        if (!gs.map.is_walkable(ix, iy)) continue;

        // Pick item type
        int ti;
        float roll = rng_float(0.0f, 1.0f);
        if (roll < 0.35f) ti = 0;       // Health Potion
        else if (roll < 0.45f) ti = 1;  // Greater Health Potion
        else if (roll < 0.75f) ti = 2;  // Gold
        else if (roll < 0.90f) ti = 3;  // Iron Sword
        else ti = 4;                     // Battle Axe

        Item item{};
        item.exists = true;
        item.x = ix; item.y = iy;
        item.template_idx = ti;

        if (ITEM_TEMPLATES[ti].type == ItemType::Gold)
            item.value = rng_int(5, 25 + gs.player.floor * 10);
        else
            item.value = ITEM_TEMPLATES[ti].value;

        gs.items[gs.num_items++] = item;
    }
}

static void generate_floor(GameState& gs) {
    generate_dungeon(gs.map, gs.player.floor);

    // Place player in first room
    if (!gs.map.rooms.empty()) {
        auto& first = gs.map.rooms[0];
        gs.player.x = first.cx();
        gs.player.y = first.cy();
    }

    spawn_enemies(gs);
    spawn_items(gs);

    compute_fov(gs.map, gs.player.x, gs.player.y, FOV_RADIUS);

    std::string msg = "-- Floor " + std::to_string(gs.player.floor + 1) + " --";
    log_msg(msg, Colors::stairs_fg());
}

static void init_game(GameState& gs) {
    g_log.clear();

    gs.game_over = false;
    gs.show_inventory = false;
    gs.time = 0.0f;
    gs.hit_flash = 0.0f;
    gs.kill_flash = 0.0f;
    gs.turn_count = 0;

    Player& p = gs.player;
    p.hp = 30; p.max_hp = 30;
    p.atk = 8; p.def = 5;
    p.level = 1;
    p.xp = 0; p.xp_next = 50;
    p.gold = 0;
    p.potions = 2;
    p.floor = 0;
    p.kills = 0;
    p.dead = false;

    generate_floor(gs);

    log_msg("Welcome to Below. Descend and survive.", Color4::yellow());
    log_msg("WASD: move  E: interact  Q: potion", { 0.6f, 0.6f, 0.7f, 1.0f });
}

// =============================================================================
// Combat
// =============================================================================

static int calc_damage(int atk, int def) {
    int dmg = atk - def / 2 + rng_int(-2, 2);
    return std::max(1, dmg);
}

static void check_level_up(GameState& gs) {
    Player& p = gs.player;
    while (p.xp >= p.xp_next) {
        p.xp -= p.xp_next;
        p.level++;
        p.xp_next = (int)(p.xp_next * 1.5f);
        p.max_hp += 5;
        p.hp = std::min(p.hp + 10, p.max_hp);
        p.atk += 2;
        p.def += 1;
        log_msg("Level up! You are now level " + std::to_string(p.level) + "!", Color4::yellow());
    }
}

static void attack_enemy(GameState& gs, Enemy& e) {
    int dmg = calc_damage(gs.player.atk, e.def);
    e.hp -= dmg;
    e.alerted = true;

    auto& tmpl = ENEMY_TEMPLATES[e.template_idx];

    if (e.hp <= 0) {
        e.alive = false;
        gs.player.xp += e.xp_reward;
        gs.player.kills++;
        gs.kill_flash = 0.3f;
        log_msg("You slay the " + std::string(tmpl.name) + "! (+" +
                std::to_string(e.xp_reward) + " XP)", Colors::gold());
        check_level_up(gs);
    } else {
        log_msg("You hit the " + std::string(tmpl.name) + " for " +
                std::to_string(dmg) + " damage.", Color4::white());
    }
}

static void enemy_attacks_player(GameState& gs, Enemy& e) {
    int dmg = calc_damage(e.atk, gs.player.def);
    gs.player.hp -= dmg;
    gs.hit_flash = 0.25f;

    auto& tmpl = ENEMY_TEMPLATES[e.template_idx];
    log_msg("The " + std::string(tmpl.name) + " hits you for " +
            std::to_string(dmg) + "!", Colors::potion_hp());

    if (gs.player.hp <= 0) {
        gs.player.hp = 0;
        gs.player.dead = true;
        gs.game_over = true;
        log_msg("You have died on floor " + std::to_string(gs.player.floor + 1) +
                ".", Color4::red());
    }
}

// =============================================================================
// Enemy AI (simple chase)
// =============================================================================

static void update_enemies(GameState& gs) {
    for (int i = 0; i < gs.num_enemies; i++) {
        Enemy& e = gs.enemies[i];
        if (!e.alive) continue;

        int dx = gs.player.x - e.x;
        int dy = gs.player.y - e.y;
        int dist = abs(dx) + abs(dy);

        // Alert if player is visible and in range
        if (gs.map.visible[e.x][e.y] && dist <= 8) {
            e.alerted = true;
            e.last_seen_px = gs.player.x;
            e.last_seen_py = gs.player.y;
        }

        if (!e.alerted) continue;

        // Adjacent? Attack!
        if (dist == 1) {
            enemy_attacks_player(gs, e);
            continue;
        }

        // Chase: move toward player (or last known position)
        int tx = e.alerted ? gs.player.x : e.last_seen_px;
        int ty = e.alerted ? gs.player.y : e.last_seen_py;

        int mx = 0, my = 0;
        if (abs(tx - e.x) > abs(ty - e.y))
            mx = (tx > e.x) ? 1 : -1;
        else
            my = (ty > e.y) ? 1 : -1;

        int nx = e.x + mx;
        int ny = e.y + my;

        if (gs.map.is_walkable(nx, ny) && !position_occupied(gs, nx, ny)) {
            e.x = nx;
            e.y = ny;
        } else {
            // Try the other axis
            mx = 0; my = 0;
            if (abs(tx - e.x) <= abs(ty - e.y))
                mx = (tx > e.x) ? 1 : ((tx < e.x) ? -1 : 0);
            else
                my = (ty > e.y) ? 1 : ((ty < e.y) ? -1 : 0);

            nx = e.x + mx;
            ny = e.y + my;
            if (gs.map.is_walkable(nx, ny) && !position_occupied(gs, nx, ny)) {
                e.x = nx;
                e.y = ny;
            }
        }
    }
}

// =============================================================================
// Player actions
// =============================================================================

static bool try_move(GameState& gs, int dx, int dy) {
    int nx = gs.player.x + dx;
    int ny = gs.player.y + dy;

    // Check for enemy at destination -> attack
    for (int i = 0; i < gs.num_enemies; i++) {
        Enemy& e = gs.enemies[i];
        if (e.alive && e.x == nx && e.y == ny) {
            attack_enemy(gs, e);
            return true; // turn consumed
        }
    }

    if (!gs.map.is_walkable(nx, ny)) return false;

    gs.player.x = nx;
    gs.player.y = ny;
    return true;
}

static void try_pickup(GameState& gs) {
    // Check for stairs
    if (gs.map.at(gs.player.x, gs.player.y) == Tile::StairsDown) {
        gs.player.floor++;
        log_msg("You descend deeper...", Colors::stairs_fg());
        generate_floor(gs);
        return;
    }

    // Check for items
    for (int i = 0; i < gs.num_items; i++) {
        Item& item = gs.items[i];
        if (!item.exists) continue;
        if (item.x != gs.player.x || item.y != gs.player.y) continue;

        auto& tmpl = ITEM_TEMPLATES[item.template_idx];
        item.exists = false;

        switch (tmpl.type) {
            case ItemType::HealthPotion:
            case ItemType::GreaterHealthPotion:
                gs.player.potions++;
                log_msg("Picked up " + std::string(tmpl.name) + ".", tmpl.color);
                break;
            case ItemType::Gold:
                gs.player.gold += item.value;
                log_msg("Picked up " + std::to_string(item.value) + " gold.", tmpl.color);
                break;
            case ItemType::Weapon: {
                int bonus = item.value;
                gs.player.atk += bonus;
                log_msg("Equipped " + std::string(tmpl.name) + "! (+" +
                        std::to_string(bonus) + " ATK)", tmpl.color);
                break;
            }
        }
        return;
    }

    log_msg("Nothing to pick up here.", { 0.5f, 0.5f, 0.5f, 1.0f });
}

static void quaff_potion(GameState& gs) {
    if (gs.player.potions <= 0) {
        log_msg("No potions left!", Colors::potion_hp());
        return;
    }
    if (gs.player.hp >= gs.player.max_hp) {
        log_msg("Already at full health.", { 0.5f, 0.5f, 0.5f, 1.0f });
        return;
    }

    gs.player.potions--;
    int heal = 20 + gs.player.level * 5;
    gs.player.hp = std::min(gs.player.hp + heal, gs.player.max_hp);
    log_msg("You drink a potion. (+" + std::to_string(heal) + " HP)", Colors::potion_hp());
}

// =============================================================================
// Drawing helpers
// =============================================================================

static void draw_tile_char(VkRenderer& r, float wx, float wy, const char* ch,
                            Color4 color, float scale = 1.0f) {
    r.draw_text(wx + 2, wy + 2, ch, color, scale);
}

static Color4 dim_color(Color4 c, float factor) {
    return { c.r * factor, c.g * factor, c.b * factor, c.a };
}

// =============================================================================
// Rendering
// =============================================================================

static void render_map(VkRenderer& renderer, GameState& gs) {
    auto& cam = renderer.camera();
    
    // Only render tiles visible on screen
    int start_x = std::max(0, (int)(cam.left() / TILE_SIZE) - 1);
    int start_y = std::max(0, (int)(cam.top()  / TILE_SIZE) - 1);
    int end_x   = std::min(MAP_W, (int)(cam.right()  / TILE_SIZE) + 2);
    int end_y   = std::min(MAP_H, (int)(cam.bottom() / TILE_SIZE) + 2);

    for (int x = start_x; x < end_x; x++) {
        for (int y = start_y; y < end_y; y++) {
            float wx = (float)(x * TILE_SIZE);
            float wy = (float)(y * TILE_SIZE);

            Tile t = gs.map.at(x, y);

            if (gs.map.visible[x][y]) {
                // Fully visible - draw background + glyph
                Color4 bg, fg;
                const char* glyph = " ";
                switch (t) {
                    case Tile::Wall:
                        bg = Colors::bg_wall(); fg = Colors::wall_fg(); glyph = "#";
                        break;
                    case Tile::Floor:
                        bg = Colors::bg_floor(); fg = Colors::floor_fg(); glyph = ".";
                        break;
                    case Tile::StairsDown:
                        bg = Colors::bg_stairs(); fg = Colors::stairs_fg(); glyph = ">";
                        break;
                    case Tile::Water:
                        bg = Colors::bg_water(); fg = Colors::water_fg(); glyph = "~";
                        // Animate water
                        fg.b += sinf(gs.time * 2.0f + x * 0.5f + y * 0.3f) * 0.15f;
                        break;
                    case Tile::Door:
                        bg = Colors::bg_door(); fg = Colors::door_fg(); glyph = "+";
                        break;
                    default:
                        bg = Colors::bg_void(); fg = Colors::bg_void();
                        break;
                }
                renderer.draw_rect({ wx, wy, (float)TILE_SIZE, (float)TILE_SIZE }, bg);
                if (t != Tile::Void)
                    draw_tile_char(renderer, wx, wy, glyph, fg, 1.5f);

            } else if (gs.map.revealed[x][y]) {
                // Fog of war - dimmed
                Color4 bg;
                const char* glyph = " ";
                switch (t) {
                    case Tile::Wall:   bg = Colors::bg_wall();   glyph = "#"; break;
                    case Tile::Floor:  bg = Colors::bg_floor();  glyph = "."; break;
                    case Tile::StairsDown: bg = Colors::bg_stairs(); glyph = ">"; break;
                    case Tile::Water:  bg = Colors::bg_water();  glyph = "~"; break;
                    case Tile::Door:   bg = Colors::bg_door();   glyph = "+"; break;
                    default:           bg = Colors::bg_void();   break;
                }
                renderer.draw_rect({ wx, wy, (float)TILE_SIZE, (float)TILE_SIZE },
                                    dim_color(bg, 0.4f));
                if (t != Tile::Void)
                    draw_tile_char(renderer, wx, wy, glyph, Colors::fog(), 1.5f);
            }
        }
    }
}

static void render_items(VkRenderer& renderer, GameState& gs) {
    for (int i = 0; i < gs.num_items; i++) {
        Item& item = gs.items[i];
        if (!item.exists) continue;
        if (!gs.map.visible[item.x][item.y]) continue;

        auto& tmpl = ITEM_TEMPLATES[item.template_idx];
        float wx = (float)(item.x * TILE_SIZE);
        float wy = (float)(item.y * TILE_SIZE);

        // Slight bob animation
        float bob = sinf(gs.time * 3.0f + item.x + item.y) * 1.5f;
        draw_tile_char(renderer, wx, wy + bob, tmpl.glyph, tmpl.color, 1.5f);
    }
}

static void render_enemies(VkRenderer& renderer, GameState& gs) {
    for (int i = 0; i < gs.num_enemies; i++) {
        Enemy& e = gs.enemies[i];
        if (!e.alive) continue;
        if (!gs.map.visible[e.x][e.y]) continue;

        auto& tmpl = ENEMY_TEMPLATES[e.template_idx];
        float wx = (float)(e.x * TILE_SIZE);
        float wy = (float)(e.y * TILE_SIZE);

        // Draw entity background (slight tint)
        Color4 bg = { tmpl.color.r * 0.15f, tmpl.color.g * 0.15f, tmpl.color.b * 0.15f, 0.5f };
        renderer.draw_rect({ wx + 1, wy + 1, (float)TILE_SIZE - 2, (float)TILE_SIZE - 2 }, bg);

        // Draw glyph
        draw_tile_char(renderer, wx, wy, tmpl.glyph, tmpl.color, 1.5f);

        // Health bar if damaged
        if (e.hp < e.max_hp) {
            float ratio = (float)e.hp / (float)e.max_hp;
            float bar_w = (float)TILE_SIZE - 2;
            renderer.draw_rect({ wx + 1, wy - 2, bar_w, 2 }, Colors::hp_bar_bg());
            renderer.draw_rect({ wx + 1, wy - 2, bar_w * ratio, 2 }, Colors::hp_bar());
        }
    }
}

static void render_player(VkRenderer& renderer, GameState& gs) {
    float wx = (float)(gs.player.x * TILE_SIZE);
    float wy = (float)(gs.player.y * TILE_SIZE);

    // Pulsing glow behind player
    float pulse = 0.5f + sinf(gs.time * 4.0f) * 0.2f;
    Color4 glow = { 1.0f, 0.85f, 0.2f, pulse * 0.3f };
    renderer.draw_rect({ wx - 2, wy - 2, (float)TILE_SIZE + 4, (float)TILE_SIZE + 4 }, glow);

    // Player glyph
    draw_tile_char(renderer, wx, wy, "@", Colors::player(), 1.5f);
}

static void render_hud(VkRenderer& renderer, GameState& gs) {
    auto& cam = renderer.camera();
    float ui_x = cam.left() + 8;
    float ui_y = cam.top() + 8;
    float vw   = cam.viewport_width() / cam.zoom;

    // Top bar background
    renderer.draw_rect({ cam.left(), cam.top(), vw, 52 }, Colors::hud_bg());

    // Floor / Level
    char buf[256];
    snprintf(buf, sizeof(buf), "Floor %d  |  Lv.%d %s  |  Kills: %d",
             gs.player.floor + 1, gs.player.level,
             gs.player.dead ? "(DEAD)" : "",
             gs.player.kills);
    renderer.draw_text(ui_x, ui_y, buf, Color4::white(), 1.8f);

    // HP bar
    float bar_x = ui_x;
    float bar_y = ui_y + 20;
    float bar_w = 180;
    float bar_h = 10;
    float hp_ratio = (float)gs.player.hp / (float)gs.player.max_hp;

    renderer.draw_rect({ bar_x, bar_y, bar_w, bar_h }, Colors::hp_bar_bg());
    renderer.draw_rect({ bar_x, bar_y, bar_w * hp_ratio, bar_h }, Colors::hp_bar());

    snprintf(buf, sizeof(buf), "HP %d/%d", gs.player.hp, gs.player.max_hp);
    renderer.draw_text(bar_x + 2, bar_y + 1, buf, Color4::white(), 1.0f);

    // XP bar
    float xp_x = bar_x + bar_w + 10;
    float xp_ratio = (float)gs.player.xp / (float)gs.player.xp_next;
    renderer.draw_rect({ xp_x, bar_y, 120, bar_h }, Colors::xp_bar_bg());
    renderer.draw_rect({ xp_x, bar_y, 120 * xp_ratio, bar_h }, Colors::xp_bar());
    snprintf(buf, sizeof(buf), "XP %d/%d", gs.player.xp, gs.player.xp_next);
    renderer.draw_text(xp_x + 2, bar_y + 1, buf, Color4::white(), 1.0f);

    // Stats
    float stats_x = xp_x + 140;
    snprintf(buf, sizeof(buf), "ATK:%d  DEF:%d  Gold:%d  Potions:%d",
             gs.player.atk, gs.player.def, gs.player.gold, gs.player.potions);
    renderer.draw_text(stats_x, bar_y + 1, buf, { 0.8f, 0.8f, 0.8f, 1.0f }, 1.2f);

    // Message log (bottom of screen)
    float log_y = cam.bottom() - 12;
    int displayed = 0;
    for (auto& entry : g_log) {
        if (displayed >= MAX_LOG_LINES) break;
        float alpha = std::min(1.0f, entry.timer / 2.0f);
        if (alpha <= 0.0f) continue;

        Color4 c = entry.color;
        c.a = alpha;
        renderer.draw_text(ui_x, log_y, entry.text, c, 1.3f);
        log_y -= 14;
        displayed++;
    }

    // Game over overlay
    if (gs.game_over) {
        float cx = cam.x;
        float cy = cam.y;
        renderer.draw_rect({ cam.left(), cam.top(), vw,
                             cam.viewport_height() / cam.zoom },
                           { 0, 0, 0, 0.6f });

        renderer.draw_text(cx - 80, cy - 30, "YOU HAVE DIED", Color4::red(), 3.0f);

        snprintf(buf, sizeof(buf), "Floor %d  |  Level %d  |  %d kills  |  %d gold",
                 gs.player.floor + 1, gs.player.level, gs.player.kills, gs.player.gold);
        renderer.draw_text(cx - 120, cy + 10, buf, Color4::white(), 1.5f);
        renderer.draw_text(cx - 70, cy + 35, "Press R to restart", Color4::yellow(), 1.5f);
    }
}

static void render_inventory(VkRenderer& renderer, GameState& gs) {
    if (!gs.show_inventory) return;

    auto& cam = renderer.camera();
    float vw = cam.viewport_width() / cam.zoom;
    float vh = cam.viewport_height() / cam.zoom;
    float panel_w = 280;
    float panel_h = 200;
    float px = cam.x - panel_w / 2;
    float py = cam.y - panel_h / 2;

    // Background
    renderer.draw_rect({ px, py, panel_w, panel_h }, { 0.08f, 0.08f, 0.12f, 0.95f });
    // Border
    renderer.draw_rect({ px, py, panel_w, 2 }, Color4::white());
    renderer.draw_rect({ px, py + panel_h - 2, panel_w, 2 }, Color4::white());
    renderer.draw_rect({ px, py, 2, panel_h }, Color4::white());
    renderer.draw_rect({ px + panel_w - 2, py, 2, panel_h }, Color4::white());

    renderer.draw_text(px + 10, py + 8, "=== INVENTORY ===", Color4::yellow(), 2.0f);

    float iy = py + 35;
    char buf[128];

    snprintf(buf, sizeof(buf), "Potions: %d", gs.player.potions);
    renderer.draw_text(px + 10, iy, buf, Colors::potion_hp(), 1.5f);
    iy += 18;

    snprintf(buf, sizeof(buf), "Gold: %d", gs.player.gold);
    renderer.draw_text(px + 10, iy, buf, Colors::gold(), 1.5f);
    iy += 18;

    snprintf(buf, sizeof(buf), "Attack: %d", gs.player.atk);
    renderer.draw_text(px + 10, iy, buf, Colors::weapon(), 1.5f);
    iy += 18;

    snprintf(buf, sizeof(buf), "Defense: %d", gs.player.def);
    renderer.draw_text(px + 10, iy, buf, { 0.5f, 0.7f, 1.0f, 1.0f }, 1.5f);
    iy += 18;

    snprintf(buf, sizeof(buf), "Level: %d  (XP: %d/%d)", gs.player.level, gs.player.xp, gs.player.xp_next);
    renderer.draw_text(px + 10, iy, buf, Color4::white(), 1.5f);
    iy += 18;

    snprintf(buf, sizeof(buf), "Total kills: %d", gs.player.kills);
    renderer.draw_text(px + 10, iy, buf, Color4::white(), 1.5f);
    iy += 25;

    renderer.draw_text(px + 10, iy, "Press I to close", { 0.5f, 0.5f, 0.6f, 1.0f }, 1.2f);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    g_rng.seed(static_cast<unsigned>(time(nullptr)));

    VkRenderer renderer;
    renderer.init("Below - Vulkan Demo", 1280, 720, false, true);

    GameState gs;
    init_game(gs);

    // Centre camera on player
    renderer.camera().look_at(
        (float)(gs.player.x * TILE_SIZE) + TILE_SIZE / 2.0f,
        (float)(gs.player.y * TILE_SIZE) + TILE_SIZE / 2.0f
    );

    float dt = 1.0f / 60.0f;

    while (renderer.is_running()) {
        Key key = renderer.poll_input();
        if (key == Key::ESCAPE) break;

        // ---- Input ----
        bool turn_taken = false;

        if (!gs.game_over && !gs.show_inventory) {
            int dx = 0, dy = 0;
            if (key == Key::W || key == Key::UP)    dy = -1;
            if (key == Key::S || key == Key::DOWN)   dy =  1;
            if (key == Key::A || key == Key::LEFT)   dx = -1;
            if (key == Key::D || key == Key::RIGHT)  dx =  1;

            if (dx != 0 || dy != 0) {
                turn_taken = try_move(gs, dx, dy);
            }

            if (key == Key::E) {
                try_pickup(gs);
                turn_taken = true;
            }

            if (key == Key::Q) {
                quaff_potion(gs);
                turn_taken = true;
            }

            if (key == Key::I) {
                gs.show_inventory = !gs.show_inventory;
            }
        } else if (gs.show_inventory) {
            if (key == Key::I || key == Key::ESCAPE) {
                gs.show_inventory = false;
            }
        }

        if (gs.game_over && key == Key::R) {
            init_game(gs);
            renderer.camera().look_at(
                (float)(gs.player.x * TILE_SIZE) + TILE_SIZE / 2.0f,
                (float)(gs.player.y * TILE_SIZE) + TILE_SIZE / 2.0f
            );
        }

        // Camera zoom
        if (key == Key::PLUS)  renderer.camera().zoom *= 1.1f;
        if (key == Key::MINUS) renderer.camera().zoom *= 0.9f;

        // ---- Update ----
        if (turn_taken && !gs.game_over) {
            compute_fov(gs.map, gs.player.x, gs.player.y, FOV_RADIUS);
            update_enemies(gs);
            gs.turn_count++;
        }

        // Timers
        gs.time += dt;
        if (gs.hit_flash > 0) gs.hit_flash -= dt;
        if (gs.kill_flash > 0) gs.kill_flash -= dt;
        for (auto& entry : g_log) entry.timer -= dt;

        // Smooth camera follow
        float target_x = (float)(gs.player.x * TILE_SIZE) + TILE_SIZE / 2.0f;
        float target_y = (float)(gs.player.y * TILE_SIZE) + TILE_SIZE / 2.0f;
        renderer.camera().lerp_to(target_x, target_y, 0.15f);

        // ---- Render ----
        float cr = 0.02f, cg = 0.02f, cb = 0.04f;
        if (gs.hit_flash > 0) { cr = 0.3f; cg = 0.02f; cb = 0.02f; }
        if (gs.kill_flash > 0) { cr = 0.05f; cg = 0.1f; cb = 0.02f; }

        renderer.begin_frame(cr, cg, cb);

        render_map(renderer, gs);
        render_items(renderer, gs);
        render_enemies(renderer, gs);
        render_player(renderer, gs);
        render_hud(renderer, gs);
        render_inventory(renderer, gs);

        // Mini-map / stats line
        {
            auto& cam = renderer.camera();
            float sx = cam.right() - 200;
            float sy = cam.top() + 8;
            char stats[128];
            snprintf(stats, sizeof(stats), "Quads:%u  Draws:%u  Turn:%d",
                     renderer.last_quad_count(), renderer.last_draw_calls(),
                     gs.turn_count);
            renderer.draw_text(sx, sy, stats, { 0.4f, 0.4f, 0.5f, 1.0f }, 1.0f);
        }

        renderer.end_frame();
    }

    renderer.destroy();
    return 0;
}
