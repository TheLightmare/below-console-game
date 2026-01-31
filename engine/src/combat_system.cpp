#include "../systems/combat_system.hpp"
#include "../systems/utility_ai.hpp"
#include "../systems/status_effect_system.hpp"
#include "../ecs/components/status_effects_component.hpp"
#include "../ecs/components/corpse_component.hpp"
#include "../loaders/item_effect_type_registry.hpp"
#include "../util/rng.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>

// Helper function to replace "%target%" placeholder with target name in effect messages
static std::string format_effect_message_for_target(const std::string& message, const std::string& target_name, bool is_player) {
    std::string result = message;
    std::string display_name = is_player ? "You" : target_name;
    
    // Replace all instances of %target% with the display name
    size_t pos = 0;
    while ((pos = result.find("%target%", pos)) != std::string::npos) {
        result.replace(pos, 8, display_name);
        pos += display_name.length();
    }
    
    return result;
}

CombatSystem::CombatSystem(ComponentManager* mgr) : manager(mgr) {}

void CombatSystem::set_message_callback(std::function<void(const std::string&)> callback) {
    message_callback = callback;
}

std::vector<EntityId> CombatSystem::get_equipped_weapons(EntityId entity_id) {
    std::vector<EntityId> weapons;
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    if (!equip) return weapons;
    
    for (const auto& pair : equip->equipped_items) {
        EntityId item_id = pair.second;
        if (item_id == 0) continue;
        
        // Skip if we already have this weapon (avoid duplicates)
        if (std::find(weapons.begin(), weapons.end(), item_id) != weapons.end()) {
            continue;
        }
        
        ItemComponent* item = manager->get_component<ItemComponent>(item_id);
        if (item && item->type == ItemComponent::Type::WEAPON) {
            weapons.push_back(item_id);
        }
    }
    
    return weapons;
}

void CombatSystem::perform_attack(EntityId attacker_id, EntityId target_id, bool is_player_attacker) {
    StatsComponent* attacker_stats = manager->get_component<StatsComponent>(attacker_id);
    StatsComponent* target_stats = manager->get_component<StatsComponent>(target_id);
    NameComponent* attacker_name = manager->get_component<NameComponent>(attacker_id);
    NameComponent* target_name = manager->get_component<NameComponent>(target_id);
    
    if (!attacker_stats || !target_stats) return;
    if (!attacker_stats->is_alive() || !target_stats->is_alive()) return;
    
    // Check if target is invulnerable
    StatusEffectsComponent* target_status = manager->get_component<StatusEffectsComponent>(target_id);
    if (target_status && target_status->is_invulnerable()) {
        if (message_callback) {
            std::string t_name = target_name ? target_name->name : "target";
            bool is_player_target = (player_id != 0 && target_id == player_id);
            if (is_player_target) {
                message_callback("The attack bounces harmlessly off your protective aura!");
            } else {
                message_callback("The attack bounces harmlessly off " + t_name + "!");
            }
        }
        return;
    }
    
    // Get status effect modifiers for attacker and target
    StatusEffectsComponent* attacker_status = manager->get_component<StatusEffectsComponent>(attacker_id);
    int attack_bonus = attacker_status ? attacker_status->attack_modifier : 0;
    int defense_bonus = target_status ? target_status->defense_modifier : 0;
    
    std::string a_name = attacker_name ? attacker_name->name : "Entity #" + std::to_string(attacker_id);
    std::string t_name = target_name ? target_name->name : "Entity #" + std::to_string(target_id);
    
    // Determine if player is the target
    bool is_player_target = (player_id != 0 && target_id == player_id);
    
    std::vector<EntityId> weapons = get_equipped_weapons(attacker_id);
    
    if (weapons.empty()) {
        int total_attack = attacker_stats->attack + attack_bonus;
        int total_defense = target_stats->defense + defense_bonus;
        int damage = total_attack - total_defense;
        if (damage < 1) damage = 1;
        
        target_stats->take_damage(damage);
        
        if (message_callback) {
            if (is_player_attacker) {
                message_callback("You hit the " + t_name + " for " + std::to_string(damage) + " damage!");
            } else if (is_player_target) {
                message_callback("The " + a_name + " hits you for " + std::to_string(damage) + " damage!");
            } else {
                message_callback("The " + a_name + " hits the " + t_name + " for " + std::to_string(damage) + " damage.");
            }
        }
    } else {
        for (EntityId weapon_id : weapons) {
            if (!target_stats->is_alive()) break;
            
            ItemComponent* weapon = manager->get_component<ItemComponent>(weapon_id);
            NameComponent* weapon_name = manager->get_component<NameComponent>(weapon_id);
            ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(weapon_id);
            
            if (!weapon) continue;
            
            // Damage is weapon-dependent with attack level modifier
            // Base damage from weapon + 10% of attack stat per level
            // Include status effect attack bonus
            int total_attack = attacker_stats->attack + attack_bonus;
            float attack_mod = 1.0f + (total_attack * 0.05f);  // 5% bonus per attack point
            int base_dmg = weapon->base_damage > 0 ? weapon->base_damage : weapon->attack_bonus;
            int total_defense = target_stats->defense + defense_bonus;
            int damage = static_cast<int>(base_dmg * attack_mod) - total_defense;
            if (damage < 1) damage = 1;
            
            target_stats->take_damage(damage);
            
            std::string w_name = weapon_name ? weapon_name->name : "weapon";
            
            if (message_callback) {
                if (is_player_attacker) {
                    message_callback("You attack with " + w_name + " for " + std::to_string(damage) + " damage!");
                } else if (is_player_target) {
                    message_callback("The " + a_name + " attacks you with " + w_name + " for " + std::to_string(damage) + " damage!");
                } else {
                    message_callback("The " + a_name + " attacks the " + t_name + " with " + w_name + " for " + std::to_string(damage) + " damage.");
                }
            }
            
            // Attempt limb severing if weapon has cutting chance
            if (weapon->cutting_chance > 0.0f) {
                attempt_limb_sever(attacker_id, target_id, weapon_id, is_player_attacker);
            }
            
            if (effects) {
                auto on_hit_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_HIT);
                for (const auto& effect : on_hit_effects) {
                    if (effect.chance < 1.0f) {
                        if (!chance(effect.chance)) continue;
                    }
                    
                    apply_effect(target_id, effect);
                }
            }
        }
    }
    
    if (!target_stats->is_alive()) {
        int xp_reward = target_stats->level * 50;
        attacker_stats->gain_experience(xp_reward);
        
        if (message_callback) {
            if (is_player_attacker) {
                message_callback("The " + t_name + " is defeated! Gained " + std::to_string(xp_reward) + " XP!");
            } else if (is_player_target) {
                message_callback("You have been defeated!");
            } else {
                message_callback("The " + a_name + " defeats the " + t_name + "!");
            }
        }
        
        // Report player death with reason
        if (is_player_target && player_death_callback) {
            player_death_callback("Slain by " + a_name);
        }
        
        // Handle death (creates corpse if applicable, then destroys entity)
        // But not the player - that's handled separately
        if (!is_player_target) {
            handle_entity_death(target_id);
        }
    }
}

