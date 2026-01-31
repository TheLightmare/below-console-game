#pragma once
#include "component_base.hpp"

struct PositionComponent : Component {
    int x = 0;
    int y = 0;
    int z = 0;
    
    PositionComponent() = default;
    PositionComponent(int x, int y, int z) : x(x), y(y), z(z) {}
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
