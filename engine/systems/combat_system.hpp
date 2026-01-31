#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../loaders/entity_loader.hpp"
#include <string>
#include <vector>
#include <functional>

// Forward declaration
class StatusEffectSystem;

// Combat system - handles all combat logic
class CombatSystem {
private:
    ComponentManager* manager;
    EntityLoader* entity_loader = nullptr;
    StatusEffectSystem* status_effect_system = nullptr;
    EntityId player_id = 0;
    std::function<void(const std::string&)> message_callback;
    std::function<void(const std::string&)> player_death_callback;  // Called when player dies with death reason

public:
    CombatSystem(ComponentManager* mgr);
    
    void set_player_id(EntityId id) { player_id = id; }
    void set_entity_loader(EntityLoader* loader) { entity_loader = loader; }
    void set_status_effect_system(StatusEffectSystem* sys) { status_effect_system = sys; }
    
    void set_message_callback(std::function<void(const std::string&)> callback);
    
    // Set callback for when player dies (receives death reason string)
    void set_player_death_callback(std::function<void(const std::string&)> callback) {
        player_death_callback = callback;
    }
    
    // Get all equipped weapons for an entity
    std::vector<EntityId> get_equipped_weapons(EntityId entity_id);
    
    // Get equipped ranged weapons
    std::vector<EntityId> get_equipped_ranged_weapons(EntityId entity_id);
    
    // Check if entity has any ranged weapon equipped
    bool has_ranged_weapon(EntityId entity_id);
    
    // Get the best (longest range) equipped ranged weapon
    EntityId get_best_ranged_weapon(EntityId entity_id);
    
    // Get range of a ranged weapon
    int get_weapon_range(EntityId weapon_id);
    
    // Perform attack from attacker to target with all equipped weapons
    void perform_attack(EntityId attacker_id, EntityId target_id, bool is_player_attacker = false);
    
    // Perform ranged attack
    bool perform_ranged_attack(EntityId attacker_id, EntityId target_id, EntityId weapon_id, bool is_player_attacker = false);
    
    // Check if target is in range for ranged attack
    bool is_in_range(EntityId attacker_id, EntityId target_id, EntityId weapon_id);
    
    // Check if there's line of sight between two positions
    bool has_line_of_sight(int x1, int y1, int x2, int y2, std::function<bool(int, int)> blocks_sight = nullptr);
    
    // Find ammo for a ranged weapon in inventory
    EntityId find_ammo_for_weapon(EntityId entity_id, EntityId weapon_id);
    
    // Consume ammo (reduce quantity or remove)
    void consume_ammo(EntityId entity_id, EntityId ammo_id);
    
    // Player attacks enemy with all weapons/limbs
    void player_attack_enemy(EntityId player_id, EntityId enemy_id);
    
    // Apply item effect to target (user_id is the entity that used/threw the item, 0 if self-use)
    void apply_effect(EntityId target_id, const ItemEffectComponent::Effect& effect, EntityId user_id = 0);
    
    // Attempt to sever a limb during combat
    void attempt_limb_sever(EntityId attacker_id, EntityId target_id, EntityId weapon_id, bool is_player_attacker);
    
    // Find enemy at position (hostile to the given entity)
    EntityId get_enemy_at(int x, int y, EntityId for_entity = 0);
    
    // Get all enemies in range of a position
    std::vector<EntityId> get_enemies_in_range(EntityId for_entity, int range);
    
    // Get all targetable entities in range
    // Optional filter function: return true to include entity, false to exclude
    // If no filter provided, includes all entities with position component
    std::vector<EntityId> get_targets_in_range(EntityId for_entity, int range, 
        std::function<bool(EntityId, ComponentManager*)> filter = nullptr);
    
    // Spawn a corpse from a dead entity, transferring relevant components
    // Returns the corpse entity ID, or 0 if no corpse was created
    EntityId spawn_corpse(EntityId dead_entity_id);
    
    // Handle entity death - creates corpse if applicable and destroys entity
    void handle_entity_death(EntityId entity_id);
};
