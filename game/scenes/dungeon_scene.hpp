#pragma once
#include "../../engine/scene/base_gameplay_scene.hpp"
#include "../../engine/world/world.hpp"
#include "../../engine/world/procedural_generator.hpp"
#include "../entities/entity_factory.hpp"
#include <ctime>
#include <algorithm>

// Game-specific dungeon scene with procedural generation based on dungeon type
class DungeonScene : public BaseGameplayScene<World, EntityFactory> {
private:
    int map_width = 80;
    int map_height = 60;
    std::unique_ptr<ProceduralGenerator> generator;
    std::vector<Room> rooms;
    
    // Exit positions
    int exit_up_x = -1, exit_up_y = -1;      // Stairs up (to previous floor / exit)
    int exit_down_x = -1, exit_down_y = -1;  // Stairs down (to next floor)
    
public:
    DungeonScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : BaseGameplayScene(con, mgr, rend, ui_sys) {}
    
    void on_enter() override {
        init_game();
    }
    
    void on_exit() override {
        // Queue player for transfer to preserve state across scene transitions
        if (transfer_manager && !transfer_manager->has_player_transfer()) {
            queue_player_for_transfer();
        }
        
        // Save world entities for persistence
        save_world_entities_to_dungeon_state();
    }
    
    // Required virtual: check if tile is walkable for this world type
    bool is_tile_walkable(int x, int y) override {
        if (x < 0 || x >= map_width || y < 0 || y >= map_height) {
            return false;
        }
        return world->get_tile(x, y).walkable;
    }
    
    // Override help content for dungeon-specific help
    std::vector<std::pair<std::string, std::vector<std::string>>> get_help_content() override {
        auto& state = game_state->dungeon;
        return {
            {"Movement:", {"WASD or Arrow Keys - Move"}},
            {"Commands:", {
                "E - Pickup items",
                "F - Interact with doors/chests/NPCs",
                "< - Ascend stairs (exit dungeon)",
                "> - Descend stairs (go deeper)",
                "I - Open Inventory",
                "C - Character Stats",
                "X - Examine nearby",
                "TAB - Faction relations",
                "H - Entity Wiki",
                "ESC - Pause game",
                "Q - Return to surface"
            }},
            {"Dungeon Info:", {
                "Dungeon: " + state.name,
                "Depth: " + std::to_string(state.depth) + "/" + std::to_string(state.max_depth),
                "Find < stairs to exit, > to go deeper"
            }}
        };
    }
    
    std::string get_help_title() override {
        return "Dungeon Help";
    }
    
    void init_game() {
        // Clear all entities from previous game
        manager->clear_all_entities();
        
        // Get dungeon state
        auto& state = game_state->dungeon;
        
        // Mark that we're in a dungeon (for save system)
        state.in_dungeon = true;
        
        // Use state dimensions or defaults
        map_width = state.active ? state.map_width : 80;
        map_height = state.active ? state.map_height : 60;
        
        // Create world
        world = std::make_unique<World>(map_width, map_height);
        
        // Use procedural generation with dungeon seed
        unsigned int seed = state.active ? state.seed + state.depth : static_cast<unsigned int>(time(nullptr));
        generator = std::make_unique<ProceduralGenerator>(seed);
        
        // Generate dungeon based on type
        generate_dungeon_by_type();
        
        // Get spawn position from first room
        auto [spawn_x, spawn_y] = generator->get_spawn_position(rooms);
        
        // Place exit stairs
        place_stairs();
        
        // Restore player from entity transfer system
        EntityId transferred_player = restore_player_from_transfer(spawn_x, spawn_y);
        
        if (transferred_player != 0) {
            // Player was restored from transfer
        } else {
            // No transfer pending - create new player (fresh dungeon entry)
            Entity player = factory->create_player(spawn_x, spawn_y, "Explorer");
            player_id = player.get_id();
        }
        
        // Clear loading flag if set
        state.loading_from_save = false;
        
        // Populate dungeon with enemies and loot
        int enemies_per_room = 2 + state.difficulty;
        int items_per_room = 1 + (state.depth > 2 ? 1 : 0);
        generator->populate_rooms(rooms, factory.get(), enemies_per_room, items_per_room);
        
        // Spawn extra enemies based on dungeon type
        spawn_type_specific_enemies();
        
        // Initialize AI system
        initialize_ai_system([this](int x, int y) {
            if (x < 0 || x >= map_width || y < 0 || y >= map_height) {
                return false;
            }
            return world->get_tile(x, y).walkable;
        });
        
        // Initialize message log
        std::string dungeon_name = state.active ? state.name : "Dungeon";
        init_message_log({
            "You enter " + dungeon_name + " - Floor " + std::to_string(state.depth),
            "Find the stairs down (>) to go deeper, or up (<) to exit.",
            "Press H for help, I for inventory, C for character."
        });
        
        if (state.depth == state.max_depth) {
            add_message("This is the deepest level. Find the treasure and escape!");
        }
    }
    
