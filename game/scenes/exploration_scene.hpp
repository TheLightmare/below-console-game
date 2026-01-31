#pragma once
#include "../../engine/scene/base_gameplay_scene.hpp"
#include "../../engine/world/chunk.hpp"
#include "../../engine/world/chunk_generator.hpp"
#include "../entities/entity_factory.hpp"
#include <ctime>
#include <cmath>
#include <algorithm>
#include <random>

// Game-specific exploration scene with infinite chunked world
class ExplorationScene : public BaseGameplayScene<ChunkedWorld, EntityFactory> {
private:
    std::unique_ptr<ChunkGenerator> generator;
    
    const int CHUNK_LOAD_RADIUS = 3;  // Load chunks within 3 chunks of player
    const int CHUNK_UNLOAD_DISTANCE = 5;  // Unload chunks more than 5 chunks away
    
    // World map mode
    bool world_map_active = false;
    int map_camera_x = 0;
    int map_camera_y = 0;
    int map_zoom = 4;  // Each tile = zoom x zoom world tiles
    
    // Persistence flag - tracks if we have an existing world
    bool world_initialized = false;
    unsigned int world_seed = 0;
    
    // Helper: Load and generate chunks around a position
    void ensure_terrain_loaded(int x, int y) {
        world->ensure_chunks_loaded(x, y, CHUNK_LOAD_RADIUS);
        
        for (const auto& pair : world->get_chunks()) {
            Chunk* chunk = pair.second.get();
            if (!chunk->is_generated()) {
                generator->generate_terrain_chunk(chunk);
                generator->generate_structures_in_chunk(chunk, world.get());
                
                ChunkCoord coord = chunk->get_coord();
                
                // Check if this chunk was visited before
                if (is_chunk_visited(coord)) {
                    // Restore saved entities from previous visit
                    restore_chunk_entities(coord);
                } else {
                    // First time visiting - spawn fresh entities
                    spawn_enemies_in_chunk(chunk);
                    spawn_structure_enemies(chunk); // Spawn NPCs at structures including villages
                    mark_chunk_visited(coord);
                }
            }
        }
    }
    
public:
    ExplorationScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : BaseGameplayScene(con, mgr, rend, ui_sys) {}
    
    void on_enter() override {
        auto& dungeon_state = game_state->dungeon;
        
        // Mark that we're in exploration mode (for save system)
        dungeon_state.in_dungeon = false;
        
        // Check if loading from a saved game
        if (dungeon_state.loading_from_save) {
            load_from_save();
        }
        // Check if returning from dungeon with saved state
        // has_return_point is only set when entering a dungeon
        else if (dungeon_state.has_return_point && dungeon_state.active) {
            // Returning from dungeon - restore player at return position
            restore_from_dungeon();
        } else if (!world_initialized) {
            // First time initialization
            init_world();
        }
        // If world_initialized is true and not returning from dungeon, 
        // we're coming back from pause menu - keep current state
        
        // Always update world_seed in DungeonState for saving
        if (world_initialized) {
            dungeon_state.world_seed = world_seed;
        }
    }
    
    void restore_from_dungeon() {
        auto& dungeon_state = game_state->dungeon;
        
        // Clear entities but keep world
        manager->clear_all_entities();
        
        // If world wasn't initialized, we need to recreate it
        if (!world_initialized || !world) {
            world = std::make_unique<ChunkedWorld>(world_seed);
            generator = std::make_unique<ChunkGenerator>(world_seed);
            generator->load_structures_from_json("assets/structures");
            generator->load_worldgen_config("assets/");
        }
        
        // Ensure terrain is loaded around return point
        ensure_terrain_loaded(dungeon_state.return_x, dungeon_state.return_y);
        
        // Restore player from entity transfer system
        EntityId transferred_player = restore_player_from_transfer(dungeon_state.return_x, dungeon_state.return_y);
        
        if (transferred_player == 0) {
            // No transfer pending - this shouldn't happen in normal flow
            // Create a basic player as fallback
            Entity player = factory->create_player(dungeon_state.return_x, dungeon_state.return_y, "Explorer");
            player_id = player.get_id();
        }
        
        // Spawn NPCs if needed (they may have been cleared)
        spawn_starting_npcs(dungeon_state.return_x, dungeon_state.return_y);
        
        // Initialize AI system
        initialize_ai_system([this](int x, int y) { return world->is_walkable(x, y); });
        
        // Clear return point to prevent re-restoring
        dungeon_state.has_return_point = false;
        dungeon_state.active = false;  // No longer in/returning from dungeon
        dungeon_state.saved_inventory.clear();
        
        // Update messages
        init_message_log({
            "You emerge from the dungeon, back to the surface.",
        });
    }
    
    // Load game state from a saved file
    void load_from_save() {
        auto& dungeon_state = game_state->dungeon;
        
        // Clear all entities
        manager->clear_all_entities();
        
        // Use the saved world seed to recreate the same world
        world_seed = dungeon_state.world_seed;
        world = std::make_unique<ChunkedWorld>(world_seed);
        generator = std::make_unique<ChunkGenerator>(world_seed);
        generator->load_structures_from_json("assets/structures");
        generator->load_worldgen_config("assets/");
        
        // Store world seed back for future saves
        game_state->dungeon.world_seed = world_seed;
        
        // Mark world as initialized
        world_initialized = true;
        
        // IMPORTANT: Distribute saved_entities to chunk_entities for proper lazy loading
        // This prevents entities from being created in unloaded chunks
        distribute_saved_entities_to_chunks();
        
        // Ensure terrain is loaded around saved player position
        // This will also restore entities in loaded chunks via restore_chunk_entities
        int spawn_x = dungeon_state.player_x;
        int spawn_y = dungeon_state.player_y;
        ensure_terrain_loaded(spawn_x, spawn_y);
        
        // Restore player from entity transfer system (populated by save_load_window)
        EntityId transferred_player = restore_player_from_transfer(spawn_x, spawn_y);
        
        if (transferred_player == 0) {
            // No transfer available - create basic player (shouldn't happen normally)
            Entity player = factory->create_player(spawn_x, spawn_y, "Explorer");
            player_id = player.get_id();
        }
        
        // NOTE: Don't call restore_world_entities_from_dungeon_state() here
        // Entities are now handled by the chunk loading system via chunk_entities
        
        // Initialize AI system
        initialize_ai_system([this](int x, int y) { return world->is_walkable(x, y); });
        
        // Clear the loading flag
        dungeon_state.loading_from_save = false;
        
        // Initialize message log
        init_message_log({
            "Game loaded successfully.",
            "WASD=move, H=help"
        });
    }
    