void CombatSystem::apply_effect(EntityId target_id, const ItemEffectComponent::Effect& effect, EntityId user_id) {
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "Target";
    bool is_player = (player_id != 0 && target_id == player_id);
    
    // Format custom message if provided
    std::string custom_msg = effect.message.empty() ? "" : 
        format_effect_message_for_target(effect.message, target_name, is_player);
    
    // Get effect type ID from enum
    std::string effect_type_id = item_effect_type_to_string(static_cast<int>(effect.type));
    
    // Look up effect definition in registry
    const ItemEffectTypeDefinition* def = ItemEffectTypeRegistry::instance().get(effect_type_id);
    
    // Helper lambda to get the appropriate message
    auto get_message = [&](int amount) -> std::string {
        if (!custom_msg.empty()) {
            return custom_msg;
        }
        if (def) {
            const std::string& tmpl = is_player ? def->message_player : def->message_other;
            if (!tmpl.empty()) {
                return ItemEffectTypeRegistry::format_message(tmpl, target_name, amount);
            }
        }
        return "";
    };
    
    // If no definition found, fall back to legacy handling for CUSTOM type
    if (!def) {
        if (effect.type == ItemEffectComponent::EffectType::CUSTOM) {
            auto callback = effect.get_callback();
            if (callback) {
                auto formatted_callback = [this, &target_name, is_player](const std::string& msg) {
                    if (message_callback) {
                        message_callback(format_effect_message_for_target(msg, target_name, is_player));
                    }
                };
                callback(target_id, user_id, manager, formatted_callback);
            } else if (message_callback && !effect.message.empty()) {
                message_callback(custom_msg);
            }
        }
        return;
    }
    
    StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
    
    // Process based on behavior type from JSON
    if (def->behavior == "instant_heal") {
        if (status_effect_system) {
            // Use status effect system for unified handling
            StatusEffectType se_type = def->is_percent ? StatusEffectType::HEAL_PERCENT : StatusEffectType::HEAL;
            status_effect_system->apply_effect(target_id, se_type, effect.magnitude, 0, user_id, false, custom_msg);
        } else if (stats) {
            int amount = def->is_percent ? (stats->max_hp * effect.magnitude) / 100 : effect.magnitude;
            int healed = stats->heal(amount);
            if (message_callback) {
                std::string msg = get_message(healed);
                if (!msg.empty()) message_callback(msg);
            }
        }
    }
    else if (def->behavior == "instant_damage") {
        if (status_effect_system) {
            status_effect_system->apply_effect(target_id, StatusEffectType::DAMAGE, effect.magnitude, 0, user_id, false, custom_msg);
        } else if (stats) {
            stats->take_damage(effect.magnitude);
            if (message_callback) {
                std::string msg = get_message(effect.magnitude);
                if (!msg.empty()) message_callback(msg);
            }
        }
    }
    else if (def->behavior == "instant_stat") {
        if (def->stat == "experience" && status_effect_system) {
            status_effect_system->apply_effect(target_id, StatusEffectType::GAIN_XP, effect.magnitude, 0, user_id, false, custom_msg);
        } else if (stats) {
            if (def->stat == "experience") {
                stats->gain_experience(effect.magnitude);
            } else if (def->stat == "attack") {
                if (def->modifier_type == "add") stats->attack += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->attack = std::max(1, stats->attack - effect.magnitude);
            } else if (def->stat == "defense") {
                if (def->modifier_type == "add") stats->defense += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->defense = std::max(0, stats->defense - effect.magnitude);
            }
            if (message_callback) {
                std::string msg = get_message(effect.magnitude);
                if (!msg.empty()) message_callback(msg);
            }
        }
    }
    else if (def->behavior == "stat_modifier") {
        // If duration specified, apply as status effect; otherwise instant
        if (status_effect_system && effect.duration > 0 && !def->status_effect_id.empty()) {
            status_effect_system->apply_effect(target_id, def->status_effect_id, 
                effect.magnitude, effect.duration, user_id, false, custom_msg);
        } else if (stats) {
            if (def->stat == "attack") {
                if (def->modifier_type == "add") stats->attack += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->attack = std::max(1, stats->attack - effect.magnitude);
            } else if (def->stat == "defense") {
                if (def->modifier_type == "add") stats->defense += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->defense = std::max(0, stats->defense - effect.magnitude);
            }
            if (message_callback) {
                std::string msg = get_message(effect.magnitude);
                if (!msg.empty()) message_callback(msg);
            }
        }
    }
    else if (def->behavior == "apply_status") {
        if (status_effect_system && !def->status_effect_id.empty()) {
            int duration = effect.duration > 0 ? effect.duration : def->default_duration;
            status_effect_system->apply_effect(target_id, def->status_effect_id, 
                effect.magnitude, duration, user_id, false, custom_msg);
        }
    }
    else if (def->behavior == "custom") {
        // Custom behavior - use callback or custom handler name
        auto callback = effect.get_callback();
        if (callback) {
            auto formatted_callback = [this, &target_name, is_player](const std::string& msg) {
                if (message_callback) {
                    message_callback(format_effect_message_for_target(msg, target_name, is_player));
                }
            };
            callback(target_id, user_id, manager, formatted_callback);
        } else if (message_callback) {
            std::string msg = get_message(effect.magnitude);
            if (!msg.empty()) message_callback(msg);
        }
    }
    else {
        // Unknown behavior - just show message
        if (message_callback) {
            std::string msg = get_message(effect.magnitude);
            if (!msg.empty()) message_callback(msg);
        }
    }
}

