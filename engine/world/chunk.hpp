#pragma once
#include "world.hpp"
#include "chunk_coord.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

// Planned multi-chunk structure (stored in world coordinates)
struct PlannedStructure {
    int id;                     // Unique structure ID
    int world_x, world_y;       // World position of structure center (x, y)
    int world_z;                // Z-level the structure sits on
    int width, height;          // Total size in tiles (x, y)
    int depth;                  // How many z-levels the structure spans
    int type;                   // StructureType enum value
    unsigned int structure_seed; // Seed for deterministic generation
    std::string structure_id;   // JSON structure definition ID (e.g., "shop", "tavern")
    
    PlannedStructure()
        : id(0), world_x(0), world_y(0), world_z(0),
          width(0), height(0), depth(1), type(-1), structure_seed(0) {}
    
    PlannedStructure(int id, int wx, int wy, int w, int h, int t, unsigned int s, const std::string& sid = "")
        : id(id), world_x(wx), world_y(wy), world_z(0),
          width(w), height(h), depth(1), type(t), structure_seed(s), structure_id(sid) {}

    PlannedStructure(int id, int wx, int wy, int wz, int w, int h, int d, int t, unsigned int s, const std::string& sid = "")
        : id(id), world_x(wx), world_y(wy), world_z(wz),
          width(w), height(h), depth(d), type(t), structure_seed(s), structure_id(sid) {}
    
    // Get bounding box in world coordinates (x, y)
    int min_x() const { return world_x - width / 2; }
    int max_x() const { return world_x + width / 2; }
    int min_y() const { return world_y - height / 2; }
    int max_y() const { return world_y + height / 2; }
    int min_z() const { return world_z; }
    int max_z() const { return world_z + depth - 1; }
    
    // Check if this structure overlaps a chunk (x, y only; z checked separately)
    bool overlaps_chunk(int chunk_x, int chunk_y) const {
        int chunk_min_x = chunk_x * CHUNK_SIZE;
        int chunk_max_x = chunk_min_x + CHUNK_SIZE - 1;
        int chunk_min_y = chunk_y * CHUNK_SIZE;
        int chunk_max_y = chunk_min_y + CHUNK_SIZE - 1;
        
        return !(max_x() < chunk_min_x || min_x() > chunk_max_x ||
                 max_y() < chunk_min_y || min_y() > chunk_max_y);
    }

    // Check if this structure overlaps a chunk (full 3-D check)
    bool overlaps_chunk_3d(int chunk_x, int chunk_y, int chunk_z) const {
        if (chunk_z < min_z() || chunk_z > max_z()) return false;
        return overlaps_chunk(chunk_x, chunk_y);
    }
    
    // Get which chunks this structure spans (including z-levels)
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
        
        for (int cz = min_z(); cz <= max_z(); ++cz) {
            for (int cy = min_chunk_y; cy <= max_chunk_y; ++cy) {
                for (int cx = min_chunk_x; cx <= max_chunk_x; ++cx) {
                    chunks.push_back(ChunkCoord(cx, cy, cz));
                }
            }
        }
        return chunks;
    }
};

// Info about a placed building/structure in a chunk
struct PlacedBuildingInfo {
    std::string structure_id;
    int center_x;
    int center_y;
    int width;
    int height;
    
    PlacedBuildingInfo() = default;
    PlacedBuildingInfo(const std::string& id, int cx, int cy, int w, int h)
        : structure_id(id), center_x(cx), center_y(cy), width(w), height(h) {}
};

// =============================================================================
// Single chunk of the world  (one z-level of a CHUNK_SIZE x CHUNK_SIZE area)
// =============================================================================
class Chunk {
private:
    std::vector<std::vector<Tile>> tiles;   // [y][x]
    ChunkCoord coord;                       // includes z
    bool generated;
    int structure_type; // -1 = none, 0 = ruins, 1 = camp, 2 = stone circle
    std::string structure_definition_id;
    int structure_center_x;
    int structure_center_y;
    std::vector<PlacedBuildingInfo> placed_buildings;
    bool is_primary_structure = false;
    
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
    int get_z() const { return coord.z; }
    
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

// =============================================================================
// Chunked world manager — now z-level aware
// =============================================================================
class ChunkedWorld {
private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> chunks;
    unsigned int seed;
    