    void on_exit() override {
        // Queue player for transfer to preserve state across scene transitions
        if (transfer_manager && !transfer_manager->has_player_transfer()) {
            queue_player_for_transfer();
        }
        
        // Save world entities for persistence
        save_world_entities_to_dungeon_state();
    }
    
    // Required virtual: check if tile is walkable for chunked world
    bool is_tile_walkable(int x, int y) override {
        Tile* tile = world->get_tile_ptr(x, y);
        return tile && tile->walkable;
    }
    
    // Hook: called after player moves
    void on_player_moved(int new_x, int new_y) override {
        // Show position info every 10 tiles
        if (std::abs(new_x) % 10 == 0 && std::abs(new_y) % 10 == 0) {
            add_message("Position: (" + std::to_string(new_x) + ", " + std::to_string(new_y) + ")");
        }
    }
    
    // Hook: called when movement is blocked
    void on_movement_blocked(int x, int y) override {
        Tile* tile = world->get_tile_ptr(x, y);
        if (tile && !tile->walkable) {
            add_message("You can't walk on that terrain!");
        } else {
            add_message("You can't move there!");
        }
    }
    
    // Override help content for exploration-specific help
    std::vector<std::pair<std::string, std::vector<std::string>>> get_help_content() override {
        return {
            {"Movement:", {"WASD or Arrow Keys - Move"}},
            {"Commands:", {
                "E - Pickup items",
                "F - Interact with nearby NPCs",
                "> or ENTER - Enter dungeon/cave entrance",
                "I - Open Inventory",
                "C - Character Stats",
                "M - World Map",
                "X - Examine nearby",
                "TAB - Faction relations",
                "H - Entity Wiki",
                "ESC - Pause game",
                "Q - Quit to menu"
            }},
            {"NPC Interactions (press F near NPCs):", {
                "Talk/Trade/Steal - Interact with NPCs"
            }},
            {"Terrain:", {
                "~ Water  . Grass  , Forest  ^ Hills  M Mountains",
                "> Dungeon entrance  # Buildings  + Doors"
            }}
        };
    }
    
    std::string get_help_title() override {
        return "Help - Exploration Mode";
    }
    
    void init_world() {
        // Reset dungeon state for new game (clears old save data, visited chunks, etc.)
        game_state->dungeon.reset();
        
        // Clear all entities from previous game
        manager->clear_all_entities();
        
        // Create chunked world
        world_seed = static_cast<unsigned int>(time(nullptr));
        world = std::make_unique<ChunkedWorld>(world_seed);
        generator = std::make_unique<ChunkGenerator>(world_seed);
        
        // Store world seed in game state for save/load
        game_state->dungeon.world_seed = world_seed;
        
        // Mark world as initialized
        world_initialized = true;
        
        // Load JSON structure definitions
        generator->load_structures_from_json("assets/structures");
        
        // Load data-driven worldgen configuration (biomes, features, etc.)
        generator->load_worldgen_config("assets/");
        
        // Spawn position - center of chunk (0,0) where starting hut is placed
        int spawn_x = CHUNK_SIZE / 2;
        int spawn_y = CHUNK_SIZE / 2;
        
        // Generate initial chunks around spawn
        ensure_terrain_loaded(spawn_x, spawn_y);
        
        // Find walkable spawn position
        if (!generator->find_walkable_position(world.get(), spawn_x, spawn_y, 50, spawn_x, spawn_y)) {
            // Fallback to origin if no walkable position found
            spawn_x = 0;
            spawn_y = 0;
        }
        
        // Create player at spawn
        Entity player = factory->create_player(spawn_x, spawn_y, "Explorer");
        player_id = player.get_id();
        

        // Give starting equipment (create next to player, they'll need to pick up)
        factory->create_sword(spawn_x + 1, spawn_y);
        factory->create_armor(spawn_x, spawn_y + 1);
        
        // Spawn some starting NPCs nearby
        spawn_starting_npcs(spawn_x, spawn_y);
        
        add_message("Your starting gear lies nearby - press E to pick up items!");
        
        // Initialize AI system
        initialize_ai_system([this](int x, int y) { return world->is_walkable(x, y); });
        
        // Initialize message log
        init_message_log({
            "Welcome to the infinite procedural wilderness!",
            "World generates as you explore - there are no boundaries!",
            "~ = Water, . = Sand/Grass, , = Forest, ^ = Hills, M = Mountains",
            "Use WASD or arrow keys to move. Press F to interact with NPCs.",
            "Press C for character stats, I for inventory, M for world map.",
            "Press ESC to pause."
        });
    }
    
