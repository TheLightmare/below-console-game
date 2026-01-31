#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/components/status_effects_component.hpp"
#include "../ecs/components/stats_component.hpp"
#include "../ecs/components/item_component.hpp"
#include "../loaders/effect_type_registry.hpp"
#include <functional>
#include <string>

// Status Effect System - processes all active status effects each turn
class StatusEffectSystem {
private:
    ComponentManager* manager;
    EntityId player_id = 0;
    std::function<void(const std::string&)> message_callback;
    std::function<void(const std::string&)> player_death_callback;  // Called when player dies with death reason
    
public:
    StatusEffectSystem(ComponentManager* mgr) : manager(mgr) {}
    
    void set_player_id(EntityId id) { player_id = id; }
    
    void set_message_callback(std::function<void(const std::string&)> callback) {
        message_callback = callback;
    }
    
    // Set callback for when player dies (receives death reason string)
    void set_player_death_callback(std::function<void(const std::string&)> callback) {
        player_death_callback = callback;
    }
    
    // Process all status effects for all entities - call this each turn
    void process_turn() {
        auto entities = manager->get_entities_with_component<StatusEffectsComponent>();
        
        for (EntityId entity_id : entities) {
            process_entity(entity_id);
        }
    }
    
    // Process status effects for a single entity
    void process_entity(EntityId entity_id) {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        if (!status || status->effects.empty()) return;
        
        StatsComponent* stats = manager->get_component<StatsComponent>(entity_id);
        NameComponent* name = manager->get_component<NameComponent>(entity_id);
        std::string entity_name = name ? name->name : "Entity";
        bool is_player = (entity_id == player_id);
        
        std::vector<std::string> messages;
        
        // Process each effect
        for (auto& effect : status->effects) {
            if (effect.is_expired()) continue;
            
            // Get effect definition from registry
            const EffectTypeDefinition* def = effect.get_definition();
            if (!def) {
                // Unknown effect, just tick and continue
                effect.tick();
                continue;
            }
            
            // Process based on tick behavior
            if (def->tick_behavior == "damage") {
                if (stats && !status->is_invulnerable()) {
                    stats->take_damage(effect.magnitude);
                    if (is_player) {
                        messages.push_back(def->name + " deals " + std::to_string(effect.magnitude) + " damage!");
                    } else {
                        messages.push_back(entity_name + " takes " + std::to_string(effect.magnitude) + " " + def->name + " damage!");
                    }
                    
                    // Check for death
                    if (stats->current_hp <= 0) {
                        if (is_player) {
                            messages.push_back("You succumb to " + def->name + "!");
                            // Report player death with reason
                            if (player_death_callback) {
                                player_death_callback("Succumbed to " + def->name);
                            }
                        } else {
                            messages.push_back(entity_name + " dies from " + def->name + "!");
                        }
                    }
                }
            } else if (def->tick_behavior == "heal") {
                if (stats && stats->current_hp < stats->max_hp) {
                    int healed = stats->heal(effect.magnitude);
                    if (healed > 0) {
                        if (is_player) {
                            messages.push_back(def->name + " heals " + std::to_string(healed) + " HP!");
                        } else {
                            messages.push_back(entity_name + " regenerates " + std::to_string(healed) + " HP!");
                        }
                    }
                }
            } else if (def->tick_behavior == "custom") {
                // Execute custom tick callback from registry
                auto& registry = EffectRegistry::instance();
                auto callback = registry.get_effect(effect.effect_type_id);
                if (callback) {
                    auto msg_cb = [&messages](const std::string& msg) {
                        messages.push_back(msg);
                    };
                    callback(entity_id, effect.source_id, manager, msg_cb);
                }
            }
            // "none" tick behavior just exists (buffs, control effects)
            
            // Tick down duration
            effect.tick();
        }
        
        // Send all messages
        if (message_callback) {
            for (const auto& msg : messages) {
                message_callback(msg);
            }
        }
        
        // Remove expired effects and notify
        std::vector<std::string> expired_ids;
        for (const auto& effect : status->effects) {
            if (effect.is_expired()) {
                expired_ids.push_back(effect.effect_type_id);
            }
        }
        
        status->remove_expired();
        
        // Notify about expired effects
        if (message_callback && is_player) {
            for (const std::string& effect_id : expired_ids) {
                std::string effect_name = get_status_effect_name(effect_id);
                message_callback("The " + effect_name + " effect wears off.");
            }
        }
    }
    