EntityId CombatSystem::get_enemy_at(int x, int y, EntityId for_entity) {
    auto entities = manager->get_entities_with_component<UtilityAIComponent>();
    
    // Get the faction of the entity we're checking for (usually the player)
    FactionComponent* for_faction = nullptr;
    if (for_entity != 0) {
        for_faction = manager->get_component<FactionComponent>(for_entity);
    }
    
    FactionManager& fm = get_faction_manager();
    
    for (EntityId id : entities) {
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        if (pos && pos->x == x && pos->y == y) {
            // If no for_entity specified, return any AI entity (legacy behavior)
            if (for_entity == 0) {
                return id;
            }
            
            // Check if this entity is hostile to for_entity
            FactionComponent* target_faction = manager->get_component<FactionComponent>(id);
            
            if (target_faction && for_faction) {
                // Use faction system to determine hostility (both directions)
                // Either we want to attack them, OR they have us as an enemy
                if (fm.should_attack(for_faction, target_faction, id) ||
                    target_faction->is_personal_enemy(for_entity) ||
                    target_faction->last_attacker == for_entity) {
                    return id;
                }
            } else if (!target_faction) {
                // Entity without faction - treat as hostile (monsters, etc.)
                return id;
            }
            // If target has faction but for_entity doesn't, don't auto-attack
            // (shouldn't happen for player, but safe fallback)
        }
    }
    return 0;
}