    void update() override {
        // Sync player_id in case soul swap or similar effect changed it
        sync_player_id();
        
        check_player_death();
        
        // Update chunks around player position
        PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
        if (player_pos) {
            // Load chunks around player
            world->ensure_chunks_loaded(player_pos->x, player_pos->y, CHUNK_LOAD_RADIUS);
            
            // Generate terrain for any newly loaded chunks
            for (const auto& pair : world->get_chunks()) {
                Chunk* chunk = pair.second.get();
                if (!chunk->is_generated()) {
                    generator->generate_terrain_chunk(chunk);
                    // Use multi-chunk aware structure generation
                    generator->generate_structures_in_chunk(chunk, world.get());
                    
                    ChunkCoord coord = chunk->get_coord();
                    
                    // Check if this chunk was visited before
                    if (is_chunk_visited(coord)) {
                        // Restore saved entities from previous visit
                        restore_chunk_entities(coord);
                    } else {
                        // First time visiting - spawn fresh entities
                        spawn_enemies_in_chunk(chunk);
                        spawn_structure_enemies(chunk); // Spawn special enemies at structures
                        mark_chunk_visited(coord);
                    }
                }
            }
            
            // Unload distant chunks and their entities to save memory
            unload_distant_chunks_with_entities(player_pos->x, player_pos->y, CHUNK_UNLOAD_DISTANCE);
        }
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
    void on_map_action() override { toggle_world_map(); }
    void on_confirm_action() override { try_enter_dungeon(); }
    
    void handle_input(Key key) override {
        // World map mode input handling
        if (world_map_active) {
            handle_map_input(key);
            return;
        }
        
        // Use common input handler from base class
        handle_common_input(key);
    }
    
    void toggle_world_map() {
        world_map_active = !world_map_active;
        if (world_map_active) {
            // Center map on player
            PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
            if (pos) {
                map_camera_x = pos->x;
                map_camera_y = pos->y;
            }
        }
    }
    
    void handle_map_input(Key key) {
        int scroll_speed = map_zoom * 4;
        
        auto& bindings = KeyBindings::instance();
        
        if (key == Key::M || bindings.is_action(key, GameAction::CANCEL) || bindings.is_action(key, GameAction::MAP)) {
            world_map_active = false;
        } else if (bindings.is_action(key, GameAction::MOVE_UP)) {
            map_camera_y -= scroll_speed;
        } else if (bindings.is_action(key, GameAction::MOVE_DOWN)) {
            map_camera_y += scroll_speed;
        } else if (bindings.is_action(key, GameAction::MOVE_LEFT)) {
            map_camera_x -= scroll_speed;
        } else if (bindings.is_action(key, GameAction::MOVE_RIGHT)) {
            map_camera_x += scroll_speed;
        } else if (bindings.is_action(key, GameAction::ZOOM_IN)) {
            if (map_zoom > 1) map_zoom--;
        } else if (bindings.is_action(key, GameAction::ZOOM_OUT)) {
            if (map_zoom < 16) map_zoom++;
        } else if (key == Key::C) {
            // Center on player
            PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
            if (pos) {
                map_camera_x = pos->x;
                map_camera_y = pos->y;
            }
        }
    }
    
    void render() override {
        console->clear();
        
        if (world_map_active) {
            render_world_map();
            return;
        }
        
        // Center camera on player
        renderer->center_camera_on_entity(player_id);
        
        // Render world using chunked rendering
        renderer->render_chunks(world.get());
        
        // Draw HUD
        ui->draw_hud(player_id, console->get_width(), console->get_height());
        
        // Draw message log
        int log_height = 6;
        ui->draw_message_log(0, console->get_height() - log_height, 
                            console->get_width(), log_height, message_log, message_scroll, &KeyBindings::instance());
        
        console->present();
    }
    
    void render_world_map() {
        int screen_w = console->get_width();
        int screen_h = console->get_height();
        
        int map_height = screen_h - 4;
        int map_width = screen_w;
        
        int half_w = (map_width * map_zoom) / 2;
        int half_h = (map_height * map_zoom) / 2;
        
        int world_x_start = map_camera_x - half_w;
        int world_y_start = map_camera_y - half_h;
        
        PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
        int player_world_x = player_pos ? player_pos->x : 0;
        int player_world_y = player_pos ? player_pos->y : 0;
        
        for (int screen_y = 0; screen_y < map_height; ++screen_y) {
            for (int screen_x = 0; screen_x < map_width; ++screen_x) {
                int base_world_x = world_x_start + screen_x * map_zoom;
                int base_world_y = world_y_start + screen_y * map_zoom;
                
                // Check if player is in this tile
                bool has_player = false;
                if (player_pos) {
                    int px_screen = (player_world_x - world_x_start) / map_zoom;
                    int py_screen = (player_world_y - world_y_start) / map_zoom;
                    has_player = (px_screen == screen_x && py_screen == screen_y);
                }
                
                if (has_player) {
                    console->set_cell(screen_x, screen_y, '@', Color::WHITE);
                    continue;
                }
                
                // Multi-point sampling for better accuracy
                int sample_points = std::min(map_zoom, 4);
                int step = std::max(1, map_zoom / sample_points);
                
                // Track what we find across sample points
                int water_count = 0, river_count = 0, road_count = 0;
                int forest_count = 0, mountain_count = 0;
                BiomeType dominant_biome = BiomeType::PLAINS;
                FeatureType dominant_feature = FeatureType::NONE;
                float max_feature_intensity = 0.0f;
                bool found_structure = false;
                int struct_type = 0;
                
                for (int sy = 0; sy < sample_points && !found_structure; ++sy) {
                    for (int sx = 0; sx < sample_points && !found_structure; ++sx) {
                        int world_x = base_world_x + sx * step;
                        int world_y = base_world_y + sy * step;
                        
                        // Check for structure in loaded chunks
                        ChunkCoord chunk_coord = {world_x / CHUNK_SIZE, world_y / CHUNK_SIZE};
                        if (world_x < 0 && world_x % CHUNK_SIZE != 0) chunk_coord.x--;
                        if (world_y < 0 && world_y % CHUNK_SIZE != 0) chunk_coord.y--;
                        
                        if (world->has_chunk(chunk_coord)) {
                            Chunk* chunk = world->get_chunk(chunk_coord);
                            if (chunk->get_structure_type() > 0) {
                                int scx, scy;
                                chunk->get_structure_center(scx, scy);
                                int struct_world_x = chunk_coord.x * CHUNK_SIZE + scx;
                                int struct_world_y = chunk_coord.y * CHUNK_SIZE + scy;
                                int dist = std::abs(world_x - struct_world_x) + std::abs(world_y - struct_world_y);
                                if (dist < map_zoom * 2) {
                                    found_structure = true;
                                    struct_type = chunk->get_structure_type();
                                }
                            }
                        }
                        
                        auto terrain = generator->sample_terrain(world_x, world_y);
                        
                        // Count features
                        if (terrain.feature != FeatureType::NONE && terrain.feature_intensity > max_feature_intensity) {
                            dominant_feature = terrain.feature;
                            max_feature_intensity = terrain.feature_intensity;
                        }
                        if (terrain.feature == FeatureType::RIVER || terrain.feature == FeatureType::LAKE) river_count++;
                        if (terrain.feature == FeatureType::ROAD || terrain.feature == FeatureType::BRIDGE) road_count++;
                        
                        // Count biomes
                        if (terrain.biome == BiomeType::OCEAN || terrain.biome == BiomeType::DEEP_OCEAN || 
                            terrain.biome == BiomeType::LAKE) water_count++;
                        if (terrain.biome == BiomeType::FOREST || terrain.biome == BiomeType::DENSE_FOREST ||
                            terrain.biome == BiomeType::JUNGLE) forest_count++;
                        if (terrain.biome == BiomeType::MOUNTAINS || terrain.biome == BiomeType::SNOW_PEAKS) mountain_count++;
                        
                        if (sx == sample_points/2 && sy == sample_points/2) {
                            dominant_biome = terrain.biome;
                        }
                    }
                }
                
                int total_samples = sample_points * sample_points;
                
                // Render structure markers first
                if (found_structure) {
                    char sym = '*'; Color color = Color::CYAN;
                    if (struct_type == 3) { sym = 'V'; color = Color::YELLOW; }
                    else if (struct_type >= 6 && struct_type <= 8) { sym = 'D'; color = Color::RED; }
                    else if (struct_type >= 9 && struct_type <= 11) { sym = 'F'; color = Color::GRAY; }
                    console->set_cell(screen_x, screen_y, sym, color);
                    continue;
                }
                
                // Render features (rivers/roads) if significant presence
                if (river_count > 0) {
                    console->set_cell(screen_x, screen_y, '~', Color::CYAN);
                    continue;
                }
                if (road_count > total_samples / 3) {
                    console->set_cell(screen_x, screen_y, ':', Color::DARK_YELLOW);
                    continue;
                }
                
                // Render dominant biome
                char sym = '.';
                Color color = Color::GREEN;
                
                // Water dominance check
                if (water_count > total_samples / 2) {
                    sym = '~'; color = Color::BLUE;
                } else {
                    switch (dominant_biome) {
                        case BiomeType::OCEAN:           sym = '~'; color = Color::BLUE; break;
                        case BiomeType::DEEP_OCEAN:      sym = '~'; color = Color::DARK_BLUE; break;
                        case BiomeType::RIVER:           sym = '~'; color = Color::CYAN; break;
                        case BiomeType::LAKE:            sym = '~'; color = Color::BLUE; break;
                        case BiomeType::BEACH:           sym = '.'; color = Color::YELLOW; break;
                        case BiomeType::TROPICAL_BEACH:  sym = '.'; color = Color::YELLOW; break;
                        case BiomeType::PLAINS:          sym = '.'; color = Color::GREEN; break;
                        case BiomeType::SAVANNA:         sym = ','; color = Color::YELLOW; break;
                        case BiomeType::FOREST:          sym = 'T'; color = Color::GREEN; break;
                        case BiomeType::DENSE_FOREST:    sym = 'T'; color = Color::DARK_GREEN; break;
                        case BiomeType::JUNGLE:          sym = '#'; color = Color::GREEN; break;
                        case BiomeType::MUSHROOM_FOREST: sym = '*'; color = Color::MAGENTA; break;
                        case BiomeType::HILLS:           sym = '^'; color = Color::GRAY; break;
                        case BiomeType::MOUNTAINS:       sym = 'M'; color = Color::GRAY; break;
                        case BiomeType::SNOW_PEAKS:      sym = 'A'; color = Color::WHITE; break;
                        case BiomeType::VOLCANIC:        sym = '^'; color = Color::RED; break;
                        case BiomeType::DESERT:          sym = '.'; color = Color::YELLOW; break;
                        case BiomeType::DESERT_DUNES:    sym = '~'; color = Color::YELLOW; break;
                        case BiomeType::SWAMP:           sym = '"'; color = Color::DARK_GREEN; break;
                        case BiomeType::TUNDRA:          sym = '.'; color = Color::WHITE; break;
                        case BiomeType::FROZEN_TUNDRA:   sym = '*'; color = Color::CYAN; break;
                        case BiomeType::DEAD_LANDS:      sym = 'x'; color = Color::DARK_GRAY; break;
                        case BiomeType::ROAD:            sym = ':'; color = Color::DARK_YELLOW; break;
                        default:                         sym = '?'; color = Color::MAGENTA; break;
                    }
                }
                console->set_cell(screen_x, screen_y, sym, color);
            }
        }
        
        // Draw UI
        int ui_y = map_height;
        for (int i = 0; i < 4; ++i) {
            for (int x = 0; x < screen_w; ++x) {
                console->set_cell(x, ui_y + i, ' ', Color::BLACK);
            }
        }
        
        console->draw_string(1, ui_y, "=== WORLD MAP ===", Color::YELLOW);
        
        std::string pos_str = "View: (" + std::to_string(map_camera_x) + ", " + std::to_string(map_camera_y) + ")";
        if (player_pos) {
            pos_str += "  Player: (" + std::to_string(player_world_x) + ", " + std::to_string(player_world_y) + ")";
        }
        pos_str += "  Zoom: 1:" + std::to_string(map_zoom);
        console->draw_string(1, ui_y + 1, pos_str, Color::WHITE);
        
        console->draw_string(1, ui_y + 2, "[WASD] Scroll  [+/-] Zoom  [C] Center  [M/ESC] Close", Color::DARK_GRAY);
        
        int legend_x = screen_w - 50;
        console->draw_string(legend_x, ui_y + 1, "@ You  V Village  D Dungeon  ~ River/Water", Color::GRAY);
        console->draw_string(legend_x, ui_y + 2, "T Forest  M Mount  A Peak  : Road  ^ Hills", Color::DARK_GRAY);
        
        console->present();
    }
    
private:
    // Helper to convert world coordinates to chunk coordinates
    ChunkCoord world_to_chunk_coord(int world_x, int world_y) {
        int chunk_x = world_x / CHUNK_SIZE;
        int chunk_y = world_y / CHUNK_SIZE;
        
        // Handle negative coordinates properly
        if (world_x < 0 && world_x % CHUNK_SIZE != 0) chunk_x--;
        if (world_y < 0 && world_y % CHUNK_SIZE != 0) chunk_y--;
        
        return ChunkCoord(chunk_x, chunk_y);
    }
    
    // Unload distant chunks AND the entities that belong to them (saving them for later)
    void unload_distant_chunks_with_entities(int center_x, int center_y, int max_distance) {
        auto& state = game_state->dungeon;
        ChunkCoord center_chunk = world_to_chunk_coord(center_x, center_y);
        
        // First, collect chunks that need to be unloaded
        std::unordered_set<ChunkCoord> chunks_to_unload;
        for (const auto& pair : world->get_chunks()) {
            const ChunkCoord& coord = pair.first;
            int dx = coord.x - center_chunk.x;
            int dy = coord.y - center_chunk.y;
            int distance = dx * dx + dy * dy;
            
            if (distance > max_distance * max_distance) {
                chunks_to_unload.insert(coord);
            }
        }
        
        if (chunks_to_unload.empty()) return;
        
        // Build a set of protected entity IDs (items in inventories)
        std::unordered_set<EntityId> protected_entities;
        protected_entities.insert(player_id);
        
        // Protect all items in any inventory
        auto entities_with_inv = manager->get_entities_with_component<InventoryComponent>();
        for (EntityId inv_holder : entities_with_inv) {
            InventoryComponent* inv = manager->get_component<InventoryComponent>(inv_holder);
            if (inv) {
                for (EntityId item_id : inv->items) {
                    protected_entities.insert(item_id);
                }
            }
        }
        
        // Also protect equipped items
        auto entities_with_equip = manager->get_entities_with_component<EquipmentComponent>();
        for (EntityId equip_holder : entities_with_equip) {
            EquipmentComponent* equip = manager->get_component<EquipmentComponent>(equip_holder);
            if (equip) {
                for (const auto& [limb_name, item_id] : equip->equipped_items) {
                    if (item_id != 0) protected_entities.insert(item_id);
                }
                if (equip->back != 0) protected_entities.insert(equip->back);
                if (equip->neck != 0) protected_entities.insert(equip->neck);
                if (equip->waist != 0) protected_entities.insert(equip->waist);
            }
        }
        
        // Group entities by their chunk and serialize them
        std::unordered_map<ChunkCoord, std::vector<SerializedEntity>> chunk_entity_map;
        std::vector<EntityId> entities_to_destroy;
        auto entities_with_pos = manager->get_entities_with_component<PositionComponent>();
        
        for (EntityId entity_id : entities_with_pos) {
            // Skip protected entities (player and inventory items)
            if (protected_entities.count(entity_id)) continue;
            
            PositionComponent* pos = manager->get_component<PositionComponent>(entity_id);
            if (!pos) continue;
            
            ChunkCoord entity_chunk = world_to_chunk_coord(pos->x, pos->y);
            
            if (chunks_to_unload.count(entity_chunk)) {
                // Serialize the entity before destroying it
                chunk_entity_map[entity_chunk].push_back(serialize_entity(entity_id));
                entities_to_destroy.push_back(entity_id);
            }
        }
        
        // Save the serialized entities per chunk
        for (auto& pair : chunk_entity_map) {
            state.chunk_entities[pair.first] = std::move(pair.second);
        }
        
        // Destroy the entities
        for (EntityId entity_id : entities_to_destroy) {
            manager->destroy_entity(entity_id);
        }
        
        // Now unload the chunks
        world->unload_distant_chunks(center_x, center_y, max_distance);
    }
    
    // Restore entities that were saved when a chunk was previously unloaded
    void restore_chunk_entities(const ChunkCoord& coord) {
        auto& state = game_state->dungeon;
        
        auto it = state.chunk_entities.find(coord);
        if (it != state.chunk_entities.end()) {
            // Restore all saved entities for this chunk
            for (const auto& se : it->second) {
                deserialize_entity(se);
            }
            // Clear the saved entities for this chunk (they're now in the world)
            state.chunk_entities.erase(it);
        }
    }
    
    // Distribute saved_entities (flat list) to chunk_entities (per-chunk map) for lazy loading
    // This should be called when loading a save, BEFORE ensure_terrain_loaded
    void distribute_saved_entities_to_chunks() {
        auto& state = game_state->dungeon;
        
        // Move each saved entity to its appropriate chunk bucket
        for (const auto& se : state.saved_entities) {
            ChunkCoord chunk = world_to_chunk_coord(se.x, se.y);
            state.chunk_entities[chunk].push_back(se);
        }
        
        // Clear the flat list since entities are now in per-chunk buckets
        state.saved_entities.clear();
    }
    
    // Check if a chunk has been visited before (has saved entity data or is marked visited)
    bool is_chunk_visited(const ChunkCoord& coord) {
        auto& state = game_state->dungeon;
        // A chunk is visited if it's in visited_chunks OR has saved entities pending restoration
        return state.visited_chunks.count(coord) > 0 || 
               state.chunk_entities.count(coord) > 0;
    }
    
    // Mark a chunk as visited
    void mark_chunk_visited(const ChunkCoord& coord) {
        auto& state = game_state->dungeon;
        state.visited_chunks.insert(coord);
    }
    
    void spawn_enemies_in_chunk(Chunk* chunk) {
        ChunkCoord coord = chunk->get_coord();
        int world_x_start = coord.x * CHUNK_SIZE;
        int world_y_start = coord.y * CHUNK_SIZE;
        
        std::mt19937 rng(world->get_seed() ^ (coord.x * 73856093) ^ (coord.y * 19349663));
        std::uniform_int_distribution<int> enemy_count_dist(0, 1); // 0-1 enemies per chunk (much rarer)
        std::uniform_int_distribution<int> position_dist(0, CHUNK_SIZE - 1);
        std::uniform_int_distribution<int> enemy_type_dist(0, 3); // Only common enemies in wilderness
        
        int num_enemies = enemy_count_dist(rng);
        
        for (int i = 0; i < num_enemies; ++i) {
            int local_x = position_dist(rng);
            int local_y = position_dist(rng);
            int world_x = world_x_start + local_x;
            int world_y = world_y_start + local_y;
            
            // Check if tile is walkable
            Tile* tile = world->get_tile_ptr(world_x, world_y);
            if (!tile || !tile->walkable) continue;
            
            // Spawn different enemy types with varied symbols (only common wilderness enemies)
            int enemy_type = enemy_type_dist(rng);
            int level = 1 + (rng() % 3);
            
            switch (enemy_type) {
                case 0: // Goblin (common)
                    factory->create_enemy(world_x, world_y, "Goblin", level);
                    break;
                case 1: // Wolf (common)
                    factory->create_wolf(world_x, world_y, level);
                    break;
                case 2: // Spider (common)
                    factory->create_spider(world_x, world_y, level);
                    break;
                case 3: // Skeleton (uncommon in wilderness)
                    {
                        EntityId id = manager->create_entity();
                        Entity skeleton(id, manager);
                        skeleton.add_component(PositionComponent{world_x, world_y, 0});
                        skeleton.add_component(RenderComponent{'s', "white", "", 5});
                        skeleton.add_component(StatsComponent{level, 0, 12 * level, 12 * level, 5 + level, 3 + level});
                        skeleton.add_component(NameComponent{"Skeleton", "An undead warrior"});
                        skeleton.add_component(CollisionComponent{true, false});
                        skeleton.add_component(FactionComponent{FactionId::UNDEAD});
                        UtilityAIComponent utility_ai;
                        UtilityPresets::setup_undead(utility_ai);
                        skeleton.add_component(utility_ai);
                        factory->add_humanoid_anatomy(skeleton);
                        break;
                    }
            }
        }
    }
    
    void spawn_structure_enemies(Chunk* chunk) {
        int structure_type = chunk->get_structure_type();
        if (structure_type == -1) return; // No structure
        
        // Only spawn for the primary chunk (the one containing the structure center)
        if (!chunk->is_primary_structure_chunk()) return;
        
        ChunkCoord coord = chunk->get_coord();
        int world_x_start = coord.x * CHUNK_SIZE;
        int world_y_start = coord.y * CHUNK_SIZE;
        
        int local_cx, local_cy;
        chunk->get_structure_center(local_cx, local_cy);
        int world_cx = world_x_start + local_cx;
        int world_cy = world_y_start + local_cy;
        
        std::mt19937 rng(world->get_seed() ^ (coord.x * 73856093) ^ (coord.y * 19349663) ^ 12345);
        
        // Helper lambda to find walkable spawn position
        auto find_spawn_pos = [&](int offset_x, int offset_y, int search_radius = 3) -> std::pair<int, int> {
            int target_x = world_cx + offset_x;
            int target_y = world_cy + offset_y;
            
            // Try the exact position first
            Tile* tile = world->get_tile_ptr(target_x, target_y);
            if (tile && tile->walkable) {
                return {target_x, target_y};
            }
            
            // Search nearby for a walkable tile
            for (int attempt = 0; attempt < search_radius * search_radius * 4; ++attempt) {
                int ox = -search_radius + static_cast<int>(rng() % (search_radius * 2 + 1));
                int oy = -search_radius + static_cast<int>(rng() % (search_radius * 2 + 1));
                int sx = target_x + ox;
                int sy = target_y + oy;
                tile = world->get_tile_ptr(sx, sy);
                if (tile && tile->walkable) {
                    return {sx, sy};
                }
            }
            return {target_x, target_y};
        };
        
        // Get the structure definition from JSON
        const std::string& structure_def_id = chunk->get_structure_definition_id();
        const StructureDefinition* struct_def = nullptr;
        
        if (!structure_def_id.empty()) {
            struct_def = generator->get_structure_loader().get_structure(structure_def_id);
        }
        
        // If no JSON definition or no spawns defined, use fallback for special cases
        if (!struct_def || struct_def->spawns.empty()) {
            // Handle VILLAGE specially - it uses placed_buildings for villager spawning
            if (structure_type == 3) { // VILLAGE
                spawn_village_entities(chunk, world_cx, world_cy, rng);
            }
            return;
        }
        
        // Process JSON-defined spawns
        for (const auto& spawn : struct_def->spawns) {
            // Check spawn chance
            std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);
            if (chance_dist(rng) > spawn.chance) continue;
            
            // Determine spawn count
            int count = spawn.count_min;
            if (spawn.count_max > spawn.count_min) {
                count = spawn.count_min + static_cast<int>(rng() % (spawn.count_max - spawn.count_min + 1));
            }
            
            for (int i = 0; i < count; ++i) {
                // Calculate spawn position
                int spawn_x = spawn.offset_x;
                int spawn_y = spawn.offset_y;
                
                // If radius is specified, randomize within radius
                if (spawn.radius > 0) {
                    spawn_x += -spawn.radius + static_cast<int>(rng() % (spawn.radius * 2 + 1));
                    spawn_y += -spawn.radius + static_cast<int>(rng() % (spawn.radius * 2 + 1));
                }
                
                auto [sx, sy] = find_spawn_pos(spawn_x, spawn_y);
                
                // Determine level for this spawn (-1 means use template level, passed as 0 to factory)
                int level = 0;  // 0 = use template's default level
                if (spawn.level_min > 0) {
                    level = spawn.level_min;
                    if (spawn.level_max > spawn.level_min) {
                        level = spawn.level_min + static_cast<int>(rng() % (spawn.level_max - spawn.level_min + 1));
                    }
                }
                
                // Create entity based on spawn type
                if (spawn.type == "npc") {
                    // Use entity template system
                    factory->create_from_template(spawn.subtype, sx, sy, level, spawn.name);
                } else if (spawn.type == "enemy") {
                    factory->create_from_template(spawn.subtype, sx, sy, level, spawn.name);
                } else if (spawn.type == "creature") {
                    factory->create_from_template(spawn.subtype, sx, sy, level, spawn.name);
                }
            }
        }
        
        // For villages, also spawn villagers per house based on placed buildings
        if (structure_type == 3) { // VILLAGE
            spawn_village_entities(chunk, world_cx, world_cy, rng);
        }
    }
    