    // Multi-chunk structure registry
    std::unordered_map<int, PlannedStructure> planned_structures;
    std::unordered_set<int> generated_structure_ids;
    int next_structure_id = 1;
    
    // Track which chunks have had structures planned
    std::unordered_set<ChunkCoord> structure_planned_chunks;
    
    // Convert world (x, y) to chunk (cx, cy).  z is passed through.
    ChunkCoord world_to_chunk(int world_x, int world_y, int z = 0) const {
        int chunk_x = world_x / CHUNK_SIZE;
        int chunk_y = world_y / CHUNK_SIZE;
        
        if (world_x < 0 && world_x % CHUNK_SIZE != 0) chunk_x--;
        if (world_y < 0 && world_y % CHUNK_SIZE != 0) chunk_y--;
        
        return ChunkCoord(chunk_x, chunk_y, z);
    }
    
    void world_to_local(int world_x, int world_y, int& local_x, int& local_y) const {
        local_x = world_x % CHUNK_SIZE;
        local_y = world_y % CHUNK_SIZE;
        
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
        auto chunk = std::make_unique<Chunk>(coord);
        Chunk* chunk_ptr = chunk.get();
        chunks[coord] = std::move(chunk);
        return chunk_ptr;
    }
    
    bool has_chunk(ChunkCoord coord) const {
        return chunks.find(coord) != chunks.end();
    }
    
    // ---- Tile access (z-aware, backward compatible) -------------------------

    Tile& get_tile(int world_x, int world_y, int z = 0) {
        ChunkCoord cc = world_to_chunk(world_x, world_y, z);
        Chunk* chunk = get_chunk(cc);
        int lx, ly;
        world_to_local(world_x, world_y, lx, ly);
        return chunk->get_tile(lx, ly);
    }
    
    Tile* get_tile_ptr(int world_x, int world_y, int z = 0) {
        ChunkCoord cc = world_to_chunk(world_x, world_y, z);
        auto it = chunks.find(cc);
        if (it == chunks.end()) return nullptr;
        int lx, ly;
        world_to_local(world_x, world_y, lx, ly);
        return &(it->second->get_tile(lx, ly));
    }
    
    void set_tile(int world_x, int world_y, const Tile& tile, int z = 0) {
        ChunkCoord cc = world_to_chunk(world_x, world_y, z);
        Chunk* chunk = get_chunk(cc);
        int lx, ly;
        world_to_local(world_x, world_y, lx, ly);
        chunk->set_tile(lx, ly, tile);
    }
    
    bool is_walkable(int world_x, int world_y, int z = 0) {
        Tile* tile = get_tile_ptr(world_x, world_y, z);
        if (!tile) return false;
        return tile->walkable;
    }

    // Check if an entity can move between z-levels at (world_x, world_y)
    bool can_go_up(int world_x, int world_y, int z) {
        Tile* here = get_tile_ptr(world_x, world_y, z);
        if (!here) return false;
        return here->is_stairs_up || here->is_ramp;
    }

    bool can_go_down(int world_x, int world_y, int z) {
        Tile* here = get_tile_ptr(world_x, world_y, z);
        if (!here) return false;
        if (here->is_stairs_down) return true;
        Tile* below = get_tile_ptr(world_x, world_y, z - 1);
        if (below && (below->is_stairs_up || below->is_ramp)) return true;
        return false;
    }
    
    // ---- Chunk management ---------------------------------------------------

    const std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>>& get_chunks() const {
        return chunks;
    }
    
