#pragma once
#include "types.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

// =============================================================================
// SpatialHash — O(1) spatial queries for entity positions
// =============================================================================
// Divides the world into cells of `cell_size` tiles. Entities are bucketed by
// their (x, y, z) position so that "who is at tile (x,y,z)?" and "who is near
// (x,y,z)?" are constant-time (amortised) lookups instead of O(N) scans.
//
// The ComponentManager calls insert/remove/move automatically when positions
// change, so game code doesn't need to interact with this directly.
// =============================================================================

class SpatialHash {
public:
    explicit SpatialHash(int cell_size = 8) : cell_size_(cell_size) {}

    // ---- Mutation -----------------------------------------------------------

    void insert(EntityId id, int x, int y, int z = 0) {
        uint64_t k = key(x, y, z);
        cells_[k].insert(id);
        entity_cells_[id] = k;
    }

    void remove(EntityId id) {
        auto it = entity_cells_.find(id);
        if (it == entity_cells_.end()) return;
        uint64_t k = it->second;
        auto cell_it = cells_.find(k);
        if (cell_it != cells_.end()) {
            cell_it->second.erase(id);
            if (cell_it->second.empty()) cells_.erase(cell_it);
        }
        entity_cells_.erase(it);
    }

    void move(EntityId id, int old_x, int old_y, int old_z,
                            int new_x, int new_y, int new_z) {
        uint64_t old_k = key(old_x, old_y, old_z);
        uint64_t new_k = key(new_x, new_y, new_z);
        if (old_k == new_k) return; // Same cell, nothing to do

        // Remove from old cell
        auto cell_it = cells_.find(old_k);
        if (cell_it != cells_.end()) {
            cell_it->second.erase(id);
            if (cell_it->second.empty()) cells_.erase(cell_it);
        }

        // Insert into new cell
        cells_[new_k].insert(id);
        entity_cells_[id] = new_k;
    }

    void clear() {
        cells_.clear();
        entity_cells_.clear();
    }

    // ---- Queries ------------------------------------------------------------

    // Get all entities whose cell contains tile (x, y, z).
    // Returned set may include entities in the same cell but at different exact
    // positions — callers should do a final exact-position check if needed.
    const std::unordered_set<EntityId>& at_cell(int x, int y, int z = 0) const {
        static const std::unordered_set<EntityId> empty;
        auto it = cells_.find(key(x, y, z));
        return it != cells_.end() ? it->second : empty;
    }

    // Get all entities within `radius` tiles of (cx, cy, cz).
    // Collects candidates from all cells that overlap the bounding box, then
    // optionally filters by exact Chebyshev distance.
    std::vector<EntityId> in_radius(int cx, int cy, int cz, int radius) const {
        std::vector<EntityId> result;
        int min_cell_x = cell_coord(cx - radius);
        int max_cell_x = cell_coord(cx + radius);
        int min_cell_y = cell_coord(cy - radius);
        int max_cell_y = cell_coord(cy + radius);
        // z is stored in the key directly — we only look at the exact z
        for (int cy_cell = min_cell_y; cy_cell <= max_cell_y; ++cy_cell) {
            for (int cx_cell = min_cell_x; cx_cell <= max_cell_x; ++cx_cell) {
                uint64_t k = cell_key(cx_cell, cy_cell, cz);
                auto it = cells_.find(k);
                if (it != cells_.end()) {
                    for (EntityId id : it->second) {
                        result.push_back(id);
                    }
                }
            }
        }
        return result;
    }

    // Convenience overload without z
    std::vector<EntityId> in_radius(int cx, int cy, int radius) const {
        return in_radius(cx, cy, 0, radius);
    }

    // Check if any entity occupies the cell that (x, y, z) maps to.
    bool has_any(int x, int y, int z = 0) const {
        auto it = cells_.find(key(x, y, z));
        return it != cells_.end() && !it->second.empty();
    }

    // Returns true if the entity is tracked.
    bool contains(EntityId id) const {
        return entity_cells_.find(id) != entity_cells_.end();
    }

    int get_cell_size() const { return cell_size_; }

private:
    int cell_size_;

    // cell key -> set of entity IDs in that cell
    std::unordered_map<uint64_t, std::unordered_set<EntityId>> cells_;
    // entity ID -> its current cell key  (for fast removal / moves)
    std::unordered_map<EntityId, uint64_t> entity_cells_;

    // Convert a tile coord to the cell coord (integer division that handles negatives)
    int cell_coord(int v) const {
        return (v >= 0) ? (v / cell_size_) : ((v - cell_size_ + 1) / cell_size_);
    }

    // Pack (cell_x, cell_y, z) into a single 64-bit key.
    // Layout: z[15:0] | cell_y[23:0] | cell_x[23:0]  (with sign bits handled
    // via offset so we don't need to worry about negative bit patterns colliding)
    uint64_t cell_key(int cell_x, int cell_y, int z) const {
        // Offset to make values positive for safe bit-packing
        constexpr int offset = 0x800000; // 2^23
        uint64_t ux = static_cast<uint64_t>(cell_x + offset) & 0xFFFFFF;
        uint64_t uy = static_cast<uint64_t>(cell_y + offset) & 0xFFFFFF;
        uint64_t uz = static_cast<uint64_t>(z + 0x8000)      & 0xFFFF;
        return (uz << 48) | (uy << 24) | ux;
    }

    // From a world tile position directly to a packed cell key
    uint64_t key(int x, int y, int z) const {
        return cell_key(cell_coord(x), cell_coord(y), z);
    }
};