    // Spawn village-specific entities (villagers in houses, guards, etc.)
    void spawn_village_entities(Chunk* chunk, int world_cx, int world_cy, std::mt19937& rng) {
        const int village_spawn_radius = 35;
        
        // Helper lambda to find walkable spawn position
        auto find_spawn_pos = [&](int offset_range) -> std::pair<int, int> {
            int max_attempts = std::max(20, offset_range * 2);
            for (int attempt = 0; attempt < max_attempts; ++attempt) {
                int ox = -offset_range + static_cast<int>(rng() % (offset_range * 2 + 1));
                int oy = -offset_range + static_cast<int>(rng() % (offset_range * 2 + 1));
                int sx = world_cx + ox;
                int sy = world_cy + oy;
                Tile* tile = world->get_tile_ptr(sx, sy);
                if (tile && tile->walkable) {
                    return {sx, sy};
                }
            }
            return {world_cx, world_cy};
        };
        
        // Spawn villagers per house based on placed buildings
        const auto& placed_buildings = chunk->get_placed_buildings();
        std::vector<std::string> villager_names = {
            "Peasant", "Farmer", "Baker's Helper", "Miller", "Tanner",
            "Weaver", "Carpenter", "Potter", "Cobbler", "Butcher",
            "Fishwife", "Goodwife", "Elder", "Youth", "Laborer"
        };
        
        for (const auto& building : placed_buildings) {
            for (int v = 0; v < building.villagers_per_house; ++v) {
                int spawn_x = building.center_x;
                int spawn_y = building.center_y;
                
                // Find walkable tile near house center
                for (int attempt = 0; attempt < 20; ++attempt) {
                    int ox = -building.width/4 + static_cast<int>(rng() % (building.width/2 + 1));
                    int oy = -building.height/4 + static_cast<int>(rng() % (building.height/2 + 1));
                    int sx = building.center_x + ox;
                    int sy = building.center_y + oy;
                    Tile* tile = world->get_tile_ptr(sx, sy);
                    if (tile && tile->walkable) {
                        spawn_x = sx;
                        spawn_y = sy;
                        break;
                    }
                }
                
                // Get bed position
                int bed_x = 0, bed_y = 0;
                if (v < static_cast<int>(building.bed_positions.size())) {
                    int house_left = building.center_x - building.width / 2;
                    int house_top = building.center_y - building.height / 2;
                    bed_x = house_left + building.bed_positions[v].first;
                    bed_y = house_top + building.bed_positions[v].second;
                }
                
                std::string name = villager_names[rng() % villager_names.size()];
                factory->create_villager_with_home(
                    spawn_x, spawn_y,
                    building.center_x, building.center_y,
                    bed_x, bed_y,
                    building.structure_id,
                    name
                );
            }
        }
        
        // Additional village NPCs (guards, etc.) if not already spawned by building structures
        // These are spawned if the village doesn't have specific buildings with NPC spawns
        bool has_merchant = false;
        bool has_innkeeper = false;
        bool has_guards = false;
        
        // Check if buildings already spawned these
        for (const auto& building : placed_buildings) {
            if (building.structure_id == "shop" || building.structure_id == "market_stall") has_merchant = true;
            if (building.structure_id == "inn" || building.structure_id == "tavern") has_innkeeper = true;
            if (building.structure_id == "guardhouse") has_guards = true;
        }
        
        // Spawn missing essential NPCs
        if (!has_merchant) {
            auto [sx, sy] = find_spawn_pos(15);
            factory->create_merchant(sx, sy, "Village Merchant");
        }
        
        if (!has_innkeeper) {
            auto [sx, sy] = find_spawn_pos(20);
            factory->create_innkeeper(sx, sy);
        }
        
        if (!has_guards) {
            for (int i = 0; i < 5; ++i) {
                auto [sx, sy] = find_spawn_pos(village_spawn_radius);
                factory->create_guard(sx, sy, "Village Guard");
            }
        }
    }
    
