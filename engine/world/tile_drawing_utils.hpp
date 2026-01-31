#pragma once

#include "chunk.hpp"

namespace TileDrawing {

inline void safe_set_tile(Chunk* chunk, int x, int y, const Tile& tile) {
    if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE) {
        chunk->set_tile(x, y, tile);
    }
}

// Fill a rectangle with a tile
inline void fill_rect(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile) {
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            safe_set_tile(chunk, x, y, tile);
        }
    }
}

// Draw a rectangle outline
inline void draw_rect_outline(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile) {
    for (int x = x1; x <= x2; ++x) {
        safe_set_tile(chunk, x, y1, tile);
        safe_set_tile(chunk, x, y2, tile);
    }
    for (int y = y1; y <= y2; ++y) {
        safe_set_tile(chunk, x1, y, tile);
        safe_set_tile(chunk, x2, y, tile);
    }
}

// Draw a circle (filled or outline)
inline void draw_circle(Chunk* chunk, int cx, int cy, int radius, const Tile& tile, bool filled = false) {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            int dist_sq = x * x + y * y;
            if (filled) {
                if (dist_sq <= radius * radius) {
                    safe_set_tile(chunk, cx + x, cy + y, tile);
                }
            } else {
                if (dist_sq >= (radius - 1) * (radius - 1) && dist_sq <= radius * radius) {
                    safe_set_tile(chunk, cx + x, cy + y, tile);
                }
            }
        }
    }
}

// Draw a path/line using Bresenham's algorithm with width
inline void draw_path(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width = 1) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    while (true) {
        for (int w = -width/2; w <= width/2; ++w) {
            if (dx > dy) {
                safe_set_tile(chunk, x, y + w, tile);
            } else {
                safe_set_tile(chunk, x + w, y, tile);
            }
        }
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

// Alias for draw_path (for backward compatibility with structure_loader)
inline void draw_line(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width = 1) {
    draw_path(chunk, x1, y1, x2, y2, tile, width);
}

// ----------------------------------------------------------------------------
// World Coordinate Helpers (for multi-chunk structures)
// ----------------------------------------------------------------------------

// Set tile using world coordinates with render bounds checking
inline void set_tile_world(Chunk* chunk, int world_x, int world_y, const Tile& tile,
                           int render_min_x, int render_max_x, int render_min_y, int render_max_y,
                           ChunkCoord chunk_coord) {
    // Check if within render bounds
    if (world_x < render_min_x || world_x > render_max_x ||
        world_y < render_min_y || world_y > render_max_y) {
        return;
    }
    
    // Convert to local coordinates
    int local_x = world_x - chunk_coord.x * CHUNK_SIZE;
    int local_y = world_y - chunk_coord.y * CHUNK_SIZE;
    
    safe_set_tile(chunk, local_x, local_y, tile);
}

// Overload that gets chunk_coord from chunk
inline void set_tile_world(Chunk* chunk, int world_x, int world_y, const Tile& tile,
                           int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    set_tile_world(chunk, world_x, world_y, tile, 
                   render_min_x, render_max_x, render_min_y, render_max_y,
                   chunk->get_coord());
}

// Fill rectangle using world coordinates
inline void fill_rect_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile,
                            int render_min_x, int render_max_x, int render_min_y, int render_max_y,
                            ChunkCoord chunk_coord) {
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            set_tile_world(chunk, x, y, tile, render_min_x, render_max_x, 
                          render_min_y, render_max_y, chunk_coord);
        }
    }
}

inline void fill_rect_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile,
                            int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    fill_rect_world(chunk, x1, y1, x2, y2, tile,
                    render_min_x, render_max_x, render_min_y, render_max_y,
                    chunk->get_coord());
}

// Draw rectangle outline using world coordinates
inline void draw_rect_outline_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile,
                                    int render_min_x, int render_max_x, int render_min_y, int render_max_y,
                                    ChunkCoord chunk_coord) {
    for (int x = x1; x <= x2; ++x) {
        set_tile_world(chunk, x, y1, tile, render_min_x, render_max_x, 
                      render_min_y, render_max_y, chunk_coord);
        set_tile_world(chunk, x, y2, tile, render_min_x, render_max_x, 
                      render_min_y, render_max_y, chunk_coord);
    }
    for (int y = y1; y <= y2; ++y) {
        set_tile_world(chunk, x1, y, tile, render_min_x, render_max_x, 
                      render_min_y, render_max_y, chunk_coord);
        set_tile_world(chunk, x2, y, tile, render_min_x, render_max_x, 
                      render_min_y, render_max_y, chunk_coord);
    }
}

