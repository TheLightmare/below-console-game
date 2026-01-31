#include "../systems/ai_system.hpp"
#include "../util/rng.hpp"
#include <cmath>

AISystem::AISystem(ComponentManager* mgr, EntityId player) 
    : manager(mgr), player_id(player), attack_callback(nullptr), walkable_checker(nullptr),
      rng(std::random_device{}()) {}


void AISystem::set_attack_callback(std::function<void(EntityId, EntityId)> callback) {
    attack_callback = callback;
}

void AISystem::set_walkable_checker(std::function<bool(int, int)> checker) {
    walkable_checker = checker;
}

float AISystem::get_distance(PositionComponent* a, PositionComponent* b) {
    if (!a || !b) return 999999.0f;
    float dx = static_cast<float>(b->x - a->x);
    float dy = static_cast<float>(b->y - a->y);
    return std::sqrt(dx * dx + dy * dy);
}

EntityId AISystem::find_best_target(EntityId entity_id, PositionComponent* pos, float range) {
    FactionComponent* my_faction = manager->get_component<FactionComponent>(entity_id);
    if (!my_faction) {
        // No faction = only target player (legacy behavior)
        PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
        if (player_pos && get_distance(pos, player_pos) <= range) {
            return player_id;
        }
        return 0;
    }
    
    FactionManager& fm = get_faction_manager();
    
    EntityId best_target = 0;
    float best_distance = range + 1.0f;
    
    // Check all entities with positions
    auto entities = manager->get_entities_with_component<PositionComponent>();
    
    for (EntityId other_id : entities) {
        if (other_id == entity_id) continue; // Don't target self
        
        PositionComponent* other_pos = manager->get_component<PositionComponent>(other_id);
        StatsComponent* other_stats = manager->get_component<StatsComponent>(other_id);
        FactionComponent* other_faction = manager->get_component<FactionComponent>(other_id);
        
        if (!other_pos || !other_stats || !other_stats->is_alive()) continue;
        
        float distance = get_distance(pos, other_pos);
        if (distance > range) continue;
        
        // Check if we should attack this entity
        bool should_attack = false;
        
        if (other_faction) {
            should_attack = fm.should_attack(my_faction, other_faction, other_id);
        } else {
            // Entity without faction - check if it's the player
            if (other_id == player_id) {
                // Use our faction's relation to the player's actual faction
                FactionComponent* player_faction = manager->get_component<FactionComponent>(player_id);
                FactionId player_faction_id = player_faction ? player_faction->faction : FactionId::PLAYER;
                should_attack = fm.is_hostile(my_faction->faction, player_faction_id);
            }
        }
        
        // Prioritize personal enemies and last attacker
        if (my_faction->is_personal_enemy(other_id) || my_faction->last_attacker == other_id) {
            should_attack = true;
            // Prioritize by making distance seem shorter
            distance *= 0.5f;
        }
        
        if (should_attack && distance < best_distance) {
            best_target = other_id;
            best_distance = distance;
        }
    }
    
    return best_target;
}

EntityId AISystem::find_ally_to_defend(EntityId entity_id, PositionComponent* pos, float range) {
    FactionComponent* my_faction = manager->get_component<FactionComponent>(entity_id);
    if (!my_faction) return 0;
    
    FactionManager& fm = get_faction_manager();
    
    auto entities = manager->get_entities_with_component<PositionComponent>();
    
    for (EntityId other_id : entities) {
        if (other_id == entity_id) continue;
        
        PositionComponent* other_pos = manager->get_component<PositionComponent>(other_id);
        FactionComponent* other_faction = manager->get_component<FactionComponent>(other_id);
        StatsComponent* other_stats = manager->get_component<StatsComponent>(other_id);
        
        if (!other_pos || !other_faction || !other_stats || !other_stats->is_alive()) continue;
        
        float distance = get_distance(pos, other_pos);
        if (distance > range) continue;
        
        // Check if this is an ally we should defend
        if (!fm.should_defend(my_faction, other_faction, other_id)) continue;
        
        // Check if this ally is being attacked (has a last_attacker)
        if (other_faction->last_attacker != 0) {
            // Check if the attacker is still alive and in range
            PositionComponent* attacker_pos = manager->get_component<PositionComponent>(other_faction->last_attacker);
            StatsComponent* attacker_stats = manager->get_component<StatsComponent>(other_faction->last_attacker);
            
            if (attacker_pos && attacker_stats && attacker_stats->is_alive()) {
                float attacker_distance = get_distance(pos, attacker_pos);
                if (attacker_distance <= range * 1.5f) {
                    // Found an ally being attacked - return the attacker as our target
                    return other_faction->last_attacker;
                }
            }
        }
    }
    
    return 0;
}