    void generate_dungeon_by_type() {
        auto& state = game_state->dungeon;
        DungeonType type = state.active ? state.type : DungeonType::STANDARD_DUNGEON;
        
        switch (type) {
            case DungeonType::CAVE:
                // Use cellular automata for organic caves
                generator->generate_cave(world.get(), 5, 0.45f);
                // Create room data from open areas for enemy placement
                rooms = find_open_areas();
                break;
                
            case DungeonType::MINE:
                // Generate mine-like structure with long corridors
                rooms = generator->generate_rooms(world.get(), 12, 3, 8);
                add_mine_decorations();
                break;
                
            case DungeonType::CATACOMBS:
                // Narrow corridors with small rooms
                rooms = generator->generate_rooms(world.get(), 15, 3, 6);
                add_catacomb_decorations();
                break;
                
            case DungeonType::LAIR:
                // Large central chamber with boss
                rooms = generate_boss_lair();
                break;
                
            case DungeonType::TOMB:
                // Linear tomb with dead ends
                rooms = generator->generate_rooms(world.get(), 10, 4, 8);
                add_tomb_decorations();
                break;
                
            default:  // STANDARD_DUNGEON
                rooms = generator->generate_rooms(world.get(), 8 + state.depth * 2, 5, 12);
                add_standard_dungeon_decorations();
                break;
        }
    }
    
    void add_standard_dungeon_decorations() {
        std::mt19937 rng(game_state->dungeon.seed + 400);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        auto& state = game_state->dungeon;
        
        // Add atmospheric details based on depth
        for (const Room& room : rooms) {
            // Water puddles (more common in deeper levels) - gray tones
            float puddle_chance = 0.1f + (state.depth * 0.02f);
            for (int y = room.y + 1; y < room.y + room.height - 1; ++y) {
                for (int x = room.x + 1; x < room.x + room.width - 1; ++x) {
                    Tile& tile = world->get_tile(x, y);
                    if (tile.walkable && dist(rng) < puddle_chance * 0.3f) {
                        tile.symbol = '~';
                        tile.color = "gray";
                    }
                }
            }
            
            // Crates and barrels in some rooms - gray non-interactables
            if (dist(rng) < 0.3f && room.width >= 5 && room.height >= 5) {
                int cx = room.x + 1;
                int cy = room.y + 1;
                if (world->get_tile(cx, cy).walkable) {
                    world->set_tile(cx, cy, Tile(false, '=', "gray"));  // Crate
                }
                if (dist(rng) < 0.5f && world->get_tile(cx + 1, cy).walkable) {
                    world->set_tile(cx + 1, cy, Tile(false, 'o', "gray"));  // Barrel
                }
            }
            
            // Moss/vegetation on floor (shows age/abandonment) - gray tones
            for (int y = room.y + 1; y < room.y + room.height - 1; ++y) {
                for (int x = room.x + 1; x < room.x + room.width - 1; ++x) {
                    Tile& tile = world->get_tile(x, y);
                    if (tile.walkable && tile.symbol == '.' && dist(rng) < 0.03f) {
                        tile.symbol = '"';
                        tile.color = "dark_gray";
                    }
                }
            }
            
            // Broken furniture - gray
            if (dist(rng) < 0.2f) {
                int fx = room.x + 1 + (rng() % std::max(1, room.width - 2));
                int fy = room.y + 1 + (rng() % std::max(1, room.height - 2));
                if (world->get_tile(fx, fy).walkable) {
                    world->set_tile(fx, fy, Tile(true, '%', "gray"));
                }
            }
        }
        
        // Add chains/shackles on walls occasionally (prison theme)
        if (state.depth >= 2 && dist(rng) < 0.4f) {
            for (const Room& room : rooms) {
                if (dist(rng) < 0.3f) {
                    int wx = room.x;
                    int wy = room.center_y();
                    Tile& wall = world->get_tile(wx, wy);
                    if (!wall.walkable) {
                        wall.symbol = '8';
                        wall.color = "gray";
                    }
                }
            }
        }
    }
    
