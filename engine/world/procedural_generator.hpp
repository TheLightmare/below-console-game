#pragma once
#include "perlin_noise.hpp"
#include "world.hpp"
#include "../ecs/entity.hpp"
#include <random>
#include <algorithm>
#include <vector>

struct Room {
    int x, y, width, height;
    
    Room(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    
    int center_x() const { return x + width / 2; }
    int center_y() const { return y + height / 2; }
    
    bool intersects(const Room& other) const {
        return x < other.x + other.width &&
               x + width > other.x &&
               y < other.y + other.height &&
               y + height > other.y;
    }
    
    bool contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

class ProceduralGenerator {
private:
    std::mt19937 rng;
    
public:
    ProceduralGenerator(unsigned int seed = 0) : rng(seed) {}
    
    void set_seed(unsigned int seed) {
        rng.seed(seed);
    }
    
    // Generate cave-like dungeon using cellular automata
    void generate_cave(World* world, int iterations = 5, float initial_wall_prob = 0.45f) {
        int width = world->get_width();
        int height = world->get_height();
        
        // Initialize with random walls
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                bool is_wall = dist(rng) < initial_wall_prob;
                world->set_tile(x, y, Tile(is_wall, is_wall ? '#' : '.', is_wall ? "dark_gray" : "brown"));
            }
        }
        
        // Cellular automata iterations
        for (int i = 0; i < iterations; ++i) {
            smooth_cave(world);
        }
        
        // Add visual variety to cave floors and walls
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                Tile& tile = world->get_tile(x, y);
                if (tile.walkable) {
                    // Vary floor appearance - use grays so items/interactables stand out
                    float r = dist(rng);
                    if (r < 0.05f) {
                        tile.symbol = ',';  // Gravel
                        tile.color = "dark_gray";
                    } else if (r < 0.08f) {
                        tile.symbol = '~';  // Puddle
                        tile.color = "gray";
                    } else if (r < 0.12f) {
                        tile.symbol = '"';  // Moss
                        tile.color = "gray";
                    } else {
                        tile.color = "dark_gray";
                    }
                } else {
                    // Vary wall appearance  
                    float r = dist(rng);
                    if (r < 0.1f) {
                        tile.symbol = '%';  // Rocky wall
                        tile.color = "dark_gray";
                    } else if (r < 0.15f) {
                        tile.color = "gray";
                    } else {
                        tile.color = "dark_gray";
                    }
                }
            }
        }
        
        // Add stalactites near walls
        for (int y = 2; y < height - 2; ++y) {
            for (int x = 2; x < width - 2; ++x) {
                if (world->get_tile(x, y).walkable && count_adjacent_walls(world, x, y) >= 3) {
                    if (dist(rng) < 0.15f) {
                        world->set_tile(x, y, Tile(true, '^', "gray"));  // Stalactite/stalagmite
                    }
                }
            }
        }
        
        // Add border walls
        for (int x = 0; x < width; ++x) {
            world->set_tile(x, 0, Tile(false, '#', "dark_gray"));
            world->set_tile(x, height - 1, Tile(false, '#', "dark_gray"));
        }
        for (int y = 0; y < height; ++y) {
            world->set_tile(0, y, Tile(false, '#', "dark_gray"));
            world->set_tile(width - 1, y, Tile(false, '#', "dark_gray"));
        }
    }
    
    // Smooth cave using cellular automata rules
    void smooth_cave(World* world) {
        int width = world->get_width();
        int height = world->get_height();
        
        std::vector<std::vector<bool>> new_map(height, std::vector<bool>(width));
        
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                int wall_count = count_adjacent_walls(world, x, y);
                
                // If 5 or more neighbors are walls, become wall
                // If 4 or fewer, become floor
                new_map[y][x] = wall_count >= 5;
            }
        }
        
        // Apply new map - use grays for non-interactable tiles
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                bool is_wall = new_map[y][x];
                world->set_tile(x, y, Tile(!is_wall, is_wall ? '#' : '.', "dark_gray"));
            }
        }
    }
    
    int count_adjacent_walls(World* world, int x, int y) {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                
                Tile tile = world->get_tile(x + dx, y + dy);
                if (!tile.walkable) count++;
            }
        }
        return count;
    }
    
    // Generate dungeon with rooms and corridors
    std::vector<Room> generate_rooms(World* world, int num_rooms, int min_size = 4, int max_size = 10) {
        int width = world->get_width();
        int height = world->get_height();
        
        // Initialize with walls - use varied wall appearance
        std::uniform_real_distribution<float> wall_dist(0.0f, 1.0f);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                char wall_char = '#';
                std::string wall_color = "dark_gray";
                float r = wall_dist(rng);
                if (r < 0.08f) {
                    wall_char = '%';  // Rough stone
                } else if (r < 0.12f) {
                    wall_color = "gray";
                }
                world->set_tile(x, y, Tile(false, wall_char, wall_color));
            }
        }
        
        std::vector<Room> rooms;
        std::uniform_int_distribution<int> size_dist(min_size, max_size);
        
        // Try to place rooms
        for (int i = 0; i < num_rooms * 3; ++i) {
            int room_w = size_dist(rng);
            int room_h = size_dist(rng);
            int room_x = rng() % (width - room_w - 2) + 1;
            int room_y = rng() % (height - room_h - 2) + 1;
            
            Room new_room(room_x, room_y, room_w, room_h);
            
            // Check for intersections (with padding)
            bool intersects = false;
            for (const Room& room : rooms) {
                Room padded(room.x - 1, room.y - 1, room.width + 2, room.height + 2);
                if (new_room.intersects(padded)) {
                    intersects = true;
                    break;
                }
            }
            
            if (!intersects) {
                carve_room(world, new_room);
                decorate_room(world, new_room);
                
                // Connect to previous room with corridor
                if (!rooms.empty()) {
                    const Room& prev = rooms.back();
                    if (rng() % 2 == 0) {
                        // Horizontal then vertical
                        create_horizontal_corridor(world, prev.center_x(), new_room.center_x(), prev.center_y());
                        create_vertical_corridor(world, prev.center_y(), new_room.center_y(), new_room.center_x());
                    } else {
                        // Vertical then horizontal
                        create_vertical_corridor(world, prev.center_y(), new_room.center_y(), prev.center_x());
                        create_horizontal_corridor(world, prev.center_x(), new_room.center_x(), new_room.center_y());
                    }
                }
                
                rooms.push_back(new_room);
                
                if (rooms.size() >= (size_t)num_rooms) break;
            }
        }
        
        // Add extra connections for loops (makes dungeon more interesting)
        if (rooms.size() >= 4) {
            int extra_connections = 1 + (rng() % 3);
            for (int i = 0; i < extra_connections; ++i) {
                size_t r1 = rng() % rooms.size();
                size_t r2 = rng() % rooms.size();
                if (r1 != r2 && std::abs((int)r1 - (int)r2) > 1) {
                    const Room& room1 = rooms[r1];
                    const Room& room2 = rooms[r2];
                    create_horizontal_corridor(world, room1.center_x(), room2.center_x(), room1.center_y());
                    create_vertical_corridor(world, room1.center_y(), room2.center_y(), room2.center_x());
                }
            }
        }
        
        return rooms;
    }
    
    void decorate_room(World* world, const Room& room) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        // Large rooms get pillars
        if (room.width >= 7 && room.height >= 7) {
            // Corner pillars
            world->set_tile(room.x + 1, room.y + 1, Tile(false, 'O', "gray"));
            world->set_tile(room.x + room.width - 2, room.y + 1, Tile(false, 'O', "gray"));
            world->set_tile(room.x + 1, room.y + room.height - 2, Tile(false, 'O', "gray"));
            world->set_tile(room.x + room.width - 2, room.y + room.height - 2, Tile(false, 'O', "gray"));
        }
        
        // Add torches near walls (30% chance per room)
        if (dist(rng) < 0.3f) {
            // Place torches on walls
            int torch_count = 2 + (rng() % 3);
            for (int t = 0; t < torch_count; ++t) {
                int side = rng() % 4;
                int tx, ty;
                switch (side) {
                    case 0: tx = room.x + 1 + (rng() % (room.width - 2)); ty = room.y; break;
                    case 1: tx = room.x + 1 + (rng() % (room.width - 2)); ty = room.y + room.height - 1; break;
                    case 2: tx = room.x; ty = room.y + 1 + (rng() % (room.height - 2)); break;
                    default: tx = room.x + room.width - 1; ty = room.y + 1 + (rng() % (room.height - 2)); break;
                }
                Tile& wall_tile = world->get_tile(tx, ty);
                if (!wall_tile.walkable) {
                    wall_tile.symbol = '*';
                    wall_tile.color = "yellow";
                }
            }
        }
        
        // Floor decorations - gray shades to keep items visible
        for (int y = room.y + 1; y < room.y + room.height - 1; ++y) {
            for (int x = room.x + 1; x < room.x + room.width - 1; ++x) {
                Tile& tile = world->get_tile(x, y);
                if (tile.walkable && tile.symbol == '.') {
                    float r = dist(rng);
                    if (r < 0.02f) {
                        tile.symbol = ',';  // Debris
                        tile.color = "gray";
                    } else if (r < 0.03f) {
                        tile.symbol = '`';  // Pebble
                        tile.color = "dark_gray";
                    }
                }
            }
        }
        
        // Checkered floor pattern for special rooms (10% chance)
        if (dist(rng) < 0.1f && room.width >= 5 && room.height >= 5) {
            for (int y = room.y + 1; y < room.y + room.height - 1; ++y) {
                for (int x = room.x + 1; x < room.x + room.width - 1; ++x) {
                    Tile& tile = world->get_tile(x, y);
                    if (tile.walkable) {
                        if ((x + y) % 2 == 0) {
                            tile.color = "dark_gray";
                        } else {
                            tile.color = "gray";
                        }
                    }
                }
            }
        }
    }
    
    void carve_room(World* world, const Room& room) {
        // Carve floor with slight gray variation - keeps items/interactables visible
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int y = room.y; y < room.y + room.height; ++y) {
            for (int x = room.x; x < room.x + room.width; ++x) {
                std::string floor_color = dist(rng) < 0.2f ? "gray" : "dark_gray";
                world->set_tile(x, y, Tile(true, '.', floor_color));
            }
        }
    }
    
    void create_horizontal_corridor(World* world, int x1, int x2, int y) {
        int start = (x1 < x2) ? x1 : x2;
        int end = (x1 > x2) ? x1 : x2;
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int x = start; x <= end; ++x) {
            // Main corridor
            std::string color = "dark_gray";
            if ((x - start) % 6 == 0 && dist(rng) < 0.5f) {
                // Occasional torch on wall above
                Tile& above = world->get_tile(x, y - 1);
                if (!above.walkable) {
                    above.symbol = '*';
                    above.color = "yellow";
                }
            }
            world->set_tile(x, y, Tile(true, '.', color));
            
            // Widen corridor occasionally
            if (dist(rng) < 0.15f) {
                world->set_tile(x, y - 1, Tile(true, '.', "dark_gray"));
            }
            if (dist(rng) < 0.15f) {
                world->set_tile(x, y + 1, Tile(true, '.', "dark_gray"));
            }
        }
    }
    
    void create_vertical_corridor(World* world, int y1, int y2, int x) {
        int start = (y1 < y2) ? y1 : y2;
        int end = (y1 > y2) ? y1 : y2;
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int y = start; y <= end; ++y) {
            std::string color = "dark_gray";
            if ((y - start) % 6 == 0 && dist(rng) < 0.5f) {
                // Occasional torch on wall to the left
                Tile& left = world->get_tile(x - 1, y);
                if (!left.walkable) {
                    left.symbol = '*';
                    left.color = "yellow";
                }
            }
            world->set_tile(x, y, Tile(true, '.', color));
            
            // Widen corridor occasionally
            if (dist(rng) < 0.15f) {
                world->set_tile(x - 1, y, Tile(true, '.', "dark_gray"));
            }
            if (dist(rng) < 0.15f) {
                world->set_tile(x + 1, y, Tile(true, '.', "dark_gray"));
            }
        }
    }
    
    // Generate terrain using Perlin noise
    void generate_terrain(World* world, unsigned int seed, double scale = 0.1, int octaves = 4) {
        PerlinNoise noise(seed);
        int width = world->get_width();
        int height = world->get_height();
        
        auto height_map = noise.generate_normalized_map(width, height, scale, octaves, 0.5);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double h = height_map[y][x];
                
                // Categorize terrain based on height
                if (h < 0.3) {
                    // Water
                    world->set_tile(x, y, Tile(false, '~', "blue"));
                } else if (h < 0.4) {
                    // Sand/Beach
                    world->set_tile(x, y, Tile(true, '.', "yellow"));
                } else if (h < 0.6) {
                    // Grass
                    world->set_tile(x, y, Tile(true, '.', "green"));
                } else if (h < 0.75) {
                    // Forest
                    world->set_tile(x, y, Tile(true, ',', "dark_green"));
                } else if (h < 0.85) {
                    // Rocky hills
                    world->set_tile(x, y, Tile(true, '^', "gray"));
                } else {
                    // Mountain peaks
                    world->set_tile(x, y, Tile(false, 'M', "white"));
                }
            }
        }
    }
    
    // Place entities in rooms (enemies, items, etc.)
    template<typename FactoryType>
    void populate_rooms(const std::vector<Room>& rooms, FactoryType* factory, 
                       int enemies_per_room = 2, int items_per_room = 1) {
        std::uniform_int_distribution<int> enemy_count_dist(1, enemies_per_room);
        std::uniform_int_distribution<int> item_count_dist(0, items_per_room);
        std::uniform_int_distribution<int> enemy_type_dist(1, 3);
        std::uniform_int_distribution<int> chest_chance(1, 100);
        
        // Skip first room (player spawn)
        for (size_t i = 1; i < rooms.size(); ++i) {
            const Room& room = rooms[i];
            
            // Spawn enemies
            int enemy_count = enemy_count_dist(rng);
            for (int e = 0; e < enemy_count; ++e) {
                int ex = room.x + 1 + (rng() % (room.width - 2));
                int ey = room.y + 1 + (rng() % (room.height - 2));
                
                int enemy_level = enemy_type_dist(rng);
                const char* enemy_names[] = {"Goblin", "Orc", "Troll"};
                factory->create_enemy(ex, ey, enemy_names[enemy_level - 1], enemy_level);
            }
            
            // 40% chance to spawn a chest with loot
            if (chest_chance(rng) <= 40) {
                int cx = room.x + 1 + (rng() % (room.width - 2));
                int cy = room.y + 1 + (rng() % (room.height - 2));
                
                auto chest = factory->create_chest(cx, cy, false);
                auto* chest_comp = chest.template get_component<ChestComponent>();
                
                if (chest_comp) {
                    // Add random loot to chest (1-3 items)
                    int loot_count = 1 + (rng() % 3);
                    for (int l = 0; l < loot_count; ++l) {
                        int loot_type = rng() % 6;
                        Entity loot_item(0, nullptr);
                        
                        // Create items at chest position (will be stored inside)
                        switch (loot_type) {
                            case 0:
                                loot_item = factory->create_potion(cx, cy);
                                break;
                            case 1:
                                loot_item = factory->create_sword(cx, cy);
                                break;
                            case 2:
                                loot_item = factory->create_helmet(cx, cy);
                                break;
                            case 3:
                                loot_item = factory->create_armor(cx, cy);
                                break;
                            case 4:
                                loot_item = factory->create_boots(cx, cy);
                                break;
                            case 5:
                                loot_item = factory->create_shield(cx, cy);
                                break;
                        }
                        
                        // Add item to chest inventory
                        chest_comp->add_item(loot_item.get_id());
                        
                        // Remove position component so item doesn't render on ground
                        loot_item.get_manager()->remove_component<PositionComponent>(loot_item.get_id());
                    }
                }
            }
            
            // Spawn floor items
            int item_count = item_count_dist(rng);
            for (int it = 0; it < item_count; ++it) {
                int ix = room.x + 1 + (rng() % (room.width - 2));
                int iy = room.y + 1 + (rng() % (room.height - 2));
                
                int item_type = rng() % 5; // Random equipment type
                switch (item_type) {
                    case 0:
                        factory->create_potion(ix, iy);
                        break;
                    case 1:
                        factory->create_sword(ix, iy);
                        break;
                    case 2:
                        factory->create_helmet(ix, iy);
                        break;
                    case 3:
                        factory->create_armor(ix, iy);
                        break;
                    case 4:
                        factory->create_boots(ix, iy);
                        break;
                }
            }
        }
    }
    
    // Get random spawn position in first room
    std::pair<int, int> get_spawn_position(const std::vector<Room>& rooms) {
        if (rooms.empty()) return {0, 0};
        
        const Room& first_room = rooms[0];
        return {first_room.center_x(), first_room.center_y()};
    }
};