void AISystem::update() {
    // Reset attack tracking for this turn
    entities_that_attacked.clear();
    
    // Update all entities with UtilityAIComponent
    auto utility_entities = manager->get_entities_with_component<UtilityAIComponent>();
    
    for (EntityId id : utility_entities) {
        update_utility_entity(id);
    }
}


void AISystem::chase_target(EntityId entity_id, PositionComponent* pos, PositionComponent* target_pos, float /*distance*/) {
    // Move towards target
    int dx = 0, dy = 0;
    
    if (target_pos->x > pos->x) dx = 1;
    else if (target_pos->x < pos->x) dx = -1;
    
    if (target_pos->y > pos->y) dy = 1;
    else if (target_pos->y < pos->y) dy = -1;
    
    // Try to move
    int new_x = pos->x + dx;
    int new_y = pos->y + dy;
    
    if (can_move_to(entity_id, new_x, new_y)) {
        pos->x = new_x;
        pos->y = new_y;
    } else if (dx != 0 && can_move_to(entity_id, pos->x + dx, pos->y)) {
        // Try horizontal movement only
        pos->x += dx;
    } else if (dy != 0 && can_move_to(entity_id, pos->x, pos->y + dy)) {
        // Try vertical movement only
        pos->y += dy;
    }
}

void AISystem::flee_from_target(EntityId entity_id, PositionComponent* pos, PositionComponent* target_pos) {
    // Move away from target
    int dx = 0, dy = 0;
    
    if (target_pos->x > pos->x) dx = -1;
    else if (target_pos->x < pos->x) dx = 1;
    
    if (target_pos->y > pos->y) dy = -1;
    else if (target_pos->y < pos->y) dy = 1;
    
    int new_x = pos->x + dx;
    int new_y = pos->y + dy;
    
    if (can_move_to(entity_id, new_x, new_y)) {
        pos->x = new_x;
        pos->y = new_y;
    }
}

void AISystem::wander(EntityId entity_id, PositionComponent* pos) {
    // Random movement
    static int move_timer = 0;
    move_timer++;
    
    // Only move every few frames
    if (move_timer % 10 != 0) return;
    
    int dx = random_int(-1, 1); // -1, 0, or 1
    int dy = random_int(-1, 1);
    
    int new_x = pos->x + dx;
    int new_y = pos->y + dy;
    
    if (can_move_to(entity_id, new_x, new_y)) {
        pos->x = new_x;
        pos->y = new_y;
    }
}

void AISystem::attack_target(EntityId attacker_id, EntityId target_id) {
    // Prevent same entity from attacking multiple times per turn
    if (entities_that_attacked.count(attacker_id) > 0) {
        return;
    }
    entities_that_attacked.insert(attacker_id);
    
    // Record the attack for faction retaliation
    FactionComponent* target_faction = manager->get_component<FactionComponent>(target_id);
    if (target_faction) {
        target_faction->last_attacker = attacker_id;
        
        // Notify allies of the same faction
        FactionManager& fm = get_faction_manager();
        auto entities = manager->get_entities_with_component<FactionComponent>();
        
        for (EntityId ally_id : entities) {
            if (ally_id == target_id || ally_id == attacker_id) continue;
            
            FactionComponent* ally_faction = manager->get_component<FactionComponent>(ally_id);
            if (ally_faction && fm.should_defend(ally_faction, target_faction, target_id)) {
                // This ally should now consider the attacker as an enemy
                ally_faction->add_enemy(attacker_id);
            }
        }
    }
    
    // Use callback if set (for multi-weapon attacks)
    if (attack_callback) {
        attack_callback(attacker_id, target_id);
    } else {
        // Fallback to simple attack
        StatsComponent* attacker_stats = manager->get_component<StatsComponent>(attacker_id);
        StatsComponent* target_stats = manager->get_component<StatsComponent>(target_id);
        
        if (!attacker_stats || !target_stats) return;
        
        // Calculate damage (attack - defense, minimum 1)
        int damage = attacker_stats->attack - target_stats->defense;
        if (damage < 1) damage = 1;
        
        // Deal damage to target
        target_stats->take_damage(damage);
    }
}