    // Find open areas in cave-generated dungeons for room-like placement
    std::vector<Room> find_open_areas() {
        std::vector<Room> open_rooms;
        std::vector<std::vector<bool>> visited(map_height, std::vector<bool>(map_width, false));
        
        // Simple flood-fill to find connected walkable regions
        for (int y = 2; y < map_height - 2; y += 8) {
            for (int x = 2; x < map_width - 2; x += 8) {
                if (world->get_tile(x, y).walkable && !visited[y][x]) {
                    // Found an open area - create a virtual room
                    int size = random_int(4, 7);
                    open_rooms.push_back(Room(x - size/2, y - size/2, size, size));
                    
                    // Mark area as visited
                    for (int dy = -size; dy < size; ++dy) {
                        for (int dx = -size; dx < size; ++dx) {
                            int nx = x + dx, ny = y + dy;
                            if (nx >= 0 && nx < map_width && ny >= 0 && ny < map_height) {
                                visited[ny][nx] = true;
                            }
                        }
                    }
                }
            }
        }
        
        // Ensure at least one room for spawn
        if (open_rooms.empty()) {
            // Fallback - find any walkable spot
            for (int y = 5; y < map_height - 5; ++y) {
                for (int x = 5; x < map_width - 5; ++x) {
                    if (world->get_tile(x, y).walkable) {
                        open_rooms.push_back(Room(x - 2, y - 2, 5, 5));
                        return open_rooms;
                    }
                }
            }
        }
        
        return open_rooms;
    }
    