    void spawn_starting_npcs(int spawn_x, int spawn_y) {
        // Spawn a few NPCs near the starting position to greet the player
        
        // Find walkable positions near spawn for NPCs
        std::vector<std::pair<int, int>> npc_positions;
        for (int dx = -5; dx <= 5; ++dx) {
            for (int dy = -5; dy <= 5; ++dy) {
                if (dx == 0 && dy == 0) continue;  // Skip player position
                if (std::abs(dx) <= 1 && std::abs(dy) <= 1) continue;  // Don't spawn right next to player
                
                int check_x = spawn_x + dx;
                int check_y = spawn_y + dy;
                
                Tile* tile = world->get_tile_ptr(check_x, check_y);
                if (tile && tile->walkable) {
                    npc_positions.push_back({check_x, check_y});
                }
            }
        }
        
        // Shuffle positions
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(npc_positions.begin(), npc_positions.end(), g);
        
        int npc_count = 0;
        
        // Spawn a merchant
        if (npc_count < static_cast<int>(npc_positions.size())) {
            auto pos = npc_positions[npc_count++];
            factory->create_merchant(pos.first, pos.second, "Traveling Merchant");
        }
        
        // Spawn a villager
        if (npc_count < static_cast<int>(npc_positions.size())) {
            auto pos = npc_positions[npc_count++];
            factory->create_villager(pos.first, pos.second, "Local Villager");
        }
        
        // Spawn a guard
        if (npc_count < static_cast<int>(npc_positions.size())) {
            auto pos = npc_positions[npc_count++];
            factory->create_guard(pos.first, pos.second, "Patrol Guard");
        }
        
        // Optionally spawn a mysterious stranger (50% chance)
        if (npc_count < static_cast<int>(npc_positions.size()) && chance(0.5f)) {
            auto pos = npc_positions[npc_count++];
            factory->create_mysterious_stranger(pos.first, pos.second);
        }
    }
    