void CombatSystem::player_attack_enemy(EntityId player_id, EntityId enemy_id) {
    StatsComponent* player_stats = manager->get_component<StatsComponent>(player_id);
    StatsComponent* enemy_stats = manager->get_component<StatsComponent>(enemy_id);
    NameComponent* enemy_name = manager->get_component<NameComponent>(enemy_id);
    AnatomyComponent* player_anatomy = manager->get_component<AnatomyComponent>(player_id);
    EquipmentComponent* player_equip = manager->get_component<EquipmentComponent>(player_id);
    
    if (!player_stats || !enemy_stats) return;
    
    // Record the attack for faction retaliation
    FactionComponent* enemy_faction = manager->get_component<FactionComponent>(enemy_id);
    if (enemy_faction) {
        enemy_faction->last_attacker = player_id;
        enemy_faction->add_enemy(player_id);
        
        // Notify nearby allies of the same faction
        FactionManager& fm = get_faction_manager();
        auto entities = manager->get_entities_with_component<FactionComponent>();
        
        for (EntityId ally_id : entities) {
            if (ally_id == enemy_id || ally_id == player_id) continue;
            
            FactionComponent* ally_faction = manager->get_component<FactionComponent>(ally_id);
            PositionComponent* ally_pos = manager->get_component<PositionComponent>(ally_id);
            PositionComponent* enemy_pos = manager->get_component<PositionComponent>(enemy_id);
            
            if (ally_faction && ally_pos && enemy_pos) {
                // Check if ally is close enough to notice (within 15 tiles)
                int dx = std::abs(ally_pos->x - enemy_pos->x);
                int dy = std::abs(ally_pos->y - enemy_pos->y);
                float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                
                if (distance <= 15.0f && fm.should_defend(ally_faction, enemy_faction, enemy_id)) {
                    // This ally should now consider the player as an enemy
                    ally_faction->add_enemy(player_id);
                }
            }
        }
    }
    
    std::string name = enemy_name ? enemy_name->name : "Enemy";
    
    // Get all equipped weapons
    std::vector<EntityId> weapons = get_equipped_weapons(player_id);
    
    if (weapons.empty()) {
        // Unarmed attack - hit with all free hands/arms
        int num_strikes = 1; // Default to 1 strike
        
        if (player_anatomy && player_equip) {
            // Count free hands that can attack
            auto hands = player_anatomy->get_limbs_of_type(LimbType::HAND);
            int free_hands = 0;
            for (auto* hand : hands) {
                if (player_equip->get_equipped(hand->name) == 0) {
                    free_hands++;
                }
            }
            
            // If no free hands, try arms
            if (free_hands == 0) {
                auto arms = player_anatomy->get_limbs_of_type(LimbType::ARM);
                for (auto* arm : arms) {
                    if (player_equip->get_equipped(arm->name) == 0) {
                        free_hands++;
                    }
                }
            }
            
            if (free_hands > 0) {
                num_strikes = free_hands;
            }
        }
        
        // Strike with each free hand/arm
        for (int i = 0; i < num_strikes; ++i) {
            if (!enemy_stats->is_alive()) break;
            
            int damage = player_stats->attack - enemy_stats->defense;
            if (damage < 1) damage = 1;
            
            enemy_stats->take_damage(damage);
            
            if (message_callback) {
                if (num_strikes > 1) {
                    message_callback("You punch the " + name + " for " + std::to_string(damage) + " damage! (strike " + std::to_string(i + 1) + "/" + std::to_string(num_strikes) + ")");
                } else {
                    message_callback("You punch the " + name + " for " + std::to_string(damage) + " damage!");
                }
            }
        }
    } else {
        // Attack with each weapon
        for (EntityId weapon_id : weapons) {
            ItemComponent* weapon = manager->get_component<ItemComponent>(weapon_id);
            NameComponent* weapon_name = manager->get_component<NameComponent>(weapon_id);
            
            if (!weapon) continue;
            
            // Calculate damage with weapon bonus
            int weapon_attack = player_stats->attack + weapon->attack_bonus;
            int damage = weapon_attack - enemy_stats->defense;
            if (damage < 1) damage = 1;
            
            enemy_stats->take_damage(damage);
            
            std::string w_name = weapon_name ? weapon_name->name : "weapon";
            if (message_callback) {
                message_callback("You hit the " + name + " with your " + w_name + " for " + std::to_string(damage) + " damage!");
            }
            
            // Attempt limb severing if weapon has cutting chance
            if (weapon->cutting_chance > 0.0f) {
                attempt_limb_sever(player_id, enemy_id, weapon_id, true);
            }
            
            // Check if enemy died from this strike
            if (!enemy_stats->is_alive()) break;
        }
    }
    
    // Check if enemy died
    if (!enemy_stats->is_alive()) {
        if (message_callback) {
            message_callback("You defeated the " + name + "!");
        }
        
        // Award XP and gold
        int xp_gained = enemy_stats->level * 10;
        int gold_gained = enemy_stats->level * 5;
        
        player_stats->gain_experience(xp_gained);
        if (message_callback) {
            message_callback("You gained " + std::to_string(xp_gained) + " XP!");
        }
        
        InventoryComponent* player_inv = manager->get_component<InventoryComponent>(player_id);
        if (player_inv) {
            player_inv->gold += gold_gained;
            if (message_callback) {
                message_callback("You gained " + std::to_string(gold_gained) + " gold!");
            }
        }
        
        // Handle death (creates corpse if applicable, then destroys entity)
        handle_entity_death(enemy_id);
    } else {
        // Enemy counter-attacks with all their weapons
        perform_attack(enemy_id, player_id, false);
    }
}

