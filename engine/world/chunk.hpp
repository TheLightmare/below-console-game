#pragma once
#include "world.hpp"
#include "chunk_coord.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <cmath>

// Planned multi-chunk structure (stored in world coordinates)
struct PlannedStructure {
    int id;                     // Unique structure ID
    int world_x, world_y;       // World position of structure center
    int width, height;          // Total size in tiles
    int type;                   // StructureType enum value
    unsigned int structure_seed; // Seed for deterministic generation
    std::string structure_id;   // JSON structure definition ID (e.g., "shop", "tavern")
    
    PlannedStructure() : id(0), world_x(0), world_y(0), width(0), height(0), type(-1), structure_seed(0) {}
    
    PlannedStructure(int id, int wx, int wy, int w, int h, int t, unsigned int s, const std::string& sid = "")
        : id(id), world_x(wx), world_y(wy), width(w), height(h), type(t), structure_seed(s), structure_id(sid) {}
    
    // Get bounding box in world coordinates
    int min_x() const { return world_x - width / 2; }
    int max_x() const { return world_x + width / 2; }
    int min_y() const { return world_y - height / 2; }
    int max_y() const { return world_y + height / 2; }
    
    // Check if this structure overlaps a chunk
    bool overlaps_chunk(int chunk_x, int chunk_y) const {
        int chunk_min_x = chunk_x * CHUNK_SIZE;
        int chunk_max_x = chunk_min_x + CHUNK_SIZE - 1;
        int chunk_min_y = chunk_y * CHUNK_SIZE;
        int chunk_max_y = chunk_min_y + CHUNK_SIZE - 1;
        
        return !(max_x() < chunk_min_x || min_x() > chunk_max_x ||
                 max_y() < chunk_min_y || min_y() > chunk_max_y);
    }
    
    // Get which chunks this structure spans
    std::vector<ChunkCoord> get_affected_chunks() const {
        std::vector<ChunkCoord> chunks;
        
        int min_chunk_x = min_x() / CHUNK_SIZE;
        int max_chunk_x = max_x() / CHUNK_SIZE;
        int min_chunk_y = min_y() / CHUNK_SIZE;
        int max_chunk_y = max_y() / CHUNK_SIZE;
        
        // Handle negative coordinates
        if (min_x() < 0 && min_x() % CHUNK_SIZE != 0) min_chunk_x--;
        if (max_x() < 0 && max_x() % CHUNK_SIZE != 0) max_chunk_x--;
        if (min_y() < 0 && min_y() % CHUNK_SIZE != 0) min_chunk_y--;
        if (max_y() < 0 && max_y() % CHUNK_SIZE != 0) max_chunk_y--;
        
        for (int cy = min_chunk_y; cy <= max_chunk_y; ++cy) {
            for (int cx = min_chunk_x; cx <= max_chunk_x; ++cx) {
                chunks.push_back(ChunkCoord(cx, cy));
            }
        }
        return chunks;
    }
};

// Info about a placed building in a chunk (for NPC spawning)
struct PlacedBuildingInfo {
    std::string structure_id;
    int center_x;
    int center_y;
    int width;
    int height;
    int villagers_per_house = 1;  // How many villagers spawn per house
    std::vector<std::pair<int, int>> bed_positions;  // Bed positions relative to center
    
    PlacedBuildingInfo() = default;
    PlacedBuildingInfo(const std::string& id, int cx, int cy, int w, int h)
        : structure_id(id), center_x(cx), center_y(cy), width(w), height(h) {}
};

// Single chunk of the world
class Chunk {
private:
    std::vector<std::vector<Tile>> tiles;
    ChunkCoord coord;
    bool generated;
    int structure_type; // -1 = none, 0 = ruins, 1 = camp, 2 = stone circle
    std::string structure_definition_id; // JSON structure ID (e.g., "shop", "tavern")
    int structure_center_x;
    int structure_center_y;
    std::vector<PlacedBuildingInfo> placed_buildings;  // Houses and other buildings for NPC spawning
    bool is_primary_structure = false;  // True if this chunk contains the structure's actual center
    
public:
    Chunk(ChunkCoord coord) 
        : tiles(CHUNK_SIZE, std::vector<Tile>(CHUNK_SIZE)), 
          coord(coord), 
          generated(false),
          structure_type(-1),
          structure_definition_id(""),
          structure_center_x(0),
          structure_center_y(0),
          is_primary_structure(false) {}
    
    Tile& get_tile(int local_x, int local_y) {
        return tiles[local_y][local_x];
    }
    