    // Generate a boss lair with large central chamber
    std::vector<Room> generate_boss_lair() {
        std::vector<Room> lair_rooms;
        std::mt19937 rng(game_state->dungeon.seed);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Fill with varied walls
        for (int y = 0; y < map_height; ++y) {
            for (int x = 0; x < map_width; ++x) {
                char wall_char = '#';
                std::string color = "dark_gray";
                if (dist(rng) < 0.1f) {
                    wall_char = '%';
                    color = "gray";
                }
                world->set_tile(x, y, Tile(false, wall_char, color));
            }
        }
        
        // Create entrance room
        Room entrance(5, map_height/2 - 4, 10, 8);
        carve_room(entrance);
        // Add braziers to entrance - gray (decorative)
        world->set_tile(entrance.x + 2, entrance.y + 2, Tile(false, '*', "gray"));
        world->set_tile(entrance.x + entrance.width - 3, entrance.y + 2, Tile(false, '*', "gray"));
        lair_rooms.push_back(entrance);
        
        // Create large central boss chamber
        int boss_x = map_width / 2 - 15;
        int boss_y = map_height / 2 - 12;
        Room boss_room(boss_x, boss_y, 30, 24);
        carve_room(boss_room);
        lair_rooms.push_back(boss_room);
        
        // Ornate floor pattern in boss room - gray tones
        for (int y = boss_y + 2; y < boss_y + 22; ++y) {
            for (int x = boss_x + 2; x < boss_x + 28; ++x) {
                Tile& tile = world->get_tile(x, y);
                if (tile.walkable) {
                    // Diamond/checkerboard pattern in grays
                    int cx = x - boss_x - 15;
                    int cy = y - boss_y - 12;
                    if ((std::abs(cx) + std::abs(cy)) % 4 == 0) {
                        tile.symbol = '+';
                        tile.color = "white";
                    } else if ((x + y) % 2 == 0) {
                        tile.color = "gray";
                    }
                }
            }
        }
        
        // Grand connecting corridor with carpet - gray tones
        int corridor_y = map_height / 2;
        for (int x = 15; x < boss_x; ++x) {
            world->set_tile(x, corridor_y, Tile(true, ':', "gray"));  // Gray carpet
            world->set_tile(x, corridor_y - 1, Tile(true, '.', "dark_gray"));
            world->set_tile(x, corridor_y + 1, Tile(true, '.', "dark_gray"));
            // Wall torches - keep yellow for visibility
            if ((x - 15) % 4 == 0) {
                world->set_tile(x, corridor_y - 2, Tile(false, '*', "yellow"));
                world->set_tile(x, corridor_y + 2, Tile(false, '*', "yellow"));
            }
        }
        
        // Grand pillars in boss room - gray
        for (int py = boss_y + 4; py < boss_y + 20; py += 4) {
            world->set_tile(boss_x + 5, py, Tile(false, 'O', "gray"));
            world->set_tile(boss_x + 24, py, Tile(false, 'O', "gray"));
        }
        
        // Center pillars forming arena - gray
        world->set_tile(boss_x + 10, boss_y + 6, Tile(false, 'O', "dark_gray"));
        world->set_tile(boss_x + 19, boss_y + 6, Tile(false, 'O', "dark_gray"));
        world->set_tile(boss_x + 10, boss_y + 17, Tile(false, 'O', "dark_gray"));
        world->set_tile(boss_x + 19, boss_y + 17, Tile(false, 'O', "dark_gray"));
        
        // Boss throne at far end - keep magenta (important)
        world->set_tile(boss_x + 14, boss_y + 2, Tile(false, '#', "gray"));
        world->set_tile(boss_x + 15, boss_y + 2, Tile(false, 'T', "magenta"));  // Throne
        world->set_tile(boss_x + 16, boss_y + 2, Tile(false, '#', "gray"));
        
        // Burning braziers near throne - gray (decorative)
        world->set_tile(boss_x + 12, boss_y + 3, Tile(false, '&', "gray"));
        world->set_tile(boss_x + 18, boss_y + 3, Tile(false, '&', "gray"));
        
        // Create treasure room behind boss
        Room treasure(boss_x + 32, map_height/2 - 5, 10, 10);
        carve_room(treasure);
        lair_rooms.push_back(treasure);
        
        // Treasure room decorations
        // Gold floor tiles
        for (int y = treasure.y + 1; y < treasure.y + treasure.height - 1; ++y) {
            for (int x = treasure.x + 1; x < treasure.x + treasure.width - 1; ++x) {
                Tile& tile = world->get_tile(x, y);
                tile.symbol = '$';
                tile.color = "yellow";
            }
        }
        // Central treasure pile
        world->set_tile(treasure.center_x(), treasure.center_y(), Tile(false, '*', "yellow"));
        
        // Connect boss to treasure with ornate passage
        for (int x = boss_x + 30; x < boss_x + 32; ++x) {
            world->set_tile(x, map_height / 2, Tile(true, ':', "yellow"));
            world->set_tile(x, map_height / 2 - 1, Tile(true, '.', "dark_gray"));
            world->set_tile(x, map_height / 2 + 1, Tile(true, '.', "dark_gray"));
        }
        
        // Side chambers with guards
        Room side1(boss_x - 12, boss_y + 4, 8, 8);
        Room side2(boss_x - 12, boss_y + 12, 8, 8);
        carve_room(side1);
        carve_room(side2);
        lair_rooms.push_back(side1);
        lair_rooms.push_back(side2);
        
        // Connect side chambers
        for (int y = side1.center_y(); y <= side2.center_y(); ++y) {
            world->set_tile(side1.x + side1.width - 1, y, Tile(true, '.', "dark_gray"));
        }
        create_horizontal_corridor(side1.center_x() + 4, boss_x, side1.center_y());
        create_horizontal_corridor(side2.center_x() + 4, boss_x, side2.center_y());
        
        return lair_rooms;
    }
    
    void create_horizontal_corridor(int x1, int x2, int y) {
        int start = std::min(x1, x2);
        int end = std::max(x1, x2);
        for (int x = start; x <= end; ++x) {
            world->set_tile(x, y, Tile(true, '.', "dark_gray"));
        }
    }
    
    void carve_room(const Room& room) {
        for (int y = room.y; y < room.y + room.height; ++y) {
            for (int x = room.x; x < room.x + room.width; ++x) {
                if (x >= 0 && x < map_width && y >= 0 && y < map_height) {
                    world->set_tile(x, y, Tile(true, '.', "dark_gray"));
                }
            }
        }
    }
    