void CombatSystem::attempt_limb_sever(EntityId attacker_id, EntityId target_id, EntityId weapon_id, bool is_player_attacker) {
    (void)attacker_id;  // Reserved for future use
    ItemComponent* weapon = manager->get_component<ItemComponent>(weapon_id);
    AnatomyComponent* target_anatomy = manager->get_component<AnatomyComponent>(target_id);
    NameComponent* target_name = manager->get_component<NameComponent>(target_id);
    NameComponent* weapon_name = manager->get_component<NameComponent>(weapon_id);
    
    if (!weapon || !target_anatomy) return;
    if (weapon->cutting_chance <= 0.0f) return;
    
    // Collect all functional limbs that can be severed
    std::vector<Limb*> severable_limbs;
    for (auto& limb : target_anatomy->limbs) {
        if (limb.functional && limb.type != LimbType::TORSO && limb.type != LimbType::HEAD) {
            severable_limbs.push_back(&limb);
        }
    }
    
    if (severable_limbs.empty()) return;
    
    // Pick a random limb
    Limb* target_limb = severable_limbs[random_index(severable_limbs.size())];
    
    // Roll for severing: weapon cutting_chance vs limb severing_resistance
    float sever_threshold = weapon->cutting_chance * (1.0f - target_limb->severing_resistance);
    
    if (chance(sever_threshold)) {
        // Limb is severed!
        target_limb->functional = false;
        
        // Unequip any item on that limb
        EquipmentComponent* target_equip = manager->get_component<EquipmentComponent>(target_id);
        if (target_equip) {
            EntityId equipped = target_equip->unequip_from_limb(target_limb->name);
            if (equipped != 0) {
                // Drop the item at target's position
                PositionComponent* target_pos = manager->get_component<PositionComponent>(target_id);
                if (target_pos) {
                    PositionComponent item_pos{target_pos->x, target_pos->y, target_pos->z};
                    manager->add_component<PositionComponent>(equipped, item_pos);
                }
            }
        }
        
        std::string t_name = target_name ? target_name->name : "Enemy";
        std::string w_name = weapon_name ? weapon_name->name : "weapon";
        
        if (message_callback) {
            if (is_player_attacker) {
                message_callback("Your " + w_name + " severs the " + t_name + "'s " + target_limb->name + "!");
            } else {
                message_callback("The " + t_name + "'s " + w_name + " severs your " + target_limb->name + "!");
            }
        }
        
        // Apply damage from blood loss
        StatsComponent* target_stats = manager->get_component<StatsComponent>(target_id);
        if (target_stats) {
            int blood_loss = 5; // Fixed damage from severing
            target_stats->take_damage(blood_loss);
            if (message_callback) {
                if (is_player_attacker) {
                    message_callback("The " + t_name + " loses " + std::to_string(blood_loss) + " HP from blood loss!");
                } else {
                    message_callback("You lose " + std::to_string(blood_loss) + " HP from blood loss!");
                }
            }
        }
    }
}
// ==================== Ranged Attack Methods ====================

std::vector<EntityId> CombatSystem::get_equipped_ranged_weapons(EntityId entity_id) {
    std::vector<EntityId> ranged_weapons;
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    if (!equip) return ranged_weapons;
    
    for (const auto& pair : equip->equipped_items) {
        EntityId item_id = pair.second;
        ItemComponent* item = manager->get_component<ItemComponent>(item_id);
        RangedWeaponComponent* ranged = manager->get_component<RangedWeaponComponent>(item_id);
        if (item && item->type == ItemComponent::Type::WEAPON && ranged) {
            ranged_weapons.push_back(item_id);
        }
    }
    
    return ranged_weapons;
}

bool CombatSystem::has_ranged_weapon(EntityId entity_id) {
    return !get_equipped_ranged_weapons(entity_id).empty();
}

EntityId CombatSystem::get_best_ranged_weapon(EntityId entity_id) {
    auto weapons = get_equipped_ranged_weapons(entity_id);
    if (weapons.empty()) return 0;
    
    EntityId best = weapons[0];
    int best_range = get_weapon_range(best);
    
    for (size_t i = 1; i < weapons.size(); ++i) {
        int range = get_weapon_range(weapons[i]);
        if (range > best_range) {
            best = weapons[i];
            best_range = range;
        }
    }
    
    return best;
}

int CombatSystem::get_weapon_range(EntityId weapon_id) {
    RangedWeaponComponent* ranged = manager->get_component<RangedWeaponComponent>(weapon_id);
    return ranged ? ranged->range : 1;
}

bool CombatSystem::is_in_range(EntityId attacker_id, EntityId target_id, EntityId weapon_id) {
    PositionComponent* attacker_pos = manager->get_component<PositionComponent>(attacker_id);
    PositionComponent* target_pos = manager->get_component<PositionComponent>(target_id);
    RangedWeaponComponent* ranged = manager->get_component<RangedWeaponComponent>(weapon_id);
    
    if (!attacker_pos || !target_pos || !ranged) return false;
    
    int dx = std::abs(target_pos->x - attacker_pos->x);
    int dy = std::abs(target_pos->y - attacker_pos->y);
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
    
    return distance >= ranged->min_range && distance <= ranged->range;
}

bool CombatSystem::has_line_of_sight(int x1, int y1, int x2, int y2, std::function<bool(int, int)> blocks_sight) {
    // Bresenham's line algorithm to check line of sight
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    int x = x1;
    int y = y1;
    
    while (x != x2 || y != y2) {
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
        
        // Don't check the final target position
        if (x == x2 && y == y2) break;
        
        // Check if this tile blocks sight
        if (blocks_sight && blocks_sight(x, y)) {
            return false;
        }
    }
    
    return true;
}

