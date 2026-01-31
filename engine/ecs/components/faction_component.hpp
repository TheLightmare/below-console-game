#pragma once
#include "component_base.hpp"
#include "../types.hpp"
#include <vector>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cmath>
#include <string>

// ==================== FACTION SYSTEM ====================

// Faction IDs - each faction has a unique ID
enum class FactionId {
    NONE = 0,           // No faction (neutral to all)
    PLAYER = 1,         // Player and allies
    VILLAGER = 2,       // Villagers, merchants, NPCs
    GUARD = 3,          // Town guards, soldiers
    WILDLIFE = 4,       // Neutral animals (deer, rabbits)
    PREDATOR = 5,       // Hostile animals (wolves, bears)
    GOBLIN = 6,         // Goblins and goblinoids
    UNDEAD = 7,         // Skeletons, zombies, ghosts
    BANDIT = 8,         // Bandits and outlaws
    DEMON = 9,          // Demons and evil creatures
    DRAGON = 10,        // Dragons
    SPIDER = 11,        // Spiders and arachnids
    ORC = 12,           // Orcs
    ELEMENTAL = 13,     // Elementals
    BEAST = 14,         // Generic hostile beasts
    
    FACTION_COUNT       // Keep last - used for array sizing
};

// Relationship between factions
enum class FactionRelation {
    ALLY = 0,           // Will defend each other
    FRIENDLY = 1,       // Won't attack, but won't defend
    NEUTRAL = 2,        // Ignores each other
    UNFRIENDLY = 3,     // May attack if provoked
    HOSTILE = 4         // Will attack on sight
};

// Faction component - assigns an entity to a faction
struct FactionComponent : Component {
    FactionId faction = FactionId::NONE;
    
    // Personal reputation modifiers (entity-specific overrides)
    // Positive = more friendly, Negative = more hostile
    std::unordered_map<FactionId, int> reputation_modifiers;
    
    // Who last attacked this entity (for retaliation)
    EntityId last_attacker = 0;
    
    // List of entities this specific entity is hostile to (personal enemies)
    std::vector<EntityId> personal_enemies;
    
    // List of entities this specific entity will defend (personal allies)
    std::vector<EntityId> personal_allies;
    
    FactionComponent() = default;
    FactionComponent(FactionId faction) : faction(faction) {}
    
    // Add a personal enemy
    void add_enemy(EntityId id) {
        if (std::find(personal_enemies.begin(), personal_enemies.end(), id) == personal_enemies.end()) {
            personal_enemies.push_back(id);
        }
    }
    
    // Add a personal ally
    void add_ally(EntityId id) {
        if (std::find(personal_allies.begin(), personal_allies.end(), id) == personal_allies.end()) {
            personal_allies.push_back(id);
        }
    }
    
    // Remove a personal enemy
    void remove_enemy(EntityId id) {
        personal_enemies.erase(std::remove(personal_enemies.begin(), personal_enemies.end(), id), personal_enemies.end());
    }
    
    // Check if entity is a personal enemy
    bool is_personal_enemy(EntityId id) const {
        return std::find(personal_enemies.begin(), personal_enemies.end(), id) != personal_enemies.end();
    }
    
    // Check if entity is a personal ally
    bool is_personal_ally(EntityId id) const {
        return std::find(personal_allies.begin(), personal_allies.end(), id) != personal_allies.end();
    }
    IMPLEMENT_CLONE(FactionComponent)
};

// Home component - tracks an NPC's home location for returning behavior
struct HomeComponent : Component {
    int home_x = 0;
    int home_y = 0;
    int bed_x = 0;      // Bed position within home
    int bed_y = 0;
    bool is_home = false;      // Currently at home?
    bool is_sleeping = false;  // Currently sleeping?
    std::string home_structure_id;  // ID of the home structure (e.g., "house_small")
    
    HomeComponent() = default;
    HomeComponent(int hx, int hy, int bx = 0, int by = 0, const std::string& structure = "")
        : home_x(hx), home_y(hy), bed_x(bx), bed_y(by), home_structure_id(structure) {}
    
    float distance_to_home(int x, int y) const {
        int dx = x - home_x;
        int dy = y - home_y;
        return std::sqrt(static_cast<float>(dx * dx + dy * dy));
    }
    
    bool is_near_home(int x, int y, int threshold = 3) const {
        return distance_to_home(x, y) <= static_cast<float>(threshold);
    }
    IMPLEMENT_CLONE(HomeComponent)
};

// Faction Manager - singleton-like class to manage faction relationships
class FactionManager {
private:
    // Relationship matrix: relationships[faction_a][faction_b] = relation from a's perspective to b
    std::array<std::array<FactionRelation, static_cast<size_t>(FactionId::FACTION_COUNT)>, 
               static_cast<size_t>(FactionId::FACTION_COUNT)> relationships;
    
public:
    FactionManager();
    
    void setup_default_relationships();
    
    // Set relationship from one faction's perspective
    void set_relation(FactionId from, FactionId to, FactionRelation relation);
    
    // Set mutual hostility between two factions
    void set_mutual_hostility(FactionId a, FactionId b);
    
    // Set mutual alliance between two factions
    void set_mutual_alliance(FactionId a, FactionId b);
    
    // Get relationship between two factions
    FactionRelation get_relation(FactionId from, FactionId to) const;
    
    // Check if faction a is hostile to faction b
    bool is_hostile(FactionId from, FactionId to) const;
    
    // Check if faction a is allied with faction b
    bool is_allied(FactionId from, FactionId to) const;
    
    // Check if faction a is friendly with faction b (ally or friendly)
    bool is_friendly(FactionId from, FactionId to) const;
    
    // Get effective relationship between two entities (considers personal modifiers)
    FactionRelation get_effective_relation(const FactionComponent* from_faction, 
                                           const FactionComponent* to_faction,
                                           EntityId to_id) const;
    
    // Check if entity A should attack entity B
    bool should_attack(const FactionComponent* attacker, const FactionComponent* target, EntityId target_id) const;
    
    // Check if entity A should defend entity B
    bool should_defend(const FactionComponent* defender, const FactionComponent* ally, EntityId ally_id) const;
};

// Global faction manager instance
inline FactionManager& get_faction_manager() {
    static FactionManager instance;
    return instance;
}
