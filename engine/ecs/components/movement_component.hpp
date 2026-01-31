#pragma once
#include "component_base.hpp"

// Velocity component (for movement)
struct VelocityComponent : Component {
    int dx = 0;
    int dy = 0;
    float speed = 1.0f;
    
    VelocityComponent() = default;
    VelocityComponent(int dx, int dy, float speed = 1.0f)
        : dx(dx), dy(dy), speed(speed) {}
    IMPLEMENT_CLONE(VelocityComponent)
};

// Player controller component
struct PlayerControllerComponent : Component {
    bool can_move = true;
    bool can_attack = true;
    
    PlayerControllerComponent() = default;
    PlayerControllerComponent(bool can_move, bool can_attack)
        : can_move(can_move), can_attack(can_attack) {}
    IMPLEMENT_CLONE(PlayerControllerComponent)
};
