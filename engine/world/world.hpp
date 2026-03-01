#pragma once
#include <string>
#include <vector>
#include <algorithm>

// =============================================================================
// Tile — basic map cell, now z-level aware
// =============================================================================
class Tile {
public:
    bool walkable;
    char symbol;
    std::string color;
    float movement_cost;  // 1.0 = normal, 0.5 = takes 2 turns

    // --- Z-level / 3-D properties -------------------------------------------
    bool has_floor    = true;   // True if this tile has a solid floor
    bool has_ceiling  = false;  // True if something covers this tile from above
    bool is_ramp      = false;  // Ramp connects this z-level to the one above
    bool is_stairs_up   = false;  // Staircase going up (like Dwarf Fortress '<')
    bool is_stairs_down = false;  // Staircase going down ('>'')
    bool transparent  = true;   // Can you see through this tile to levels below?
    char below_symbol = ' ';    // Symbol shown when looking from above (dimmed)
    std::string below_color = "dark_gray"; // Colour when seen from above

    Tile(bool walkable = true, char symbol = '.', const std::string& color = "white", float move_cost = 1.0f)
        : walkable(walkable), symbol(symbol), color(color), movement_cost(move_cost) {}
};

// =============================================================================
// World — simple fixed-size 3-D world grid  (width x height x depth layers)
// =============================================================================
class World {
private:
    int width;
    int height;
    int depth;   // Number of z-levels
    // tiles[z][y][x]
    std::vector<std::vector<std::vector<Tile>>> tiles;
public:
    World(int w, int h, int d = 1)
        : width(w), height(h), depth(d),
          tiles(d, std::vector<std::vector<Tile>>(h, std::vector<Tile>(w))) {}

    // Access with z (defaults to 0 for backward compatibility)
    Tile& get_tile(int x, int y, int z = 0) {
        z = std::clamp(z, 0, depth - 1);
        return tiles[z][y][x];
    }
    void set_tile(int x, int y, const Tile& tile, int z = 0) {
        z = std::clamp(z, 0, depth - 1);
        tiles[z][y][x] = tile;
    }

    int get_width()  const { return width;  }
    int get_height() const { return height; }
    int get_depth()  const { return depth;  }

    // Check whether a z-level exists
    bool valid_z(int z) const { return z >= 0 && z < depth; }

    // Check if movement between z-levels is possible at (x, y)
    bool can_go_up(int x, int y, int z) const {
        if (z + 1 >= depth) return false;
        const Tile& here = tiles[z][y][x];
        return here.is_stairs_up || here.is_ramp;
    }
    bool can_go_down(int x, int y, int z) const {
        if (z - 1 < 0) return false;
        const Tile& below = tiles[z - 1][y][x];
        return below.is_stairs_up || below.is_ramp || tiles[z][y][x].is_stairs_down;
    }
};