EntityId CombatSystem::find_ammo_for_weapon(EntityId entity_id, EntityId weapon_id) {
    RangedWeaponComponent* ranged = manager->get_component<RangedWeaponComponent>(weapon_id);
    if (!ranged || !ranged->requires_ammo()) return 0;
    
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    if (!inv) return 0;
    
    for (EntityId item_id : inv->items) {
        AmmoComponent* ammo = manager->get_component<AmmoComponent>(item_id);
        if (ammo && ammo->ammo_type == ranged->ammo_type && ammo->quantity > 0) {
            return item_id;
        }
    }
    
    return 0;
}

void CombatSystem::consume_ammo(EntityId entity_id, EntityId ammo_id) {
    AmmoComponent* ammo = manager->get_component<AmmoComponent>(ammo_id);
    if (!ammo) return;
    
    ammo->quantity--;
    
    if (ammo->quantity <= 0) {
        InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
        if (inv) {
            inv->remove_item(ammo_id);
        }
        manager->destroy_entity(ammo_id);
    }
}

bool CombatSystem::perform_ranged_attack(EntityId attacker_id, EntityId target_id, EntityId weapon_id, bool is_player_attacker) {
    StatsComponent* attacker_stats = manager->get_component<StatsComponent>(attacker_id);
    StatsComponent* target_stats = manager->get_component<StatsComponent>(target_id);
    NameComponent* attacker_name = manager->get_component<NameComponent>(attacker_id);
    NameComponent* target_name = manager->get_component<NameComponent>(target_id);
    NameComponent* weapon_name = manager->get_component<NameComponent>(weapon_id);
    ItemComponent* weapon = manager->get_component<ItemComponent>(weapon_id);
    RangedWeaponComponent* ranged = manager->get_component<RangedWeaponComponent>(weapon_id);
    
    if (!attacker_stats || !target_stats || !weapon || !ranged) return false;
    if (!attacker_stats->is_alive() || !target_stats->is_alive()) return false;
    
    std::string a_name = attacker_name ? attacker_name->name : "Unknown";
    std::string t_name = target_name ? target_name->name : "Enemy";
    std::string w_name = weapon_name ? weapon_name->name : "ranged weapon";
    
    bool is_player_target = (player_id != 0 && target_id == player_id);
    
    // Check range
    if (!is_in_range(attacker_id, target_id, weapon_id)) {
        if (message_callback && is_player_attacker) {
            message_callback("Target is out of range!");
        }
        return false;
    }
    
    // Check ammo
    int ammo_damage_bonus = 0;
    EntityId ammo_id = 0;
    if (ranged->requires_ammo()) {
        ammo_id = find_ammo_for_weapon(attacker_id, weapon_id);
        if (ammo_id == 0) {
            if (message_callback && is_player_attacker) {
                message_callback("No ammo!");
            }
            return false;
        }
        AmmoComponent* ammo = manager->get_component<AmmoComponent>(ammo_id);
        if (ammo) {
            ammo_damage_bonus = ammo->damage_bonus;
        }
    }
    
    // Record attack for faction retaliation
    FactionComponent* target_faction = manager->get_component<FactionComponent>(target_id);
    if (target_faction) {
        target_faction->last_attacker = attacker_id;
        target_faction->add_enemy(attacker_id);
    }
    
    // Calculate hit chance (base 80% + accuracy bonus, penalty for distance)
    PositionComponent* attacker_pos = manager->get_component<PositionComponent>(attacker_id);
    PositionComponent* target_pos = manager->get_component<PositionComponent>(target_id);
    float distance = 1.0f;
    if (attacker_pos && target_pos) {
        int dx = std::abs(target_pos->x - attacker_pos->x);
        int dy = std::abs(target_pos->y - attacker_pos->y);
        distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
    }
    
    float hit_chance = 0.80f + (ranged->accuracy_bonus * 0.05f) - (distance * 0.02f);
    if (hit_chance < 0.1f) hit_chance = 0.1f;
    if (hit_chance > 0.95f) hit_chance = 0.95f;
    
    bool hit = chance(hit_chance);
    
    // Consume ammo before checking hit
    if (ammo_id != 0) {
        consume_ammo(attacker_id, ammo_id);
    }
    
    // Handle thrown weapons that consume themselves
    if (ranged->consumes_self) {
        EquipmentComponent* equip = manager->get_component<EquipmentComponent>(attacker_id);
        if (equip) {
            // Find and unequip the weapon
            for (auto& pair : equip->equipped_items) {
                if (pair.second == weapon_id) {
                    equip->equipped_items.erase(pair.first);
                    break;
                }
            }
        }
        // Drop the weapon at target location (or destroy it)
        if (target_pos) {
            manager->add_component<PositionComponent>(weapon_id, PositionComponent{target_pos->x, target_pos->y, 0});
        }
    }
    
    if (!hit) {
        if (message_callback) {
            if (is_player_attacker) {
                message_callback("Your shot misses the " + t_name + "!");
            } else if (is_player_target) {
                message_callback("The " + a_name + "'s shot misses you!");
            }
        }
        return true; // Attack happened but missed
    }
    
    // Calculate damage
    int total_attack = attacker_stats->attack + weapon->attack_bonus + ammo_damage_bonus;
    int damage = total_attack - target_stats->defense;
    if (damage < 1) damage = 1;
    
    target_stats->take_damage(damage);
    
    if (message_callback) {
        if (is_player_attacker) {
            message_callback("You shoot the " + t_name + " with your " + w_name + " for " + std::to_string(damage) + " damage!");
        } else if (is_player_target) {
            message_callback("The " + a_name + " shoots you with " + w_name + " for " + std::to_string(damage) + " damage!");
        } else {
            message_callback("The " + a_name + " shoots the " + t_name + " for " + std::to_string(damage) + " damage.");
        }
    }
    
    // Apply on-hit effects
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(weapon_id);
    if (effects) {
        auto on_hit_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_HIT);
        for (const auto& effect : on_hit_effects) {
            if (effect.chance < 1.0f) {
                if (!chance(effect.chance)) continue;
            }
            apply_effect(target_id, effect);
        }
    }
    
    // Check if target died
    if (!target_stats->is_alive()) {
        int xp_reward = target_stats->level * 50;
        attacker_stats->gain_experience(xp_reward);
        
        if (message_callback) {
            if (is_player_attacker) {
                message_callback("The " + t_name + " is defeated! Gained " + std::to_string(xp_reward) + " XP!");
            } else if (is_player_target) {
                message_callback("You have been defeated!");
            }
        }
        
        // Report player death with reason
        if (is_player_target && player_death_callback) {
            player_death_callback("Shot down by " + a_name);
        }
        
        if (!is_player_target) {
            // Award gold
            InventoryComponent* attacker_inv = manager->get_component<InventoryComponent>(attacker_id);
            if (attacker_inv) {
                int gold_gained = target_stats->level * 5;
                attacker_inv->gold += gold_gained;
                if (message_callback && is_player_attacker) {
                    message_callback("You gained " + std::to_string(gold_gained) + " gold!");
                }
            }
            handle_entity_death(target_id);
        }
    }
    
    return true;
}