    void unload_distant_chunks(int center_x, int center_y, int max_distance, int center_z = 0) {
        ChunkCoord center_chunk = world_to_chunk(center_x, center_y, center_z);
        
        std::vector<ChunkCoord> to_unload;
        for (const auto& pair : chunks) {
            const ChunkCoord& coord = pair.first;
            int dx = coord.x - center_chunk.x;
            int dy = coord.y - center_chunk.y;
            int dz = coord.z - center_chunk.z;
            int distance = dx * dx + dy * dy + dz * dz;
            if (distance > max_distance * max_distance) {
                to_unload.push_back(coord);
            }
        }
        for (const ChunkCoord& coord : to_unload) {
            chunks.erase(coord);
        }
    }
    
    size_t get_loaded_chunk_count() const {
        return chunks.size();
    }
    
    // Ensure chunks are loaded around a position (on a single z-level)
    void ensure_chunks_loaded(int center_x, int center_y, int radius, int z = 0) {
        ChunkCoord center_chunk = world_to_chunk(center_x, center_y, z);
        
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                ChunkCoord coord(center_chunk.x + dx, center_chunk.y + dy, z);
                get_chunk(coord);
            }
        }
    }

    // Ensure chunks are loaded across a range of z-levels
    void ensure_chunks_loaded_3d(int center_x, int center_y, int center_z,
                                  int xy_radius, int z_radius = 1) {
        ChunkCoord center_chunk = world_to_chunk(center_x, center_y, center_z);
        for (int dz = -z_radius; dz <= z_radius; ++dz) {
            for (int dy = -xy_radius; dy <= xy_radius; ++dy) {
                for (int dx = -xy_radius; dx <= xy_radius; ++dx) {
                    ChunkCoord coord(center_chunk.x + dx, center_chunk.y + dy, center_z + dz);
                    get_chunk(coord);
                }
            }
        }
    }
    
    // ========== Multi-chunk structure management ==========
    
    int plan_structure(int world_x, int world_y, int width, int height, int type,
                       unsigned int struct_seed, const std::string& structure_id = "") {
        int id = next_structure_id++;
        planned_structures[id] = PlannedStructure(id, world_x, world_y, width, height, type, struct_seed, structure_id);
        return id;
    }

    // Plan a multi-level structure
    int plan_structure_3d(int world_x, int world_y, int world_z,
                          int width, int height, int depth_z, int type,
                          unsigned int struct_seed, const std::string& structure_id = "") {
        int id = next_structure_id++;
        planned_structures[id] = PlannedStructure(id, world_x, world_y, world_z,
                                                   width, height, depth_z, type,
                                                   struct_seed, structure_id);
        return id;
    }
    
    std::vector<const PlannedStructure*> get_structures_for_chunk(int chunk_x, int chunk_y) const {
        std::vector<const PlannedStructure*> result;
        for (const auto& pair : planned_structures) {
            if (pair.second.overlaps_chunk(chunk_x, chunk_y)) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }

    std::vector<const PlannedStructure*> get_structures_for_chunk_3d(int chunk_x, int chunk_y, int chunk_z) const {
        std::vector<const PlannedStructure*> result;
        for (const auto& pair : planned_structures) {
            if (pair.second.overlaps_chunk_3d(chunk_x, chunk_y, chunk_z)) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }
    
    bool is_structure_generated(int structure_id) const {
        return generated_structure_ids.find(structure_id) != generated_structure_ids.end();
    }
    
    void mark_structure_generated(int structure_id) {
        generated_structure_ids.insert(structure_id);
    }
    
    bool has_structures_planned(ChunkCoord coord) const {
        return structure_planned_chunks.find(coord) != structure_planned_chunks.end();
    }
    
    void mark_structures_planned(ChunkCoord coord) {
        structure_planned_chunks.insert(coord);
    }
    
    const PlannedStructure* get_planned_structure(int id) const {
        auto it = planned_structures.find(id);
        return (it != planned_structures.end()) ? &it->second : nullptr;
    }
    
    const std::unordered_map<int, PlannedStructure>& get_all_planned_structures() const {
        return planned_structures;
    }
    
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
    
    // Public coordinate helpers
    ChunkCoord get_chunk_coord(int world_x, int world_y, int z = 0) const {
        return world_to_chunk(world_x, world_y, z);
    }
};
