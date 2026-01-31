#pragma once
#include "component_base.hpp"

struct StatsComponent : Component {
    int level = 1;
    int experience = 0;
    int max_hp = 20;
    int current_hp = 20;
    int attack = 5;
    int defense = 5;
    int attribute_points = 0;  // Points to spend on stats
    
    StatsComponent() = default;
    StatsComponent(int level, int experience, int max_hp, int current_hp, int attack, int defense)
        : level(level), experience(experience), max_hp(max_hp), current_hp(current_hp), attack(attack), defense(defense), attribute_points(0) {}

    bool is_alive() const { return current_hp > 0; }
    
    int heal(int amount) {
        int old_hp = current_hp;
        int new_hp = current_hp + amount;
        current_hp = (new_hp > max_hp) ? max_hp : new_hp;
        return current_hp - old_hp;
    }
    
    // Apply damage to the entity, returns actual damage dealt
    int take_damage(int amount) {
        int old_hp = current_hp;
        int new_hp = current_hp - amount;
        current_hp = (new_hp < 0) ? 0 : new_hp;
        return old_hp - current_hp;
    }
    
    void gain_experience(int exp) {
        experience += exp;
        while (experience >= get_exp_to_level()) {
            level_up();
        }
    }
    
    int get_exp_to_level() const {
        return level * 100; // Simple formula
    }
    
    void level_up() {
        level++;
        experience = 0;
        attribute_points += 3;  // Gain 3 points per level to spend
        // Small automatic HP increase
        max_hp += 2;
        current_hp = max_hp;
    }
    IMPLEMENT_CLONE(StatsComponent)
};

struct NameComponent : Component {
    std::string name;
    std::string description;
    
    NameComponent() = default;
    NameComponent(const std::string& name, const std::string& description = "")
        : name(name), description(description) {}
    IMPLEMENT_CLONE(NameComponent)
};
