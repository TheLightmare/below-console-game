#pragma once
#include <functional>

// Chunk size constant
const int CHUNK_SIZE = 32;

// Chunk coordinate for addressing chunks in the world grid
// z selects the z-level (layer) within a column of chunks.
struct ChunkCoord {
    int x, y, z;
    
    ChunkCoord(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Hash function for ChunkCoord (includes z)
namespace std {
    template <>
    struct hash<ChunkCoord> {
        size_t operator()(const ChunkCoord& coord) const {
            size_t h = hash<int>()(coord.x);
            h ^= hash<int>()(coord.y) << 16;
            h ^= hash<int>()(coord.z) << 8;
            return h;
        }
    };
}
