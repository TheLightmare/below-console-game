#pragma once
#include "../../engine/ecs/component.hpp"
#include "../../engine/ecs/component_manager.hpp"
#include "../../engine/ecs/entity.hpp"
#include "../../engine/ecs/components/status_effects_component.hpp"
#include "../../engine/loaders/effect_type_registry.hpp"
#include "../../engine/loaders/item_effect_type_registry.hpp"
#include "../../engine/systems/utility_ai.hpp"
#include "../../engine/util/rng.hpp"

// Forward declaration for EntityFactory
class EntityFactory;

// Initialize all registered effects
// This must be called once at game startup (before loading any saves)
// The factory pointer is needed for effects that use factory methods (like cloning)
inline void init_effect_registry(EntityFactory* factory = nullptr) {
    (void)factory; // Reserved for future use (e.g., effects that create entities)
    
    // Load effect type definitions from JSON
    if (!load_effect_types("assets/effects/effects.json")) {
        // Fallback: effect types will use default values if JSON loading fails
        // This allows the game to still run but with reduced functionality
    }
    
    // Load item effect type definitions from JSON
    if (!load_item_effect_types("assets/effects/item_effects.json")) {
        // Fallback: item effect types will use default behavior if JSON loading fails
    }
    
    auto& registry = EffectRegistry::instance();
    
    // ==================== Mutagen Effect ====================
    // Adds a random mutant limb to the target's anatomy
    registry.register_effect("effect.mutagen", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                   std::function<void(const std::string&)> msg_cb) -> bool {
        AnatomyComponent* anatomy = mgr->get_component<AnatomyComponent>(target_id);
        if (!anatomy) {
            if (msg_cb) msg_cb("The mutagen has no effect...");
            return false;
        }
        
        // Random limb types and names for mutation
        static const std::vector<std::pair<LimbType, std::string>> mutations = {
            {LimbType::ARM, "mutant arm"},
            {LimbType::HEAD, "extra head"},
            {LimbType::HAND, "clawed hand"},
            {LimbType::HAND, "tentacle"},
            {LimbType::OTHER, "tail"},
            {LimbType::OTHER, "wing"},
            {LimbType::LEG, "extra leg"},
            {LimbType::HAND, "prehensile tongue"}
        };
        
        int idx = random_index(mutations.size());
        const auto& [limb_type, limb_name] = mutations[idx];
        anatomy->add_limb(limb_type, limb_name, 0.3f);
        
        if (msg_cb) {
            msg_cb("%target%'s body writhes in agony as a " + limb_name + " bursts forth!");
        }
        return true;
    });
    
    // ==================== Cloning Potion Effect ====================
    // Creates a clone of the target entity at an adjacent position
    registry.register_effect("effect.cloning_potion", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                          std::function<void(const std::string&)> msg_cb) -> bool {
        // Get target position
        PositionComponent* pos = mgr->get_component<PositionComponent>(target_id);
        if (!pos) {
            if (msg_cb) msg_cb("Cannot clone entity without position.");
            return false;
        }
        
        // Get the original name before cloning
        NameComponent* original_name = mgr->get_component<NameComponent>(target_id);
        std::string name_for_message = original_name ? original_name->name : "the target";
        
        // Create clone entity
        EntityId clone_id = mgr->create_entity();
        
        // Dynamically copy ALL components from target to clone
        // This uses the virtual clone() method on each component, so any new
        // component types are automatically supported without code changes here
        mgr->copy_all_components(target_id, clone_id);
        
        // Override position to offset the clone by 1 tile
        PositionComponent* clone_pos = mgr->get_component<PositionComponent>(clone_id);
        if (clone_pos) {
            clone_pos->x = pos->x + 1;
        }
        
        // Update the clone's name to indicate it's a clone
        NameComponent* clone_name = mgr->get_component<NameComponent>(clone_id);
        if (clone_name) {
            clone_name->name = clone_name->name + " (Clone)";
        } else {
            mgr->add_component<NameComponent>(clone_id, "Clone", "");
        }
        
        if (msg_cb) {
            msg_cb("A perfect clone of " + name_for_message + " appears!");
        }
        return true;
    });

    // ==================== Body Swap Effect ====================
    // Swaps the anatomy of the user with the target - must be thrown at a target!
    registry.register_effect("effect.body_swap", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                          std::function<void(const std::string&)> msg_cb) -> bool {
        
        // Body swap requires a different target (must throw the potion)
        if (user_id == 0 || user_id == target_id) {
            if (msg_cb) msg_cb("The potion splashes harmlessly... You need to throw it at someone!");
            return false;
        }
        
        AnatomyComponent* target_anatomy = mgr->get_component<AnatomyComponent>(target_id);
        if (!target_anatomy) {
            if (msg_cb) msg_cb("The target has no body to swap with!");
            return false;
        }
        AnatomyComponent* user_anatomy = mgr->get_component<AnatomyComponent>(user_id);
        if (!user_anatomy) {
            if (msg_cb) msg_cb("You have no body to swap!");
            return false;
        }

        // Swap the anatomy components
        std::swap(*target_anatomy, *user_anatomy);

        // Swap render components to reflect new appearances
        RenderComponent* target_render = mgr->get_component<RenderComponent>(target_id);
        RenderComponent* user_render = mgr->get_component<RenderComponent>(user_id);
        if (target_render && user_render) {
            std::swap(*target_render, *user_render);
        }

        NameComponent* target_name = mgr->get_component<NameComponent>(target_id);
        std::string name_for_message = target_name ? target_name->name : "the target";

        NameComponent* user_name = mgr->get_component<NameComponent>(user_id);
        std::string user_name_for_message = user_name ? user_name->name : "the user";

        if(msg_cb) {
            msg_cb(user_name_for_message + " and " + name_for_message + " have swapped bodies!");
        }

        return true;
    });


    // ==================== Soul Swap Effect ====================
    // Change control of target and user entities, including faction allegiance
    registry.register_effect("effect.soul_swap", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                   std::function<void(const std::string&)> msg_cb) -> bool {

        if (user_id == 0 || user_id == target_id) {
            if (msg_cb) msg_cb("The soul swap fails... You need to yeet it at someone!");
            return false;
        }

        // determine if any of the entities is the player
        bool user_is_player = false;
        if(mgr->get_component<PlayerControllerComponent>(user_id)) {
            user_is_player = true;
        }

        bool target_is_player = false;
        if(mgr->get_component<PlayerControllerComponent>(target_id)) {
            target_is_player = true;
        }

        if(!user_is_player && !target_is_player) {
            if (msg_cb) msg_cb("The soul swap fails... Neither target is the player!");
            return false;
        }



        if(user_is_player) {
            // Copy the target's AI before removing it (so we can give it to the old player body)
            UtilityAIComponent* target_ai = mgr->get_component<UtilityAIComponent>(target_id);
            UtilityAIComponent ai_copy;
            if (target_ai) {
                ai_copy = *target_ai;  // Copy the AI configuration
            } else {
                // If target has no AI, set up a basic aggressive AI
                UtilityPresets::setup_aggressive_melee(ai_copy);
            }
            
            // give player control to target
            mgr->remove_component<PlayerControllerComponent>(user_id);
            mgr->add_component<PlayerControllerComponent>(target_id, true, true);
            
            // remove AI control from target, give it to the old player body
            mgr->remove_component<UtilityAIComponent>(target_id);
            auto& new_ai = mgr->add_component<UtilityAIComponent>(user_id);
            new_ai = ai_copy;  // Apply the copied AI behaviors
            
        } else if(target_is_player) {
            // Copy the user's AI before removing it
            UtilityAIComponent* user_ai = mgr->get_component<UtilityAIComponent>(user_id);
            UtilityAIComponent ai_copy;
            if (user_ai) {
                ai_copy = *user_ai;
            } else {
                UtilityPresets::setup_aggressive_melee(ai_copy);
            }
            
            // give player control to user
            mgr->remove_component<PlayerControllerComponent>(target_id);
            mgr->add_component<PlayerControllerComponent>(user_id, true, true);
            
            // remove AI control from user, give it to the old player body
            mgr->remove_component<UtilityAIComponent>(user_id);
            auto& new_ai = mgr->add_component<UtilityAIComponent>(target_id);
            new_ai = ai_copy;
        }

        NameComponent* target_name = mgr->get_component<NameComponent>(target_id);
        std::string name_for_message = target_name ? target_name->name : "the target";

        NameComponent* user_name = mgr->get_component<NameComponent>(user_id);
        std::string user_name_for_message = user_name ? user_name->name : "the user";

        if(msg_cb) {
            msg_cb(user_name_for_message + " and " + name_for_message + " have swapped souls!");
        }


        return true;
        
    });
    
    // ==================== Berserk Potion Effect ====================
    // Doubles attack but halves defense for a duration (uses status effect system)
    registry.register_effect("effect.berserk", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                   std::function<void(const std::string&)> msg_cb) -> bool {
        StatsComponent* stats = mgr->get_component<StatsComponent>(target_id);
        if (!stats) {
            if (msg_cb) msg_cb("The potion has no effect.");
            return false;
        }
        
        // Apply temporary berserk status using status effects
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // +5 attack, -2 defense for 15 turns
            status->add_effect(StatusEffectType::BUFF_ATTACK, 5, 15, user_id);
            status->add_effect(StatusEffectType::DEBUFF_DEFENSE, 2, 15, user_id);
            status->add_effect(StatusEffectType::BERSERK, 1, 15, user_id);
        }
        
        if (msg_cb) msg_cb("%target% feels a berserker rage surging through!");
        return true;
    });
    
    // ==================== Experience Potion Effect ====================
    // Grants bonus experience
    registry.register_effect("effect.experience_boost", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                            std::function<void(const std::string&)> msg_cb) -> bool {
        StatsComponent* stats = mgr->get_component<StatsComponent>(target_id);
        if (!stats) {
            if (msg_cb) msg_cb("The potion has no effect.");
            return false;
        }
        
        int xp_gain = 50 + (stats->level * 10);
        stats->gain_experience(xp_gain);
        
        if (msg_cb) msg_cb("Ancient knowledge floods %target%'s mind! Gained " + std::to_string(xp_gain) + " XP!");
        return true;
    });
    
    // ==================== Full Heal Effect ====================
    // Fully restores HP
    registry.register_effect("effect.full_heal", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                     std::function<void(const std::string&)> msg_cb) -> bool {
        StatsComponent* stats = mgr->get_component<StatsComponent>(target_id);
        if (!stats) {
            if (msg_cb) msg_cb("The potion has no effect.");
            return false;
        }
        
        stats->current_hp = stats->max_hp;
        
        if (msg_cb) msg_cb("%target% feels completely restored!");
        return true;
    });
    
    // ==================== Swiftness Effect ====================
    // Increases movement speed temporarily using status effect system
    registry.register_effect("effect.swiftness", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                     std::function<void(const std::string&)> msg_cb) -> bool {
        // Apply temporary speed buff using status effects
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // Speed buff for 20 turns (also gives small attack bonus for faster strikes)
            status->add_effect(StatusEffectType::BUFF_SPEED, 2, 20, user_id);
            status->add_effect(StatusEffectType::BUFF_ATTACK, 2, 20, user_id);
        }
        
        if (msg_cb) msg_cb("%target% feels light as a feather! Reflexes sharpen.");
        return true;
    });
    
    // ==================== Cure Poison Effect ====================
    // Removes poison effects from target
    registry.register_effect("effect.cure_poison", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                       std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (status && status->has_effect(StatusEffectType::POISON)) {
            status->remove_effects(StatusEffectType::POISON);
            if (msg_cb) msg_cb("The poison is purged from %target%'s body!");
        } else {
            // No poison to cure, just heal a small amount
            StatsComponent* stats = mgr->get_component<StatsComponent>(target_id);
            if (stats) {
                stats->heal(10);
            }
            if (msg_cb) msg_cb("%target% feels refreshed!");
        }
        return true;
    });
    
    // ==================== Night Vision Effect ====================
    // Increases vision range temporarily
    registry.register_effect("effect.night_vision", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                        std::function<void(const std::string&)> msg_cb) -> bool {
        // TODO: Implement vision range buff when FOV system supports it
        // For now, just display message
        (void)target_id;
        (void)mgr;
        
        if (msg_cb) msg_cb("%target%'s eyes adjust to the darkness!");
        return true;
    });
    
    // ==================== Weapon Poison Effect ====================
    // Temporarily buffs attack to simulate poison-coated weapon
    registry.register_effect("effect.weapon_poison", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                         std::function<void(const std::string&)> msg_cb) -> bool {
        // Apply temporary attack buff using status effects
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // +3 attack for 10 turns (simulates poison damage on weapons)
            status->add_effect(StatusEffectType::BUFF_ATTACK, 3, 10, user_id);
        }
        
        if (msg_cb) msg_cb("%target%'s weapon drips with deadly venom!");
        return true;
    });
    
    // ==================== Curse Effect ====================
    // Debuffs enemy when thrown
    registry.register_effect("effect.curse", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                 std::function<void(const std::string&)> msg_cb) -> bool {
        StatsComponent* stats = mgr->get_component<StatsComponent>(target_id);
        if (!stats) {
            if (msg_cb) msg_cb("The curse dissipates harmlessly.");
            return false;
        }
        
        // Reduce target's attack and defense
        stats->attack = std::max(1, stats->attack - 3);
        stats->defense = std::max(0, stats->defense - 3);
        
        if (msg_cb) msg_cb("A dark curse weakens the target!");
        return true;
    });
    
    // ==================== Random Buff Effect (Witch's Brew) ====================
    // Grants a random beneficial effect
    registry.register_effect("effect.random_buff", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                       std::function<void(const std::string&)> msg_cb) -> bool {
        StatsComponent* stats = mgr->get_component<StatsComponent>(target_id);
        if (!stats) {
            if (msg_cb) msg_cb("The strange brew has no effect.");
            return false;
        }
        
        int effect_type = random_int(0, 5);
        switch (effect_type) {
            case 0: // Major heal
                stats->heal(stats->max_hp / 2);
                if (msg_cb) msg_cb("Healing energy surges through %target%!");
                break;
            case 1: // Attack boost
                stats->attack += 5;
                if (msg_cb) msg_cb("Raw power floods %target%'s muscles!");
                break;
            case 2: // Defense boost
                stats->defense += 5;
                if (msg_cb) msg_cb("%target%'s skin toughens like dragon scales!");
                break;
            case 3: // Full heal
                stats->current_hp = stats->max_hp;
                if (msg_cb) msg_cb("Complete restoration washes over %target%!");
                break;
            case 4: // XP boost
                stats->gain_experience(100);
                if (msg_cb) msg_cb("Ancient wisdom floods %target%'s mind!");
                break;
            case 5: // All stats boost
                stats->attack += 2;
                stats->defense += 2;
                stats->heal(25);
                if (msg_cb) msg_cb("%target% feels empowered in every way!");
                break;
        }
        return true;
    });
    
    // ==================== Poison Effect ====================
    // Applies poison status effect - damage over time
    registry.register_effect("effect.poison", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                  std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // 3 damage per turn for 5 turns
            status->add_effect(StatusEffectType::POISON, 3, 5, user_id);
            if (msg_cb) msg_cb("%target% is poisoned!");
        }
        return true;
    });
    
    // ==================== Strong Poison Effect ====================
    // Applies stronger poison - thrown weapons, traps
    registry.register_effect("effect.strong_poison", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                         std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // 5 damage per turn for 8 turns
            status->add_effect(StatusEffectType::POISON, 5, 8, user_id);
            if (msg_cb) msg_cb("%target% is badly poisoned!");
        }
        return true;
    });
    
    // ==================== Regeneration Effect ====================
    // Applies regeneration status effect - heal over time
    registry.register_effect("effect.regeneration", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                        std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // Heal 5 HP per turn for 10 turns
            status->add_effect(StatusEffectType::REGENERATION, 5, 10, user_id);
            if (msg_cb) msg_cb("%target%'s wounds begin to close!");
        }
        return true;
    });
    
    // ==================== Burning Effect ====================
    // Applies burning status effect - fire damage over time
    registry.register_effect("effect.burning", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                   std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // 4 damage per turn for 3 turns
            status->add_effect(StatusEffectType::BURN, 4, 3, user_id);
            if (msg_cb) msg_cb("%target% is set ablaze!");
        }
        return true;
    });
    
    // ==================== Bleeding Effect ====================
    // Applies bleed status effect - damage over time from wounds
    registry.register_effect("effect.bleeding", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                    std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // 2 damage per turn for 6 turns
            status->add_effect(StatusEffectType::BLEED, 2, 6, user_id);
            if (msg_cb) msg_cb("%target% starts bleeding!");
        }
        return true;
    });
    
    // ==================== Stun Effect ====================
    // Applies stun - target cannot act for a few turns
    registry.register_effect("effect.stun", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // Stunned for 2 turns
            status->add_effect(StatusEffectType::STUN, 1, 2, user_id);
            if (msg_cb) msg_cb("%target% is stunned!");
        }
        return true;
    });
    
    // ==================== Temporary Attack Buff ====================
    // Temporary attack boost using status effect system
    registry.register_effect("effect.buff_attack", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                       std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // +5 attack for 10 turns
            status->add_effect(StatusEffectType::BUFF_ATTACK, 5, 10, user_id);
            if (msg_cb) msg_cb("%target% feels a surge of power!");
        }
        return true;
    });
    
    // ==================== Temporary Defense Buff ====================
    // Temporary defense boost using status effect system
    registry.register_effect("effect.buff_defense", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                        std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // +5 defense for 10 turns
            status->add_effect(StatusEffectType::BUFF_DEFENSE, 5, 10, user_id);
            if (msg_cb) msg_cb("%target%'s skin hardens!");
        }
        return true;
    });
    
    // ==================== Invisibility Effect ====================
    // Makes target invisible temporarily
    registry.register_effect("effect.invisibility", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                        std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // Invisible for 15 turns
            status->add_effect(StatusEffectType::INVISIBILITY, 1, 15, user_id);
            if (msg_cb) msg_cb("%target% fades from view!");
        }
        return true;
    });
    
    // ==================== Invulnerability Effect ====================
    // Makes target immune to damage temporarily
    registry.register_effect("effect.invulnerability", [](EntityId target_id, EntityId user_id, ComponentManager* mgr, 
                                                           std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (!status) {
            mgr->add_component(target_id, StatusEffectsComponent());
            status = mgr->get_component<StatusEffectsComponent>(target_id);
        }
        
        if (status) {
            // Invulnerable for 5 turns
            status->add_effect(StatusEffectType::INVULNERABLE, 1, 5, user_id);
            if (msg_cb) msg_cb("%target% is surrounded by a protective aura!");
        }
        return true;
    });
    
    // ==================== Cure All Effect ====================
    // Removes all negative status effects
    registry.register_effect("effect.cure_all", [](EntityId target_id, EntityId /*user_id*/, ComponentManager* mgr, 
                                                    std::function<void(const std::string&)> msg_cb) -> bool {
        StatusEffectsComponent* status = mgr->get_component<StatusEffectsComponent>(target_id);
        if (status) {
            // Remove negative effects
            status->remove_effects(StatusEffectType::POISON);
            status->remove_effects(StatusEffectType::BURN);
            status->remove_effects(StatusEffectType::BLEED);
            status->remove_effects(StatusEffectType::STUN);
            status->remove_effects(StatusEffectType::PARALYSIS);
            status->remove_effects(StatusEffectType::BLIND);
            status->remove_effects(StatusEffectType::CONFUSION);
            status->remove_effects(StatusEffectType::FEAR);
            status->remove_effects(StatusEffectType::DEBUFF_ATTACK);
            status->remove_effects(StatusEffectType::DEBUFF_DEFENSE);
            status->remove_effects(StatusEffectType::DEBUFF_SPEED);
            if (msg_cb) msg_cb("All ailments are purged from %target%!");
        } else {
            if (msg_cb) msg_cb("%target% feels cleansed!");
        }
        return true;
    });
}