bool AISystem::can_move_to(EntityId mover_id, int x, int y) {
    // Check if tile is walkable using world checker
    if (walkable_checker && !walkable_checker(x, y)) {
        return false;
    }
    
    // Check if occupied by another entity with collision using centralized utility
    return !manager->is_position_blocked(x, y, mover_id);
}

// ============================================================================
// IAUS (Infinite Axis Utility System) Implementation
// ============================================================================

void AISystem::update_utility_entity(EntityId entity_id) {
    UtilityAIComponent* utility_ai = manager->get_component<UtilityAIComponent>(entity_id);
    PositionComponent* pos = manager->get_component<PositionComponent>(entity_id);
    StatsComponent* stats = manager->get_component<StatsComponent>(entity_id);
    
    if (!utility_ai || !pos || !stats) return;
    if (!stats->is_alive()) return;
    
    // Build context
    UtilityContext context;
    build_utility_context(entity_id, context);
    
    // Find best target
    EntityId target_id = find_best_target(entity_id, pos, utility_ai->aggro_range);
    if (target_id != 0) {
        context.target_id = target_id;
        context.target_pos = manager->get_component<PositionComponent>(target_id);
        context.target_stats = manager->get_component<StatsComponent>(target_id);
        context.target_faction = manager->get_component<FactionComponent>(target_id);
        
        if (context.target_pos) {
            context.distance_to_target = get_distance(pos, context.target_pos);
        }
    }
    
    // Count nearby entities
    count_nearby_entities(entity_id, pos, utility_ai, context);
    
    // Evaluate all actions and find the best one
    UtilityAction* best_action = nullptr;
    float best_score = utility_ai->score_threshold;
    
    std::uniform_real_distribution<float> noise_dist(0.0f, utility_ai->randomness);
    
    for (auto& action : utility_ai->actions) {
        // Set target for the action
        action.target_id = target_id;
        
        // Evaluate the action
        float score = action.evaluate(context);
        
        // Add randomness if configured
        if (utility_ai->randomness > 0.0f) {
            score += noise_dist(rng);
        }
        
        // Add momentum bonus
        if (utility_ai->use_momentum && action.type == utility_ai->last_action) {
            score += utility_ai->momentum_value;
        }
        
        if (score > best_score) {
            best_score = score;
            best_action = &action;
        }
    }
    
    // Execute the best action
    if (best_action) {
        execute_utility_action(entity_id, *best_action, context);
        
        // Update state
        if (best_action->type == utility_ai->last_action) {
            utility_ai->action_streak++;
        } else {
            utility_ai->action_streak = 1;
        }
        utility_ai->last_action = best_action->type;
        
        // Trigger cooldown
        best_action->trigger_cooldown();
    }
    
    // Tick cooldowns for all actions
    utility_ai->tick_cooldowns();
}