std::vector<EntityId> CombatSystem::get_enemies_in_range(EntityId for_entity, int range) {
    std::vector<EntityId> enemies;
    
    PositionComponent* for_pos = manager->get_component<PositionComponent>(for_entity);
    FactionComponent* for_faction = manager->get_component<FactionComponent>(for_entity);
    if (!for_pos) return enemies;
    
    FactionManager& fm = get_faction_manager();
    auto entities = manager->get_entities_with_component<UtilityAIComponent>();
    
    for (EntityId id : entities) {
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        StatsComponent* stats = manager->get_component<StatsComponent>(id);
        if (!pos || !stats || !stats->is_alive()) continue;
        
        int dx = std::abs(pos->x - for_pos->x);
        int dy = std::abs(pos->y - for_pos->y);
        float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
        
        if (distance > range) continue;
        
        // Check hostility
        FactionComponent* target_faction = manager->get_component<FactionComponent>(id);
        bool is_hostile = false;
        
        if (target_faction && for_faction) {
            is_hostile = fm.should_attack(for_faction, target_faction, id) ||
                        target_faction->is_personal_enemy(for_entity);
        } else if (!target_faction) {
            // No faction = hostile
            is_hostile = true;
        }
        
        if (is_hostile) {
            enemies.push_back(id);
        }
    }
    
    return enemies;
}

std::vector<EntityId> CombatSystem::get_targets_in_range(EntityId for_entity, int range, 
    std::function<bool(EntityId, ComponentManager*)> filter) {
    std::vector<EntityId> targets;
    
    PositionComponent* for_pos = manager->get_component<PositionComponent>(for_entity);
    if (!for_pos) return targets;
    
    // Get all entities with position components
    auto entities = manager->get_entities_with_component<PositionComponent>();
    
    for (EntityId id : entities) {
        // Don't target self
        if (id == for_entity) continue;
        
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        if (!pos) continue;
        
        int dx = std::abs(pos->x - for_pos->x);
        int dy = std::abs(pos->y - for_pos->y);
        float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
        
        if (distance <= range && distance >= 1) {
            // Apply filter if provided, otherwise include all
            if (filter) {
                if (filter(id, manager)) {
                    targets.push_back(id);
                }
            } else {
                targets.push_back(id);
            }
        }
    }
    
    return targets;
}