    void add_mine_decorations() {
        std::mt19937 rng(game_state->dungeon.seed + 100);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Add mine rails, ore veins, and support beams
        for (const Room& room : rooms) {
            // Ore veins on walls
            for (int y = room.y; y < room.y + room.height; ++y) {
                for (int x = room.x; x < room.x + room.width; ++x) {
                    Tile& tile = world->get_tile(x, y);
                    if (!tile.walkable && dist(rng) < 0.15f) {
                        // Different ore types
                        float ore_type = dist(rng);
                        if (ore_type < 0.5f) {
                            tile.symbol = '*';
                            tile.color = "yellow";  // Gold ore
                        } else if (ore_type < 0.8f) {
                            tile.symbol = '*';
                            tile.color = "cyan";    // Copper/gems
                        } else {
                            tile.symbol = '*';
                            tile.color = "red";     // Ruby
                        }
                    }
                }
            }
            
            // Mine cart tracks - gray (non-interactable)
            if (room.width > room.height && dist(rng) < 0.4f) {
                int track_y = room.center_y();
                for (int x = room.x + 1; x < room.x + room.width - 1; ++x) {
                    if (world->get_tile(x, track_y).walkable) {
                        world->set_tile(x, track_y, Tile(true, '=', "gray"));
                    }
                }
                // Mine cart
                int cart_x = room.x + 2 + (rng() % std::max(1, room.width - 4));
                world->set_tile(cart_x, track_y, Tile(true, 'o', "dark_gray"));
            }
            
            // Support beams (wooden pillars) - gray
            if (room.width >= 5 && room.height >= 5 && dist(rng) < 0.5f) {
                world->set_tile(room.x + 1, room.center_y(), Tile(false, 'I', "gray"));
                world->set_tile(room.x + room.width - 2, room.center_y(), Tile(false, 'I', "gray"));
            }
            
            // Pickaxes and lanterns
            if (dist(rng) < 0.2f) {
                int lx = room.x + 1 + (rng() % std::max(1, room.width - 2));
                int ly = room.y + 1 + (rng() % std::max(1, room.height - 2));
                if (world->get_tile(lx, ly).walkable) {
                    world->set_tile(lx, ly, Tile(true, '?', "gray"));  // Lantern
                }
            }
        }
        
        // Collapsed sections
        for (int i = 0; i < 3; ++i) {
            int rx = 5 + (rng() % (map_width - 10));
            int ry = 5 + (rng() % (map_height - 10));
            if (world->get_tile(rx, ry).walkable) {
                // Small rubble pile
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dist(rng) < 0.4f) {
                            int nx = rx + dx, ny = ry + dy;
                            if (nx > 0 && nx < map_width && ny > 0 && ny < map_height) {
                                Tile& t = world->get_tile(nx, ny);
                                if (t.walkable) {
                                    t.symbol = ',';
                                    t.color = "gray";
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    void add_catacomb_decorations() {
        std::mt19937 rng(game_state->dungeon.seed + 200);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Add coffins, bones, crypts, and candles
        for (const Room& room : rooms) {
            // Bone piles scattered around
            int bone_count = 1 + (rng() % 3);
            for (int i = 0; i < bone_count; ++i) {
                int bx = room.x + 1 + (rng() % std::max(1, room.width - 2));
                int by = room.y + 1 + (rng() % std::max(1, room.height - 2));
                if (world->get_tile(bx, by).walkable) {
                    world->set_tile(bx, by, Tile(true, '%', "white"));
                }
            }
            
            // Stone coffins/sarcophagi along walls
            if (room.width >= 5 && dist(rng) < 0.6f) {
                // Line coffins against north wall
                for (int x = room.x + 2; x < room.x + room.width - 2; x += 3) {
                    if (world->get_tile(x, room.y + 1).walkable) {
                        world->set_tile(x, room.y + 1, Tile(false, '=', "gray"));  // Coffin
                    }
                }
            }
            
            // Skull decorations on walls
            for (int x = room.x; x < room.x + room.width; ++x) {
                Tile& top = world->get_tile(x, room.y);
                Tile& bot = world->get_tile(x, room.y + room.height - 1);
                if (!top.walkable && dist(rng) < 0.1f) {
                    top.symbol = '0';
                    top.color = "white";
                }
                if (!bot.walkable && dist(rng) < 0.1f) {
                    bot.symbol = '0';
                    bot.color = "white";
                }
            }
            
            // Candles
            if (dist(rng) < 0.4f) {
                int cx = room.x + 1 + (rng() % std::max(1, room.width - 2));
                int cy = room.y + 1 + (rng() % std::max(1, room.height - 2));
                if (world->get_tile(cx, cy).walkable) {
                    world->set_tile(cx, cy, Tile(true, 'i', "yellow"));
                }
            }
            
            // Cobwebs in corners
            if (dist(rng) < 0.5f) {
                world->set_tile(room.x, room.y, Tile(false, '~', "white"));
            }
            if (dist(rng) < 0.5f) {
                world->set_tile(room.x + room.width - 1, room.y, Tile(false, '~', "white"));
            }
        }
        
        // Eerie floor tiles
        for (int y = 1; y < map_height - 1; ++y) {
            for (int x = 1; x < map_width - 1; ++x) {
                Tile& tile = world->get_tile(x, y);
                if (tile.walkable && tile.symbol == '.' && dist(rng) < 0.05f) {
                    tile.symbol = ':';  // Cracked floor
                    tile.color = "dark_gray";
                }
            }
        }
    }
    
    void add_tomb_decorations() {
        std::mt19937 rng(game_state->dungeon.seed + 300);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Add sarcophagi, hieroglyphs, treasure, and traps
        for (const Room& room : rooms) {
            // Central sarcophagus in larger rooms - gray tones
            if (room.width >= 6 && room.height >= 6 && dist(rng) < 0.7f) {
                int sx = room.center_x();
                int sy = room.center_y();
                // Ornate sarcophagus
                world->set_tile(sx, sy, Tile(false, '&', "gray"));
                // Surrounding tiles
                world->set_tile(sx - 1, sy, Tile(true, ':', "dark_gray"));
                world->set_tile(sx + 1, sy, Tile(true, ':', "dark_gray"));
            }
            
            // Hieroglyphic walls - gray
            for (int y = room.y; y < room.y + room.height; ++y) {
                Tile& left = world->get_tile(room.x, y);
                Tile& right = world->get_tile(room.x + room.width - 1, y);
                if (!left.walkable && dist(rng) < 0.3f) {
                    left.symbol = '|';
                    left.color = "gray";
                }
                if (!right.walkable && dist(rng) < 0.3f) {
                    right.symbol = '|';
                    right.color = "gray";
                }
            }
            
            // Treasure urns - gray (non-interactable decor)
            if (dist(rng) < 0.3f) {
                int ux = room.x + 1 + (rng() % std::max(1, room.width - 2));
                int uy = room.y + 1 + (rng() % std::max(1, room.height - 2));
                if (world->get_tile(ux, uy).walkable) {
                    world->set_tile(ux, uy, Tile(true, 'U', "gray"));
                }
            }
            
            // Floor patterns - dusty tiles, gray tones
            for (int y = room.y + 1; y < room.y + room.height - 1; ++y) {
                for (int x = room.x + 1; x < room.x + room.width - 1; ++x) {
                    Tile& tile = world->get_tile(x, y);
                    if (tile.walkable) {
                        // Sandy/dusty floor
                        if (dist(rng) < 0.2f) {
                            tile.symbol = ',';
                            tile.color = "gray";
                        } else {
                            tile.color = "dark_gray";
                        }
                    }
                }
            }
            
            // Torch sconces - keep these bright (interactable visual cue)
            if (dist(rng) < 0.4f) {
                world->set_tile(room.x, room.center_y(), Tile(false, '*', "yellow"));
                world->set_tile(room.x + room.width - 1, room.center_y(), Tile(false, '*', "yellow"));
            }
        }
        
        // Ancient statues in corridors - gray
        for (int i = 0; i < 5; ++i) {
            int sx = 5 + (rng() % (map_width - 10));
            int sy = 5 + (rng() % (map_height - 10));
            if (world->get_tile(sx, sy).walkable) {
                // Check if in corridor (surrounded by some walls)
                int walls = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (!world->get_tile(sx + dx, sy + dy).walkable) walls++;
                    }
                }
                if (walls >= 4 && walls <= 6) {
                    world->set_tile(sx, sy, Tile(false, '@', "gray"));
                }
            }
        }
    }
    
    void place_stairs() {
        auto& state = game_state->dungeon;
        
        if (rooms.empty()) return;
        
        // Place stairs up in first room (exit / previous floor)
        const Room& first_room = rooms[0];
        exit_up_x = first_room.center_x() + 1;
        exit_up_y = first_room.center_y();
        world->set_tile(exit_up_x, exit_up_y, Tile(true, '<', "cyan"));
        
        // Place stairs down in last room (to next floor) if not at max depth
        if (state.depth < state.max_depth && rooms.size() > 1) {
            const Room& last_room = rooms[rooms.size() - 1];
            exit_down_x = last_room.center_x();
            exit_down_y = last_room.center_y();
            world->set_tile(exit_down_x, exit_down_y, Tile(true, '>', "magenta"));
        } else if (state.depth == state.max_depth && rooms.size() > 1) {
            // On the final floor, place a treasure chest instead
            const Room& last_room = rooms[rooms.size() - 1];
            int tx = last_room.center_x();
            int ty = last_room.center_y();
            
            // Create a special treasure chest
            auto chest = factory->create_chest(tx, ty, false);
            auto* chest_comp = chest.get_component<ChestComponent>();
            if (chest_comp) {
                // Add valuable loot
                Entity sword = factory->create_sword(tx, ty);
                chest_comp->add_item(sword.get_id());
                manager->remove_component<PositionComponent>(sword.get_id());
                
                for (int i = 0; i < 3; ++i) {
                    Entity potion = factory->create_potion(tx, ty);
                    chest_comp->add_item(potion.get_id());
                    manager->remove_component<PositionComponent>(potion.get_id());
                }
            }
        }
    }
    
    void spawn_type_specific_enemies() {
        auto& state = game_state->dungeon;
        if (!state.active || rooms.size() < 2) return;
        
        std::mt19937 rng(state.seed + state.depth * 1000);
        
        switch (state.type) {
            case DungeonType::CAVE:
                // Spawn bats and spiders
                for (size_t i = 1; i < rooms.size(); i += 2) {
                    const Room& room = rooms[i];
                    int x = room.x + 1 + (rng() % std::max(1, room.width - 2));
                    int y = room.y + 1 + (rng() % std::max(1, room.height - 2));
                    if (world->get_tile(x, y).walkable) {
                        if (rng() % 2 == 0) {
                            factory->create_spider(x, y, state.difficulty);
                        } else {
                            factory->spawn_enemy(x, y, 'b', "gray", "Cave Bat", "A screeching bat", 
                                state.difficulty, 8, 3, 1);
                        }
                    }
                }
                break;
                
            case DungeonType::MINE:
                // Spawn goblins and golems
                for (size_t i = 1; i < rooms.size(); i += 2) {
                    const Room& room = rooms[i];
                    int x = room.x + 1 + (rng() % std::max(1, room.width - 2));
                    int y = room.y + 1 + (rng() % std::max(1, room.height - 2));
                    if (world->get_tile(x, y).walkable) {
                        factory->create_enemy(x, y, "Goblin Miner", state.difficulty + 1);
                    }
                }
                break;
                
            case DungeonType::CATACOMBS:
                // Spawn undead
                for (size_t i = 1; i < rooms.size(); i += 2) {
                    const Room& room = rooms[i];
                    int x = room.x + 1 + (rng() % std::max(1, room.width - 2));
                    int y = room.y + 1 + (rng() % std::max(1, room.height - 2));
                    if (world->get_tile(x, y).walkable) {
                        factory->spawn_enemy(x, y, 's', "white", "Skeleton", "An undead warrior",
                            state.difficulty + 1, 15, 6, 4);
                    }
                }
                break;
                
            case DungeonType::LAIR:
                // Spawn the boss in the boss room
                if (rooms.size() >= 2) {
                    const Room& boss_room = rooms[1];
                    factory->spawn_enemy(boss_room.center_x(), boss_room.center_y(),
                        'D', "red", "Dragon", "A fearsome dragon!",
                        state.difficulty + 5, 100, 20, 12);
                    
                    // Dragon cultists
                    for (int i = 0; i < 3; ++i) {
                        int x = boss_room.x + 3 + (rng() % (boss_room.width - 6));
                        int y = boss_room.y + 3 + (rng() % (boss_room.height - 6));
                        factory->spawn_enemy(x, y, 'c', "red", "Dragon Cultist", "A zealous follower",
                            state.difficulty + 1, 20, 7, 4);
                    }
                }
                break;
                
            case DungeonType::TOMB:
                // Spawn mummies
                for (size_t i = 1; i < rooms.size(); i += 2) {
                    const Room& room = rooms[i];
                    int x = room.center_x();
                    int y = room.center_y() + 1;
                    if (world->get_tile(x, y).walkable) {
                        factory->spawn_enemy(x, y, 'm', "yellow", "Mummy", "An ancient guardian",
                            state.difficulty + 2, 25, 8, 6);
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    void update() override {
        // Sync player_id in case soul swap or similar effect changed it
        sync_player_id();
        
        check_player_death();
    }
    
    void process_turn() {
        // Sync player_id in case soul swap or similar effect changed it
        sync_player_id();
        
        // Process status effects for all entities (poison damage, regen, etc.)
        if (status_effect_system) {
            status_effect_system->process_turn();
        }
        
        // Called after player action - let enemies take their turn
        if (ai_system) {
            ai_system->update();
        }
    }
    
    // Override scene-specific action hooks
    void on_confirm_action() override { try_use_stairs_down(); }
    void on_secondary_action() override { try_use_stairs_up(); }
    void on_quit_action() override { exit_to_surface(); }
    
    void try_use_stairs_up() {
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;
        
        Tile tile = world->get_tile(pos->x, pos->y);
        if (tile.symbol == '<') {
            auto& state = game_state->dungeon;
            
            if (state.depth > 1) {
                // Go to previous floor
                state.depth--;
                
                // Queue player for transfer to next floor
                queue_player_for_transfer();
                
                add_message("You ascend to a higher floor...");
                init_game();  // Regenerate the floor
            } else {
                // Exit dungeon completely
                exit_to_surface();
            }
        } else {
            add_message("There are no stairs up here. Look for '<' symbols.");
        }
    }
    
    void try_use_stairs_down() {
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;
        
        Tile tile = world->get_tile(pos->x, pos->y);
        if (tile.symbol == '>') {
            auto& state = game_state->dungeon;
            
            if (state.depth < state.max_depth) {
                // Go to next floor
                state.depth++;
                
                // Queue player for transfer to next floor
                queue_player_for_transfer();
                
                add_message("You descend deeper into the darkness...");
                init_game();  // Generate the new floor
            } else {
                add_message("This is the deepest level. Find the treasure and escape via '<'!");
            }
        } else {
            add_message("There are no stairs down here. Look for '>' symbols.");
        }
    }
    
    void exit_to_surface() {
        // Show confirmation
        console->clear();
        ui->draw_panel(20, 10, 40, 8, "Exit Dungeon?", Color::YELLOW);
        console->draw_string(22, 12, "Return to the surface?", Color::WHITE);
        console->draw_string(22, 14, "[Y] Yes, leave dungeon", Color::GREEN);
        console->draw_string(22, 15, "[N] No, stay and explore", Color::RED);
        console->present();
        
        Key confirm;
        do {
            confirm = Input::wait_for_key();
        } while (confirm != Key::Y && confirm != Key::N && confirm != Key::ESCAPE);
        
        if (confirm == Key::Y) {
            // Transfer player to exploration scene
            queue_player_for_transfer();
            
            // Keep active=true so exploration scene knows we're returning from dungeon
            // (restore_from_dungeon will set it to false after restoring)
            
            // Return to exploration
            change_scene(SceneId::EXPLORATION);
        }
    }
    
    void render() override {
        console->clear();
        
        // Center camera on player
        renderer->center_camera_on_entity(player_id);
        
        // Render world and entities
        renderer->render_all(world.get());
        
        // Draw HUD with dungeon info
        ui->draw_hud(player_id, console->get_width(), console->get_height());
        
        // Draw dungeon floor indicator
        auto& state = game_state->dungeon;
        std::string floor_info = state.name + " - Floor " + std::to_string(state.depth) + "/" + std::to_string(state.max_depth);
        console->draw_string(console->get_width() - static_cast<int>(floor_info.length()) - 1, 0, floor_info, Color::CYAN);
        
        // Draw message log
        int log_height = 6;
        ui->draw_message_log(0, console->get_height() - log_height, 
                            console->get_width(), log_height, message_log, message_scroll, &KeyBindings::instance());
        
        console->present();
    }
};