void AISystem::build_utility_context(EntityId entity_id, UtilityContext& context) {
    context.manager = manager;
    context.self_id = entity_id;
    context.player_id = player_id;
    context.self_pos = manager->get_component<PositionComponent>(entity_id);
    context.self_stats = manager->get_component<StatsComponent>(entity_id);
    context.self_faction = manager->get_component<FactionComponent>(entity_id);
    context.self_inventory = manager->get_component<InventoryComponent>(entity_id);
    context.self_equipment = manager->get_component<EquipmentComponent>(entity_id);
    
    // Home component for villagers with homes
    HomeComponent* home = manager->get_component<HomeComponent>(entity_id);
    if (home && context.self_pos) {
        context.has_home = true;
        context.home_x = home->home_x;
        context.home_y = home->home_y;
        context.distance_to_home = home->distance_to_home(context.self_pos->x, context.self_pos->y);
        context.is_at_home = home->is_near_home(context.self_pos->x, context.self_pos->y, 3);
        // TODO: Add proper day/night cycle - for now, placeholder
        context.is_night_time = false;
    } else {
        context.has_home = false;
        context.distance_to_home = 0.0f;
        context.is_at_home = false;
        context.is_night_time = false;
    }
    
    UtilityAIComponent* utility_ai = manager->get_component<UtilityAIComponent>(entity_id);
    context.self_utility_ai = utility_ai;
    if (utility_ai) {
        context.last_action = utility_ai->last_action;
    }
}

void AISystem::execute_utility_action(EntityId entity_id, UtilityAction& action, UtilityContext& context) {
    PositionComponent* pos = context.self_pos;
    
    switch (action.type) {
        case UtilityActionType::IDLE:
            // Do nothing
            break;
            
        case UtilityActionType::WANDER:
            wander(entity_id, pos);
            break;
            
        case UtilityActionType::CHASE:
            if (context.target_pos) {
                chase_target(entity_id, pos, context.target_pos, context.distance_to_target);
            }
            break;
            
        case UtilityActionType::FLEE:
        case UtilityActionType::RETREAT:
            if (context.target_pos) {
                flee_from_target(entity_id, pos, context.target_pos);
            }
            break;
            
        case UtilityActionType::ATTACK_MELEE:
            if (action.target_id != 0 && context.target_in_melee_range()) {
                attack_target(entity_id, action.target_id);
            }
            break;
            
        case UtilityActionType::ATTACK_RANGED:
            if (action.target_id != 0) {
                // Ranged attack - would need line of sight check
                attack_target(entity_id, action.target_id);
            }
            break;
            
        case UtilityActionType::DEFEND_ALLY:
            // Find ally under attack and move toward their attacker
            if (context.ally_under_attack) {
                EntityId attacker = find_ally_attacker(entity_id, pos, context);
                if (attacker != 0) {
                    PositionComponent* attacker_pos = manager->get_component<PositionComponent>(attacker);
                    if (attacker_pos) {
                        if (get_distance(pos, attacker_pos) <= 1.5f) {
                            attack_target(entity_id, attacker);
                        } else {
                            chase_target(entity_id, pos, attacker_pos, get_distance(pos, attacker_pos));
                        }
                    }
                }
            }
            break;
            
        case UtilityActionType::GUARD_POSITION:
            // Stay in place, but attack if enemy comes close
            if (context.target_in_melee_range() && action.target_id != 0) {
                attack_target(entity_id, action.target_id);
            }
            break;
            
        case UtilityActionType::CALL_FOR_HELP:
            // Alert nearby allies - they will target our target
            alert_allies(entity_id, action.target_id, context);
            break;
            
        case UtilityActionType::INVESTIGATE:
            // Move toward last known point of interest (or wander curiously)
            if (context.target_pos) {
                // Move toward target location but stop at a distance
                if (context.distance_to_target > 3.0f) {
                    chase_target(entity_id, pos, context.target_pos, context.distance_to_target);
                }
            } else {
                // No specific target - wander curiously
                wander(entity_id, pos);
            }
            break;
            
        case UtilityActionType::PATROL:
            // For now, patrol is similar to wander
            wander(entity_id, pos);
            break;
            
        case UtilityActionType::RETURN_HOME:
            // Move toward home position
            if (context.has_home && !context.is_at_home) {
                // Create a temporary target position for home
                PositionComponent home_pos;
                home_pos.x = context.home_x;
                home_pos.y = context.home_y;
                home_pos.z = 0;
                chase_target(entity_id, pos, &home_pos, context.distance_to_home);
            }
            break;
            
        case UtilityActionType::SLEEP:
            // Stay in place and do nothing (sleeping)
            // Could add visual indicator or healing here in the future
            break;
            
        case UtilityActionType::CUSTOM:
            if (action.custom_execute) {
                action.custom_execute(entity_id, context);
            }
            break;
            
        default:
            break;
    }
}