    void set_tile(int local_x, int local_y, const Tile& tile) {
        tiles[local_y][local_x] = tile;
    }
    
    bool is_generated() const { return generated; }
    void set_generated(bool gen) { generated = gen; }
    
    ChunkCoord get_coord() const { return coord; }
    
    // Structure data
    void set_structure_type(int type) { structure_type = type; }
    int get_structure_type() const { return structure_type; }
    void set_structure_definition_id(const std::string& id) { structure_definition_id = id; }
    const std::string& get_structure_definition_id() const { return structure_definition_id; }
    void set_structure_center(int cx, int cy) { structure_center_x = cx; structure_center_y = cy; }
    void get_structure_center(int& cx, int& cy) const { cx = structure_center_x; cy = structure_center_y; }
    void set_primary_structure(bool primary) { is_primary_structure = primary; }
    bool is_primary_structure_chunk() const { return is_primary_structure; }
    
    // Building data for NPC spawning
    void add_placed_building(const PlacedBuildingInfo& building) {
        placed_buildings.push_back(building);
    }
    
    const std::vector<PlacedBuildingInfo>& get_placed_buildings() const {
        return placed_buildings;
    }
    
    void clear_placed_buildings() {
        placed_buildings.clear();
    }
};

// Chunked world manager
class ChunkedWorld {
private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> chunks;
    unsigned int seed;
    
    // Multi-chunk structure registry
    std::unordered_map<int, PlannedStructure> planned_structures;  // id -> structure
    std::unordered_set<int> generated_structure_ids;               // Already rendered structures
    int next_structure_id = 1;
    
    // Track which chunks have had structures planned
    std::unordered_set<ChunkCoord> structure_planned_chunks;
    
    // Convert world coordinates to chunk coordinates
    ChunkCoord world_to_chunk(int world_x, int world_y) const {
        int chunk_x = world_x / CHUNK_SIZE;
        int chunk_y = world_y / CHUNK_SIZE;
        
        // Handle negative coordinates properly
        if (world_x < 0 && world_x % CHUNK_SIZE != 0) chunk_x--;
        if (world_y < 0 && world_y % CHUNK_SIZE != 0) chunk_y--;
        
        return ChunkCoord(chunk_x, chunk_y);
    }
    
    // Convert world coordinates to local chunk coordinates
    void world_to_local(int world_x, int world_y, int& local_x, int& local_y) const {
        local_x = world_x % CHUNK_SIZE;
        local_y = world_y % CHUNK_SIZE;
        
        // Handle negative coordinates
        if (local_x < 0) local_x += CHUNK_SIZE;
        if (local_y < 0) local_y += CHUNK_SIZE;
    }
    
public:
    ChunkedWorld(unsigned int seed = 0) : seed(seed) {}
    
    void set_seed(unsigned int new_seed) { seed = new_seed; }
    unsigned int get_seed() const { return seed; }
    
    // Get or create a chunk
    Chunk* get_chunk(ChunkCoord coord) {
        auto it = chunks.find(coord);
        if (it != chunks.end()) {
            return it->second.get();
        }
        
        // Create new chunk
        auto chunk = std::make_unique<Chunk>(coord);
        Chunk* chunk_ptr = chunk.get();
        chunks[coord] = std::move(chunk);
        
        return chunk_ptr;
    }
    
    // Check if chunk exists
    bool has_chunk(ChunkCoord coord) const {
        return chunks.find(coord) != chunks.end();
    }
    
    // Get tile at world coordinates
    Tile& get_tile(int world_x, int world_y) {
        ChunkCoord chunk_coord = world_to_chunk(world_x, world_y);
        Chunk* chunk = get_chunk(chunk_coord);
        
        int local_x, local_y;
        world_to_local(world_x, world_y, local_x, local_y);
        
        return chunk->get_tile(local_x, local_y);
    }
    
    // Get tile pointer (returns nullptr if chunk not loaded)
    Tile* get_tile_ptr(int world_x, int world_y) {
        ChunkCoord chunk_coord = world_to_chunk(world_x, world_y);
        
        // Check if chunk exists
        auto it = chunks.find(chunk_coord);
        if (it == chunks.end()) {
            return nullptr;
        }
        
        int local_x, local_y;
        world_to_local(world_x, world_y, local_x, local_y);
        
        return &(it->second->get_tile(local_x, local_y));
    }
    