inline void draw_rect_outline_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile,
                                    int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    draw_rect_outline_world(chunk, x1, y1, x2, y2, tile,
                            render_min_x, render_max_x, render_min_y, render_max_y,
                            chunk->get_coord());
}

// Draw circle using world coordinates
inline void draw_circle_world(Chunk* chunk, int cx, int cy, int radius, const Tile& tile, bool filled,
                              int render_min_x, int render_max_x, int render_min_y, int render_max_y,
                              ChunkCoord chunk_coord) {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            int dist_sq = x * x + y * y;
            if (filled) {
                if (dist_sq <= radius * radius) {
                    set_tile_world(chunk, cx + x, cy + y, tile, render_min_x, render_max_x,
                                  render_min_y, render_max_y, chunk_coord);
                }
            } else {
                if (dist_sq >= (radius - 1) * (radius - 1) && dist_sq <= radius * radius) {
                    set_tile_world(chunk, cx + x, cy + y, tile, render_min_x, render_max_x,
                                  render_min_y, render_max_y, chunk_coord);
                }
            }
        }
    }
}

inline void draw_circle_world(Chunk* chunk, int cx, int cy, int radius, const Tile& tile, bool filled,
                              int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    draw_circle_world(chunk, cx, cy, radius, tile, filled,
                      render_min_x, render_max_x, render_min_y, render_max_y,
                      chunk->get_coord());
}

// Draw path/line using world coordinates
inline void draw_path_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width,
                            int render_min_x, int render_max_x, int render_min_y, int render_max_y,
                            ChunkCoord chunk_coord) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    while (true) {
        for (int w = -width/2; w <= width/2; ++w) {
            if (dx > dy) {
                set_tile_world(chunk, x, y + w, tile, render_min_x, render_max_x,
                              render_min_y, render_max_y, chunk_coord);
            } else {
                set_tile_world(chunk, x + w, y, tile, render_min_x, render_max_x,
                              render_min_y, render_max_y, chunk_coord);
            }
        }
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

inline void draw_path_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width,
                            int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    draw_path_world(chunk, x1, y1, x2, y2, tile, width,
                    render_min_x, render_max_x, render_min_y, render_max_y,
                    chunk->get_coord());
}

// Alias for draw_path_world (for backward compatibility with structure_loader)
inline void draw_line_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width,
                            int render_min_x, int render_max_x, int render_min_y, int render_max_y,
                            ChunkCoord chunk_coord) {
    draw_path_world(chunk, x1, y1, x2, y2, tile, width,
                    render_min_x, render_max_x, render_min_y, render_max_y, chunk_coord);
}

inline void draw_line_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width,
                            int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    draw_path_world(chunk, x1, y1, x2, y2, tile, width,
                    render_min_x, render_max_x, render_min_y, render_max_y);
}

// Simple path drawing for world coords (used for roads)
inline void draw_simple_path_world(Chunk* chunk, int x1, int y1, int x2, int y2, const Tile& tile, int width,
                                   int render_min_x, int render_max_x, int render_min_y, int render_max_y) {
    int dx = (x2 > x1) ? 1 : (x2 < x1) ? -1 : 0;
    int dy = (y2 > y1) ? 1 : (y2 < y1) ? -1 : 0;
    
    int x = x1, y = y1;
    while (x != x2 || y != y2) {
        for (int w = -width/2; w <= width/2; ++w) {
            if (dx != 0) set_tile_world(chunk, x, y + w, tile, render_min_x, render_max_x, render_min_y, render_max_y);
            if (dy != 0) set_tile_world(chunk, x + w, y, tile, render_min_x, render_max_x, render_min_y, render_max_y);
        }
        if (x != x2) x += dx;
        if (y != y2) y += dy;
    }
}

} // namespace TileDrawing