void AISystem::count_nearby_entities(EntityId entity_id, PositionComponent* pos, 
                                      UtilityAIComponent* utility_ai, UtilityContext& context) {
    FactionComponent* my_faction = manager->get_component<FactionComponent>(entity_id);
    FactionManager& fm = get_faction_manager();
    
    auto entities = manager->get_entities_with_component<PositionComponent>();
    
    for (EntityId other_id : entities) {
        if (other_id == entity_id) continue;
        
        PositionComponent* other_pos = manager->get_component<PositionComponent>(other_id);
        StatsComponent* other_stats = manager->get_component<StatsComponent>(other_id);
        FactionComponent* other_faction = manager->get_component<FactionComponent>(other_id);
        
        if (!other_pos || !other_stats || !other_stats->is_alive()) continue;
        
        float distance = get_distance(pos, other_pos);
        
        if (my_faction && other_faction) {
            // Check for enemies (faction hostility OR personal enemy)
            if (distance <= utility_ai->aggro_range) {
                if (fm.is_hostile(my_faction->faction, other_faction->faction) ||
                    my_faction->is_personal_enemy(other_id) ||
                    my_faction->last_attacker == other_id) {
                    context.nearby_enemies++;
                }
            }
            
            // Check for allies
            if (distance <= utility_ai->ally_range) {
                if (fm.is_allied(my_faction->faction, other_faction->faction)) {
                    context.nearby_allies++;
                    
                    // Check if ally is under attack
                    if (other_faction->last_attacker != 0) {
                        context.ally_under_attack = true;
                    }
                }
            }
        }
    }
    
    context.in_combat = context.nearby_enemies > 0;
}

EntityId AISystem::find_ally_attacker(EntityId entity_id, PositionComponent* pos, const UtilityContext& context) {
    (void)context; // Reserved for future use
    FactionComponent* my_faction = manager->get_component<FactionComponent>(entity_id);
    if (!my_faction) return 0;
    
    FactionManager& fm = get_faction_manager();
    UtilityAIComponent* utility_ai = manager->get_component<UtilityAIComponent>(entity_id);
    float range = utility_ai ? utility_ai->ally_range : 10.0f;
    
    auto entities = manager->get_entities_with_component<FactionComponent>();
    
    for (EntityId ally_id : entities) {
        if (ally_id == entity_id) continue;
        
        FactionComponent* ally_faction = manager->get_component<FactionComponent>(ally_id);
        PositionComponent* ally_pos = manager->get_component<PositionComponent>(ally_id);
        
        if (!ally_faction || !ally_pos) continue;
        
        float distance = get_distance(pos, ally_pos);
        if (distance > range) continue;
        
        // Check if this is an ally being attacked
        if (fm.is_allied(my_faction->faction, ally_faction->faction) && 
            ally_faction->last_attacker != 0) {
            return ally_faction->last_attacker;
        }
    }
    
    return 0;
}

void AISystem::alert_allies(EntityId entity_id, EntityId target_id, const UtilityContext& context) {
    if (target_id == 0) return;
    
    FactionComponent* my_faction = context.self_faction;
    if (!my_faction) return;
    
    FactionManager& fm = get_faction_manager();
    UtilityAIComponent* utility_ai = manager->get_component<UtilityAIComponent>(entity_id);
    float range = utility_ai ? utility_ai->ally_range : 10.0f;
    
    auto entities = manager->get_entities_with_component<FactionComponent>();
    
    for (EntityId ally_id : entities) {
        if (ally_id == entity_id) continue;
        
        FactionComponent* ally_faction = manager->get_component<FactionComponent>(ally_id);
        PositionComponent* ally_pos = manager->get_component<PositionComponent>(ally_id);
        
        if (!ally_faction || !ally_pos) continue;
        
        float distance = get_distance(context.self_pos, ally_pos);
        if (distance > range) continue;
        
        // Alert allied entities
        if (fm.is_allied(my_faction->faction, ally_faction->faction)) {
            ally_faction->add_enemy(target_id);
        }
    }
}