    // Set tile at world coordinates
    void set_tile(int world_x, int world_y, const Tile& tile) {
        ChunkCoord chunk_coord = world_to_chunk(world_x, world_y);
        Chunk* chunk = get_chunk(chunk_coord);
        
        int local_x, local_y;
        world_to_local(world_x, world_y, local_x, local_y);
        
        chunk->set_tile(local_x, local_y, tile);
    }
    
    // Check if a position is walkable
    bool is_walkable(int world_x, int world_y) {
        Tile* tile = get_tile_ptr(world_x, world_y);
        if (!tile) {
            // If chunk not loaded, assume not walkable for safety
            return false;
        }
        return tile->walkable;
    }
    
    // Get all loaded chunks
    const std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>>& get_chunks() const {
        return chunks;
    }
    
    // Unload chunks far from a position (memory management)
    void unload_distant_chunks(int center_x, int center_y, int max_distance) {
        ChunkCoord center_chunk = world_to_chunk(center_x, center_y);
        
        std::vector<ChunkCoord> to_unload;
        
        for (const auto& pair : chunks) {
            const ChunkCoord& coord = pair.first;
            int dx = coord.x - center_chunk.x;
            int dy = coord.y - center_chunk.y;
            int distance = dx * dx + dy * dy;
            
            if (distance > max_distance * max_distance) {
                to_unload.push_back(coord);
            }
        }
        
        for (const ChunkCoord& coord : to_unload) {
            chunks.erase(coord);
        }
    }
    
    // Get loaded chunk count
    size_t get_loaded_chunk_count() const {
        return chunks.size();
    }
    
    // Ensure chunks are loaded around a position
    void ensure_chunks_loaded(int center_x, int center_y, int radius) {
        ChunkCoord center_chunk = world_to_chunk(center_x, center_y);
        
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                ChunkCoord coord(center_chunk.x + dx, center_chunk.y + dy);
                get_chunk(coord); // Creates if doesn't exist
            }
        }
    }
    
    // ========== Multi-chunk structure management ==========
    
    // Register a planned structure (returns structure ID)
    int plan_structure(int world_x, int world_y, int width, int height, int type, unsigned int struct_seed, const std::string& structure_id = "") {
        int id = next_structure_id++;
        planned_structures[id] = PlannedStructure(id, world_x, world_y, width, height, type, struct_seed, structure_id);
        return id;
    }
    
    // Get all structures that overlap a given chunk
    std::vector<const PlannedStructure*> get_structures_for_chunk(int chunk_x, int chunk_y) const {
        std::vector<const PlannedStructure*> result;
        for (const auto& pair : planned_structures) {
            if (pair.second.overlaps_chunk(chunk_x, chunk_y)) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }
    
    // Check if a structure has already been rendered
    bool is_structure_generated(int structure_id) const {
        return generated_structure_ids.find(structure_id) != generated_structure_ids.end();
    }
    
    // Mark a structure as rendered
    void mark_structure_generated(int structure_id) {
        generated_structure_ids.insert(structure_id);
    }
    
    // Check if structures have been planned for a chunk region
    bool has_structures_planned(ChunkCoord coord) const {
        return structure_planned_chunks.find(coord) != structure_planned_chunks.end();
    }
    
    // Mark a chunk region as having structures planned
    void mark_structures_planned(ChunkCoord coord) {
        structure_planned_chunks.insert(coord);
    }
    
    // Get a planned structure by ID
    const PlannedStructure* get_planned_structure(int id) const {
        auto it = planned_structures.find(id);
        return (it != planned_structures.end()) ? &it->second : nullptr;
    }
    
    // Get all planned structures
    const std::unordered_map<int, PlannedStructure>& get_all_planned_structures() const {
        return planned_structures;
    }
    
    // Check if there's a structure of a given type within a certain chunk distance
    bool has_nearby_structure_type(int chunk_x, int chunk_y, int structure_type, int min_distance_chunks) const {
        int world_x = chunk_x * CHUNK_SIZE + CHUNK_SIZE / 2;
        int world_y = chunk_y * CHUNK_SIZE + CHUNK_SIZE / 2;
        int min_dist_world = min_distance_chunks * CHUNK_SIZE;
        int min_dist_sq = min_dist_world * min_dist_world;
        
        for (const auto& pair : planned_structures) {
            if (pair.second.type == structure_type) {
                int dx = pair.second.world_x - world_x;
                int dy = pair.second.world_y - world_y;
                int dist_sq = dx * dx + dy * dy;
                if (dist_sq < min_dist_sq) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // Convert world to chunk coordinate (public access)
    ChunkCoord get_chunk_coord(int world_x, int world_y) const {
        return world_to_chunk(world_x, world_y);
    }
};