    void try_enter_dungeon() {
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;
        
        // Get the tile the player is standing on
        Tile* tile = world->get_tile_ptr(pos->x, pos->y);
        if (!tile) return;
        
        // Check if it's a dungeon entrance tile (represented by '>')
        if (tile->symbol == '>') {
            // Get dungeon state
            auto& dungeon_state = game_state->dungeon;
            dungeon_state.reset();
            
            // Determine dungeon type based on nearby structure
            DungeonType dungeon_type = DungeonType::STANDARD_DUNGEON;
            std::string dungeon_name = "Unknown Depths";
            std::string entrance_desc = "You descend into darkness...";
            int base_difficulty = 1;
            
            // Check which chunk we're in and what structure spawned it
            ChunkCoord coord(pos->x / CHUNK_SIZE, pos->y / CHUNK_SIZE);
            if (pos->x < 0 && pos->x % CHUNK_SIZE != 0) coord.x--;
            if (pos->y < 0 && pos->y % CHUNK_SIZE != 0) coord.y--;
            
            Chunk* chunk = world->get_chunk(coord);
            if (chunk) {
                int structure_type = chunk->get_structure_type();
                switch (structure_type) {
                    case 6:  // DUNGEON_ENTRANCE
                        dungeon_type = DungeonType::STANDARD_DUNGEON;
                        dungeon_name = "Ancient Dungeon";
                        entrance_desc = "Stone steps lead deep underground. Ancient runes flicker faintly.";
                        base_difficulty = 2;
                        break;
                    case 7:  // CAVE_ENTRANCE
                        dungeon_type = DungeonType::CAVE;
                        dungeon_name = "Dark Cave";
                        entrance_desc = "A cold wind blows from the cave mouth. You hear distant dripping water.";
                        base_difficulty = 1;
                        break;
                    case 8:  // MINE_ENTRANCE
                        dungeon_type = DungeonType::MINE;
                        dungeon_name = "Abandoned Mine";
                        entrance_desc = "Rotting wooden supports line the tunnel. Ore veins glitter in the darkness.";
                        base_difficulty = 2;
                        break;
                    case 21: // ABANDONED_CASTLE
                        dungeon_type = DungeonType::CATACOMBS;
                        dungeon_name = "Castle Catacombs";
                        entrance_desc = "Crumbling steps descend into the castle's forgotten depths.";
                        base_difficulty = 3;
                        break;
                    case 22: // DRAGON_LAIR
                        dungeon_type = DungeonType::LAIR;
                        dungeon_name = "Dragon's Den";
                        entrance_desc = "Intense heat radiates from below. The smell of sulfur fills the air.";
                        base_difficulty = 5;
                        break;
                    case 25: // DESERT_TEMPLE
                        dungeon_type = DungeonType::TOMB;
                        dungeon_name = "Pharaoh's Tomb";
                        entrance_desc = "Hieroglyphics warn of curses awaiting those who disturb the dead.";
                        base_difficulty = 3;
                        break;
                    default:
                        dungeon_type = DungeonType::CAVE;
                        dungeon_name = "Mysterious Passage";
                        entrance_desc = "Darkness awaits below...";
                        base_difficulty = 1;
                        break;
                }
            }
            
            // Configure dungeon state
            dungeon_state.configure_for_type(dungeon_type, base_difficulty);
            dungeon_state.name = dungeon_name;
            dungeon_state.return_x = pos->x;
            dungeon_state.return_y = pos->y;
            dungeon_state.has_return_point = true;
            
            // Show confirmation dialog
            console->clear();
            ui->draw_panel(15, 8, 50, 14, "Enter " + dungeon_name + "?", Color::YELLOW);
            
            int y = 10;
            // Word wrap the description
            std::string desc = entrance_desc;
            while (desc.length() > 46) {
                size_t break_pos = desc.rfind(' ', 46);
                if (break_pos == std::string::npos) break_pos = 46;
                console->draw_string(17, y++, desc.substr(0, break_pos), Color::WHITE);
                desc = desc.substr(break_pos + 1);
            }
            console->draw_string(17, y++, desc, Color::WHITE);
            
            y++;
            console->draw_string(17, y++, "Dungeon Type: " + dungeon_name, Color::CYAN);
            console->draw_string(17, y++, "Floors: " + std::to_string(dungeon_state.max_depth), Color::GRAY);
            console->draw_string(17, y++, "Difficulty: " + std::to_string(dungeon_state.difficulty), Color::GRAY);
            y++;
            console->draw_string(17, y++, "WARNING: Dungeons contain dangerous enemies!", Color::RED);
            console->draw_string(17, y++, "You can exit by finding the stairs up (<).", Color::GRAY);
            y++;
            console->draw_string(25, y, "[Y] Enter   [N] Stay", Color::CYAN);
            console->present();
            
            // Wait for confirmation
            Key confirm;
            do {
                confirm = Input::wait_for_key();
            } while (confirm != Key::Y && confirm != Key::N && confirm != Key::ESCAPE);
            
            if (confirm == Key::Y) {
                // Mark dungeon as active
                dungeon_state.active = true;
                
                // Transfer player to dungeon scene
                queue_player_for_transfer();
                
                add_message("You steel yourself and descend into the " + dungeon_name + "...");
                
                // Transition to dungeon scene
                change_scene(SceneId::DUNGEON);  // "game" is registered as DungeonScene in main.cpp
            } else {
                add_message("You decide not to enter... for now.");
            }
        } else {
            add_message("There's no entrance here. Look for '>' symbols.");
        }
    }
};