    // Apply a status effect to an entity using string ID
    // Instant effects (duration 0) are processed immediately and not stored
    void apply_effect(EntityId target_id, const std::string& effect_id, int magnitude, int duration, 
                      EntityId source = 0, bool stacks = false, const std::string& custom_message = "") {
        StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
        NameComponent* name = manager->get_component<NameComponent>(target_id);
        std::string target_name = name ? name->name : "Target";
        bool is_player = (target_id == player_id);
        
        // Get effect definition from registry
        const EffectTypeDefinition* def = EffectTypeRegistry::instance().get(effect_id);
        
        // Handle instant effects (duration 0 or marked as instant in registry)
        bool is_instant = (duration == 0) || (def && def->is_instant);
        
        if (is_instant && def) {
            if (effect_id == "heal") {
                if (stats) {
                    int healed = stats->heal(magnitude);
                    if (message_callback && healed > 0) {
                        std::string msg = custom_message.empty() ?
                            (is_player ? "Restored " + std::to_string(healed) + " HP!" :
                             target_name + " is healed for " + std::to_string(healed) + " HP!") :
                            custom_message;
                        message_callback(msg);
                    }
                }
                return;
            } else if (effect_id == "heal_percent") {
                if (stats) {
                    int amount = (stats->max_hp * magnitude) / 100;
                    int healed = stats->heal(amount);
                    if (message_callback && healed > 0) {
                        std::string msg = custom_message.empty() ?
                            (is_player ? "Restored " + std::to_string(healed) + " HP!" :
                             target_name + " is healed for " + std::to_string(healed) + " HP!") :
                            custom_message;
                        message_callback(msg);
                    }
                }
                return;
            } else if (effect_id == "damage") {
                if (stats) {
                    // Check for invulnerability
                    StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(target_id);
                    if (status && status->is_invulnerable()) {
                        if (message_callback) {
                            std::string msg = is_player ? "The attack has no effect!" :
                                target_name + " is unaffected!";
                            message_callback(msg);
                        }
                        return;
                    }
                    
                    stats->take_damage(magnitude);
                    if (message_callback) {
                        std::string msg = custom_message.empty() ?
                            (is_player ? "Took " + std::to_string(magnitude) + " damage!" :
                             target_name + " takes " + std::to_string(magnitude) + " damage!") :
                            custom_message;
                        message_callback(msg);
                    }
                    
                    // Check for player death
                    if (is_player && stats->current_hp <= 0 && player_death_callback) {
                        player_death_callback("Killed by a magical effect");
                    }
                }
                return;
            } else if (effect_id == "gain_xp") {
                if (stats) {
                    stats->gain_experience(magnitude);
                    if (message_callback) {
                        std::string msg = custom_message.empty() ?
                            (is_player ? "Gained " + std::to_string(magnitude) + " experience!" :
                             target_name + " gained " + std::to_string(magnitude) + " experience!") :
                            custom_message;
                        message_callback(msg);
                    }
                }
                return;
            }
            // Other instant effects fall through to be added as status (unusual case)
        }
        
        // Duration-based effects - add to status component
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            manager->add_component(target_id, StatusEffectsComponent());
            status = manager->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            status->add_effect(effect_id, magnitude, duration, source, stacks);
            
            // Notify
            if (message_callback) {
                std::string effect_name = def ? def->name : effect_id;
                std::string duration_str = (duration == -1) ? "" : " for " + std::to_string(duration) + " turns";
                
                std::string msg = custom_message.empty() ?
                    (is_player ? "You are " + effect_name + duration_str + "!" :
                     target_name + " is " + effect_name + duration_str + "!") :
                    custom_message;
                message_callback(msg);
            }
        }
    }
    
    // Legacy overload using enum
    void apply_effect(EntityId target_id, StatusEffectType type, int magnitude, int duration, 
                      EntityId source = 0, bool stacks = false, const std::string& custom_message = "") {
        apply_effect(target_id, effect_type_to_string(type), magnitude, duration, source, stacks, custom_message);
    }
    
    // Cure a specific effect by ID
    void cure_effect(EntityId target_id, const std::string& effect_id) {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(target_id);
        if (status) {
            bool had_effect = status->has_effect(effect_id);
            status->remove_effects(effect_id);
            
            if (had_effect && message_callback) {
                NameComponent* name = manager->get_component<NameComponent>(target_id);
                std::string target_name = name ? name->name : "Target";
                bool is_player = (target_id == player_id);
                
                std::string effect_name = get_status_effect_name(effect_id);
                if (is_player) {
                    message_callback("The " + effect_name + " effect is cured!");
                } else {
                    message_callback(target_name + "'s " + effect_name + " is cured!");
                }
            }
        }
    }
    
    // Legacy overload using enum
    void cure_effect(EntityId target_id, StatusEffectType type) {
        cure_effect(target_id, effect_type_to_string(type));
    }
    
    // Cure all effects on an entity
    void cure_all(EntityId target_id) {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(target_id);
        if (status && !status->effects.empty()) {
            status->clear_all_effects();
            
            if (message_callback) {
                bool is_player = (target_id == player_id);
                if (is_player) {
                    message_callback("All status effects are purged!");
                }
            }
        }
    }
    
    // Check if entity has a specific effect by ID
    bool has_effect(EntityId entity_id, const std::string& effect_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status && status->has_effect(effect_id);
    }
    
    // Legacy overload by enum
    bool has_effect(EntityId entity_id, StatusEffectType type) const {
        return has_effect(entity_id, effect_type_to_string(type));
    }
    
    // Check if entity can act (uses registry-based check)
    bool can_act(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        if (!status) return true;
        return status->can_act();
    }
    
    // Check if entity can move (uses registry-based check)
    bool can_move(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        if (!status) return true;
        return status->can_move();
    }
    
    // Get stat modifiers from status effects
    int get_attack_modifier(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status ? status->attack_modifier : 0;
    }
    
    int get_defense_modifier(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status ? status->defense_modifier : 0;
    }
    
    int get_speed_modifier(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status ? status->speed_modifier : 0;
    }
    
    // Check if entity is invisible
    bool is_invisible(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status && status->is_invisible();
    }
    
    // Check if entity is invulnerable
    bool is_invulnerable(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status && status->is_invulnerable();
    }
    
    // Get entity that target is afraid of (for fleeing AI)
    EntityId get_fear_source(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status ? status->get_fear_source() : 0;
    }
    
    // Get entity that target is charmed by (for ally AI)
    EntityId get_charm_source(EntityId entity_id) const {
        StatusEffectsComponent* status = manager->get_component<StatusEffectsComponent>(entity_id);
        return status ? status->get_charm_source() : 0;
    }
};
