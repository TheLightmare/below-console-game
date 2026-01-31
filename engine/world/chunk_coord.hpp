#pragma once
#include <functional>

// Chunk size constant
const int CHUNK_SIZE = 32;

// Chunk coordinate for addressing chunks in the world grid
struct ChunkCoord {
    int x, y;
    
    ChunkCoord(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
};

// Hash function for ChunkCoord
namespace std {
    template <>
    struct hash<ChunkCoord> {
        size_t operator()(const ChunkCoord& coord) const {
            return hash<int>()(coord.x) ^ (hash<int>()(coord.y) << 16);
        }
    };
}
