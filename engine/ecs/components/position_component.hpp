#pragma once
#include "component_base.hpp"
#include <cmath>
#include <ostream>

struct PositionComponent : Component {
    int x = 0;
    int y = 0;
    int z = 0;
    
    PositionComponent() = default;
    PositionComponent(int x, int y, int z) : x(x), y(y), z(z) {}

    // Equality / inequality (compares x, y, z)
    bool operator==(const PositionComponent& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const PositionComponent& other) const {
        return !(*this == other);
    }

    // Arithmetic – returns a new PositionComponent
    PositionComponent operator+(const PositionComponent& other) const {
        return PositionComponent(x + other.x, y + other.y, z + other.z);
    }
    PositionComponent operator-(const PositionComponent& other) const {
        return PositionComponent(x - other.x, y - other.y, z - other.z);
    }

    // Compound assignment
    PositionComponent& operator+=(const PositionComponent& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    PositionComponent& operator-=(const PositionComponent& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    // Manhattan distance (useful for range checks)
    int manhattan_distance(const PositionComponent& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }

    // Chebyshev distance (king-move distance, common for grid games)
    int chebyshev_distance(const PositionComponent& other) const {
        return std::max(std::abs(x - other.x), std::abs(y - other.y));
    }

    // Stream insertion for debug output  e.g.  "(-3, 7, 0)"
    friend std::ostream& operator<<(std::ostream& os, const PositionComponent& p) {
        return os << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    }

    IMPLEMENT_CLONE(PositionComponent)
};

struct CollisionComponent : Component {
    bool blocks_movement = false;
    bool blocks_sight = false;

    CollisionComponent() = default;
    CollisionComponent(bool blocks_movement, bool blocks_sight)
        : blocks_movement(blocks_movement), blocks_sight(blocks_sight) {}
    IMPLEMENT_CLONE(CollisionComponent)
};
