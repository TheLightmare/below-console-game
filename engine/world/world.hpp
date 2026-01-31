#pragma once
#include <string>
#include <vector>

class Tile {
public:
    bool walkable;
    char symbol;
    std::string color;
    float movement_cost;  // 1.0 = normal, 0.5 = takes 2 turns

    Tile(bool walkable = true, char symbol = '.', const std::string& color = "white", float move_cost = 1.0f)
        : walkable(walkable), symbol(symbol), color(color), movement_cost(move_cost) {}
};

class World {
private:
    int width;
    int height;
    std::vector<std::vector<Tile>> tiles;
public:
    World(int w, int h) : width(w), height(h), tiles(h, std::vector<Tile>(w)) {}
    Tile& get_tile(int x, int y) {
        return tiles[y][x];
    }
    void set_tile(int x, int y, const Tile& tile) {
        tiles[y][x] = tile;
    }
    int get_width() const { return width; }
    int get_height() const { return height; }
};