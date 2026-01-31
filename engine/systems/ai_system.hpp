#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "utility_ai.hpp"
#include <vector>
#include <functional>
#include <cmath>
#include <random>
#include <unordered_set>

// AI System - handles entity behavior using Utility AI (IAUS)
class AISystem {
private:
    ComponentManager* manager;
    EntityId player_id;
    std::function<void(EntityId, EntityId)> attack_callback;
    std::function<bool(int, int)> walkable_checker;
    std::mt19937 rng;
    
    // Track which entities have attacked this turn to prevent duplicate attacks
    std::unordered_set<EntityId> entities_that_attacked;

public:
    AISystem(ComponentManager* mgr, EntityId player);
    
    // Set the player entity ID (for soul swap and similar effects)
    void set_player_id(EntityId id) { player_id = id; }
    
    // Set callback for when AI attacks (used for multi-weapon attacks)
    void set_attack_callback(std::function<void(EntityId, EntityId)> callback);
    
    // Set callback to check if a tile is walkable
    void set_walkable_checker(std::function<bool(int, int)> checker);

    void update();

private:
    // Movement and combat helpers
    void chase_target(EntityId entity_id, PositionComponent* pos, PositionComponent* target_pos, float distance);
    void flee_from_target(EntityId entity_id, PositionComponent* pos, PositionComponent* target_pos);
    void wander(EntityId entity_id, PositionComponent* pos);
    void attack_target(EntityId attacker_id, EntityId target_id);
    bool can_move_to(EntityId mover_id, int x, int y);
    
    // Faction-aware target finding
    EntityId find_best_target(EntityId entity_id, PositionComponent* pos, float range);
    EntityId find_ally_to_defend(EntityId entity_id, PositionComponent* pos, float range);
    float get_distance(PositionComponent* a, PositionComponent* b);
    
    // Utility AI (IAUS) methods
    void update_utility_entity(EntityId entity_id);
    void execute_utility_action(EntityId entity_id, UtilityAction& action, UtilityContext& context);
    void build_utility_context(EntityId entity_id, UtilityContext& context);
    void count_nearby_entities(EntityId entity_id, PositionComponent* pos, 
                               UtilityAIComponent* utility_ai, UtilityContext& context);
    EntityId find_ally_attacker(EntityId entity_id, PositionComponent* pos, const UtilityContext& context);
    void alert_allies(EntityId entity_id, EntityId target_id, const UtilityContext& context);
};