EntityId CombatSystem::spawn_corpse(EntityId dead_entity_id) {
    LeavesCorpseComponent* leaves_corpse = manager->get_component<LeavesCorpseComponent>(dead_entity_id);
    if (!leaves_corpse) {
        return 0;  // Entity doesn't leave a corpse
    }
    
    PositionComponent* pos = manager->get_component<PositionComponent>(dead_entity_id);
    if (!pos) {
        return 0;  // Need position to place corpse
    }
    
    NameComponent* name = manager->get_component<NameComponent>(dead_entity_id);
    std::string entity_name = name ? name->name : "creature";
    
    EntityId corpse_id = 0;
    
    // Try to create from template if entity loader is available
    if (entity_loader) {
        Entity corpse_entity = entity_loader->create_entity_from_template(manager, "corpse", pos->x, pos->y);
        corpse_id = corpse_entity.get_id();
    }
    
    // Fallback: create manually if template failed or no loader
    if (corpse_id == 0) {
        corpse_id = manager->create_entity();
        manager->add_component(corpse_id, PositionComponent{pos->x, pos->y, pos->z});
        manager->add_component(corpse_id, CollisionComponent{false, false});
        
        // Default inventory
        InventoryComponent inv;
        inv.max_items = 20;
        manager->add_component(corpse_id, inv);
        
        // Default equipment
        manager->add_component(corpse_id, EquipmentComponent{});
        
        // Default corpse component
        CorpseComponent corpse_comp;
        manager->add_component(corpse_id, corpse_comp);
    }
    
    // Override/customize the corpse with dead entity's data
    std::string corpse_name = leaves_corpse->corpse_name.empty() ? 
        make_corpse_name(entity_name) : leaves_corpse->corpse_name;
    std::string corpse_desc = leaves_corpse->corpse_description.empty() ?
        make_corpse_description(entity_name) : leaves_corpse->corpse_description;
    
    // Update render component
    RenderComponent* render = manager->get_component<RenderComponent>(corpse_id);
    if (render) {
        render->ch = leaves_corpse->corpse_symbol;
        render->color = leaves_corpse->corpse_color;
    } else {
        RenderComponent new_render;
        new_render.ch = leaves_corpse->corpse_symbol;
        new_render.color = leaves_corpse->corpse_color;
        new_render.priority = 2;
        manager->add_component(corpse_id, new_render);
    }
    
    // Update name component
    NameComponent* name_comp = manager->get_component<NameComponent>(corpse_id);
    if (name_comp) {
        name_comp->name = corpse_name;
        name_comp->description = corpse_desc;
    } else {
        manager->add_component(corpse_id, NameComponent{corpse_name, corpse_desc});
    }
    
    // Update corpse marker component
    CorpseComponent* corpse_comp = manager->get_component<CorpseComponent>(corpse_id);
    if (corpse_comp) {
        corpse_comp->original_entity_name = entity_name;
        corpse_comp->original_entity = dead_entity_id;
        corpse_comp->decay_turns_remaining = leaves_corpse->decay_turns;
    }
    
    // Transfer anatomy if configured
    if (leaves_corpse->transfer_anatomy) {
        AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(dead_entity_id);
        if (anatomy) {
            // Remove existing anatomy if any
            if (manager->get_component<AnatomyComponent>(corpse_id)) {
                manager->remove_component<AnatomyComponent>(corpse_id);
            }
            manager->add_component(corpse_id, *anatomy);
        }
    }
    
    // Transfer inventory if configured
    if (leaves_corpse->transfer_inventory) {
        InventoryComponent* inv = manager->get_component<InventoryComponent>(dead_entity_id);
        if (inv) {
            // Remove existing inventory and replace with dead entity's
            if (manager->get_component<InventoryComponent>(corpse_id)) {
                manager->remove_component<InventoryComponent>(corpse_id);
            }
            manager->add_component(corpse_id, *inv);
        }
    }
    
    // Transfer equipment if configured
    if (leaves_corpse->transfer_equipment) {
        EquipmentComponent* equip = manager->get_component<EquipmentComponent>(dead_entity_id);
        if (equip) {
            // Remove existing equipment and replace with dead entity's
            if (manager->get_component<EquipmentComponent>(corpse_id)) {
                manager->remove_component<EquipmentComponent>(corpse_id);
            }
            manager->add_component(corpse_id, *equip);
        }
    }
    
    // Update stats - corpses have minimal stats for possession
    StatsComponent* old_stats = manager->get_component<StatsComponent>(dead_entity_id);
    StatsComponent* corpse_stats = manager->get_component<StatsComponent>(corpse_id);
    if (corpse_stats) {
        corpse_stats->level = old_stats ? old_stats->level : 1;
        corpse_stats->max_hp = 1;  // Corpses are fragile
        corpse_stats->current_hp = 1;
        corpse_stats->attack = old_stats ? old_stats->attack / 2 : 1;  // Half attack
        corpse_stats->defense = 0;  // No defense
    } else {
        StatsComponent new_stats;
        new_stats.level = old_stats ? old_stats->level : 1;
        new_stats.max_hp = 1;
        new_stats.current_hp = 1;
        new_stats.attack = old_stats ? old_stats->attack / 2 : 1;
        new_stats.defense = 0;
        manager->add_component(corpse_id, new_stats);
    }
    
    return corpse_id;
}

void CombatSystem::handle_entity_death(EntityId entity_id) {
    // Try to spawn a corpse
    EntityId corpse_id = spawn_corpse(entity_id);
    
    if (corpse_id != 0) {
        // Clear the inventory/equipment from the original entity
        // so items don't get destroyed with it (they're now on the corpse)
        InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
        if (inv) {
            inv->items.clear();
        }
        EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
        if (equip) {
            // Clear limb-based equipment
            equip->equipped_items.clear();
            // Clear general slots
            equip->back = 0;
            equip->neck = 0;
            equip->waist = 0;
        }
    }
    
    // Now destroy the original entity
    manager->destroy_entity(entity_id);
}
