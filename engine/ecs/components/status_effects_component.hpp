#pragma once
#include "component_base.hpp"
#include "../types.hpp"
#include "../../loaders/effect_type_registry.hpp"
#include <vector>
#include <string>
#include <algorithm>

// ==================== STATUS EFFECT SYSTEM ====================
// Tracks ongoing effects on entities (poison, regeneration, buffs, debuffs, etc.)
// Instant effects (duration 0) are processed immediately and not stored.
// Effect types are now loaded from JSON (assets/effects/effects.json)

// Legacy enum for backward compatibility during transition
// New code should use string-based effect_type_id instead
enum class StatusEffectType {
    // ========== INSTANT EFFECTS (duration 0) ==========
    HEAL,               // Heal HP instantly
    HEAL_PERCENT,       // Heal % of max HP instantly
    DAMAGE,             // Deal damage instantly
    GAIN_XP,            // Grant experience points
    
    // ========== DAMAGE OVER TIME ==========
    POISON,             // Deals damage each turn
    BURN,               // Fire damage each turn
    BLEED,              // Bleed damage each turn
    
    // ========== HEALING OVER TIME ==========
    REGENERATION,       // Heals HP each turn
    
    // ========== STAT MODIFIERS (temporary) ==========
    BUFF_ATTACK,        // Increased attack
    BUFF_DEFENSE,       // Increased defense
    BUFF_SPEED,         // Increased movement speed
    DEBUFF_ATTACK,      // Decreased attack
    DEBUFF_DEFENSE,     // Decreased defense
    DEBUFF_SPEED,       // Decreased movement speed (slowed)
    
    // ========== CONTROL EFFECTS ==========
    STUN,               // Cannot act
    PARALYSIS,          // Cannot move (can still attack adjacent)
    BLIND,              // Reduced vision range
    CONFUSION,          // Random movement direction
    FEAR,               // Flees from source
    CHARM,              // Treats source as ally
    
    // ========== MISC ==========
    INVISIBILITY,       // Cannot be seen by enemies
    INVULNERABLE,       // Cannot take damage
    BERSERK,            // Increased damage but attacks anyone
    
    // ========== CUSTOM ==========
    CUSTOM              // Uses effect_id for custom tick behavior
};

// Convert StatusEffectType enum to string ID (for registry lookup)
inline std::string effect_type_to_string(StatusEffectType type) {
    switch (type) {
        case StatusEffectType::HEAL: return "heal";
        case StatusEffectType::HEAL_PERCENT: return "heal_percent";
        case StatusEffectType::DAMAGE: return "damage";
        case StatusEffectType::GAIN_XP: return "gain_xp";
        case StatusEffectType::POISON: return "poison";
        case StatusEffectType::BURN: return "burn";
        case StatusEffectType::BLEED: return "bleed";
        case StatusEffectType::REGENERATION: return "regeneration";
        case StatusEffectType::BUFF_ATTACK: return "buff_attack";
        case StatusEffectType::BUFF_DEFENSE: return "buff_defense";
        case StatusEffectType::BUFF_SPEED: return "buff_speed";
        case StatusEffectType::DEBUFF_ATTACK: return "debuff_attack";
        case StatusEffectType::DEBUFF_DEFENSE: return "debuff_defense";
        case StatusEffectType::DEBUFF_SPEED: return "debuff_speed";
        case StatusEffectType::STUN: return "stun";
        case StatusEffectType::PARALYSIS: return "paralysis";
        case StatusEffectType::BLIND: return "blind";
        case StatusEffectType::CONFUSION: return "confusion";
        case StatusEffectType::FEAR: return "fear";
        case StatusEffectType::CHARM: return "charm";
        case StatusEffectType::INVISIBILITY: return "invisibility";
        case StatusEffectType::INVULNERABLE: return "invulnerable";
        case StatusEffectType::BERSERK: return "berserk";
        case StatusEffectType::CUSTOM: return "custom";
        default: return "unknown";
    }
}

// Convert string ID to StatusEffectType enum (for backward compatibility)
inline StatusEffectType string_to_effect_type(const std::string& id) {
    if (id == "heal") return StatusEffectType::HEAL;
    if (id == "heal_percent") return StatusEffectType::HEAL_PERCENT;
    if (id == "damage") return StatusEffectType::DAMAGE;
    if (id == "gain_xp") return StatusEffectType::GAIN_XP;
    if (id == "poison") return StatusEffectType::POISON;
    if (id == "burn") return StatusEffectType::BURN;
    if (id == "bleed") return StatusEffectType::BLEED;
    if (id == "regeneration") return StatusEffectType::REGENERATION;
    if (id == "buff_attack") return StatusEffectType::BUFF_ATTACK;
    if (id == "buff_defense") return StatusEffectType::BUFF_DEFENSE;
    if (id == "buff_speed") return StatusEffectType::BUFF_SPEED;
    if (id == "debuff_attack") return StatusEffectType::DEBUFF_ATTACK;
    if (id == "debuff_defense") return StatusEffectType::DEBUFF_DEFENSE;
    if (id == "debuff_speed") return StatusEffectType::DEBUFF_SPEED;
    if (id == "stun") return StatusEffectType::STUN;
    if (id == "paralysis") return StatusEffectType::PARALYSIS;
    if (id == "blind") return StatusEffectType::BLIND;
    if (id == "confusion") return StatusEffectType::CONFUSION;
    if (id == "fear") return StatusEffectType::FEAR;
    if (id == "charm") return StatusEffectType::CHARM;
    if (id == "invisibility") return StatusEffectType::INVISIBILITY;
    if (id == "invulnerable") return StatusEffectType::INVULNERABLE;
    if (id == "berserk") return StatusEffectType::BERSERK;
    return StatusEffectType::CUSTOM;  // Default to custom for unknown IDs
}

// Check if an effect type is instant (uses registry)
inline bool is_instant_effect(const std::string& effect_id) {
    return EffectTypeRegistry::instance().is_instant(effect_id);
}

// Legacy overload for backward compatibility
inline bool is_instant_effect(StatusEffectType type) {
    return is_instant_effect(effect_type_to_string(type));
}

// Get display name for a status effect (uses registry)
inline std::string get_status_effect_name(const std::string& effect_id) {
    return EffectTypeRegistry::instance().get_name(effect_id);
}

// Legacy overload for backward compatibility
inline std::string get_status_effect_name(StatusEffectType type) {
    return get_status_effect_name(effect_type_to_string(type));
}

// Get display color for a status effect (uses registry, returns hex)
inline int get_status_effect_color(const std::string& effect_id) {
    return EffectTypeRegistry::instance().get_color_hex(effect_id);
}

// Legacy overload for backward compatibility
inline int get_status_effect_color(StatusEffectType type) {
    return get_status_effect_color(effect_type_to_string(type));
}

// Individual status effect instance
struct StatusEffect {
    std::string effect_type_id = "poison";  // Primary identifier - references effects.json
    StatusEffectType type = StatusEffectType::POISON;  // Legacy - kept for backward compatibility
    int magnitude = 1;              // Effect strength (damage/heal amount, stat modifier, etc.)
    int turns_remaining = 0;        // Turns left (-1 = permanent until cured)
    int turns_total = 0;            // Original duration (for display)
    EntityId source_id = 0;         // Who applied this effect (for fear, charm, retaliation)
    bool stacks = false;            // If true, multiple of same type can stack; if false, refreshes duration
    
    StatusEffect() = default;
    
    // Primary constructor using string ID
    StatusEffect(const std::string& id, int mag, int duration, EntityId source = 0, bool can_stack = false)
        : effect_type_id(id), type(string_to_effect_type(id)), magnitude(mag), 
          turns_remaining(duration), turns_total(duration), source_id(source), stacks(can_stack) {}
    
    // Legacy constructor using enum (converts to string ID)
    StatusEffect(StatusEffectType t, int mag, int duration, EntityId source = 0, bool can_stack = false)
        : effect_type_id(effect_type_to_string(t)), type(t), magnitude(mag), 
          turns_remaining(duration), turns_total(duration), source_id(source), stacks(can_stack) {}
    
    bool is_expired() const { return turns_remaining == 0; }
    bool is_permanent() const { return turns_remaining == -1; }
    
    // Get definition from registry
    const EffectTypeDefinition* get_definition() const {
        return EffectTypeRegistry::instance().get(effect_type_id);
    }
    
    // Get display name from registry
    std::string get_name() const {
        return EffectTypeRegistry::instance().get_name(effect_type_id);
    }
    
    // Get display color from registry
    int get_color() const {
        return EffectTypeRegistry::instance().get_color_hex(effect_type_id);
    }
    
    // Check if this is an instant effect
    bool is_instant() const {
        return EffectTypeRegistry::instance().is_instant(effect_type_id);
    }
    
    // Check if this is a negative effect
    bool is_negative() const {
        return EffectTypeRegistry::instance().is_negative(effect_type_id);
    }
    
    // Tick the effect (called each turn) - returns true if effect should continue
    bool tick() {
        if (turns_remaining > 0) {
            turns_remaining--;
        }
        return !is_expired();
    }
};

// Component that tracks all active status effects on an entity
struct StatusEffectsComponent : Component {
    std::vector<StatusEffect> effects;
    
    // Cached stat modifiers (updated when effects change)
    int attack_modifier = 0;
    int defense_modifier = 0;
    int speed_modifier = 0;
    
    StatusEffectsComponent() = default;
    
    // Add a new status effect
    void add_effect(const StatusEffect& effect) {
        // Check if we already have this effect type (compare by effect_type_id)
        if (!effect.stacks) {
            for (auto& existing : effects) {
                if (existing.effect_type_id == effect.effect_type_id) {
                    // Refresh duration if new effect is longer, keep higher magnitude
                    if (effect.turns_remaining > existing.turns_remaining || effect.turns_remaining == -1) {
                        existing.turns_remaining = effect.turns_remaining;
                        existing.turns_total = effect.turns_total;
                    }
                    if (effect.magnitude > existing.magnitude) {
                        existing.magnitude = effect.magnitude;
                    }
                    existing.source_id = effect.source_id;  // Update source
                    recalculate_modifiers();
                    return;
                }
            }
        }
        
        // Add new effect
        effects.push_back(effect);
        recalculate_modifiers();
    }
    
    // Add effect with string ID
    void add_effect(const std::string& effect_id, int magnitude, int duration, EntityId source = 0, bool stacks = false) {
        add_effect(StatusEffect(effect_id, magnitude, duration, source, stacks));
    }
    
    // Add effect with simple parameters (legacy - uses enum)
    void add_effect(StatusEffectType type, int magnitude, int duration, EntityId source = 0, bool stacks = false) {
        add_effect(StatusEffect(type, magnitude, duration, source, stacks));
    }
    
    // Remove all effects of a specific type (by string ID)
    void remove_effects(const std::string& effect_id) {
        effects.erase(std::remove_if(effects.begin(), effects.end(),
            [&effect_id](const StatusEffect& e) { return e.effect_type_id == effect_id; }), effects.end());
        recalculate_modifiers();
    }
    
    // Remove all effects of a specific type (legacy - by enum)
    void remove_effects(StatusEffectType type) {
        std::string effect_id = effect_type_to_string(type);
        effects.erase(std::remove_if(effects.begin(), effects.end(),
            [&effect_id](const StatusEffect& e) { return e.effect_type_id == effect_id; }), effects.end());
        recalculate_modifiers();
    }
    
    // Remove all effects from a specific source
    void remove_effects_from(EntityId source) {
        effects.erase(std::remove_if(effects.begin(), effects.end(),
            [source](const StatusEffect& e) { return e.source_id == source; }), effects.end());
        recalculate_modifiers();
    }
    
    // Remove all effects (cure all)
    void clear_all_effects() {
        effects.clear();
        attack_modifier = 0;
        defense_modifier = 0;
        speed_modifier = 0;
    }
    
    // Remove expired effects
    void remove_expired() {
        effects.erase(std::remove_if(effects.begin(), effects.end(),
            [](const StatusEffect& e) { return e.is_expired(); }), effects.end());
        recalculate_modifiers();
    }
    
    // Check if entity has a specific effect by ID
    bool has_effect(const std::string& effect_id) const {
        for (const auto& effect : effects) {
            if (effect.effect_type_id == effect_id && !effect.is_expired()) return true;
        }
        return false;
    }
    
    // Check if entity has a specific effect type (legacy - by enum)
    bool has_effect(StatusEffectType type) const {
        std::string effect_id = effect_type_to_string(type);
        return has_effect(effect_id);
    }
    
    // Get total magnitude of a specific effect type (for stacking effects)
    int get_effect_magnitude(const std::string& effect_id) const {
        int total = 0;
        for (const auto& effect : effects) {
            if (effect.effect_type_id == effect_id && !effect.is_expired()) {
                total += effect.magnitude;
            }
        }
        return total;
    }
    
    // Legacy overload by enum
    int get_effect_magnitude(StatusEffectType type) const {
        return get_effect_magnitude(effect_type_to_string(type));
    }
    
    // Get the strongest effect of a type (for non-stacking display)
    const StatusEffect* get_strongest_effect(const std::string& effect_id) const {
        const StatusEffect* strongest = nullptr;
        for (const auto& effect : effects) {
            if (effect.effect_type_id == effect_id && !effect.is_expired()) {
                if (!strongest || effect.magnitude > strongest->magnitude) {
                    strongest = &effect;
                }
            }
        }
        return strongest;
    }
    
    // Legacy overload by enum
    const StatusEffect* get_strongest_effect(StatusEffectType type) const {
        return get_strongest_effect(effect_type_to_string(type));
    }
    
    // Recalculate cached stat modifiers from all active effects
    void recalculate_modifiers() {
        attack_modifier = 0;
        defense_modifier = 0;
        speed_modifier = 0;
        
        for (const auto& effect : effects) {
            if (effect.is_expired()) continue;
            
            // Use registry to get modifier info
            const auto* def = effect.get_definition();
            if (def) {
                if (def->stat == "attack") {
                    if (def->modifier_type == "add") {
                        attack_modifier += effect.magnitude;
                    } else if (def->modifier_type == "subtract") {
                        attack_modifier -= effect.magnitude;
                    }
                } else if (def->stat == "defense") {
                    if (def->modifier_type == "add") {
                        defense_modifier += effect.magnitude;
                    } else if (def->modifier_type == "subtract") {
                        defense_modifier -= effect.magnitude;
                    }
                } else if (def->stat == "speed") {
                    if (def->modifier_type == "add") {
                        speed_modifier += effect.magnitude;
                    } else if (def->modifier_type == "subtract") {
                        speed_modifier -= effect.magnitude;
                    }
                }
            }
        }
    }
    
    // Check control effects using string IDs
    bool is_stunned() const { return has_effect("stun"); }
    bool is_paralyzed() const { return has_effect("paralysis"); }
    bool is_blinded() const { return has_effect("blind"); }
    bool is_confused() const { return has_effect("confusion"); }
    bool is_afraid() const { return has_effect("fear"); }
    bool is_charmed() const { return has_effect("charm"); }
    bool is_invisible() const { return has_effect("invisibility"); }
    bool is_invulnerable() const { return has_effect("invulnerable"); }
    bool is_berserk() const { return has_effect("berserk"); }
    
    // Check if entity can act (not stunned or paralyzed)
    bool can_act() const {
        for (const auto& effect : effects) {
            if (effect.is_expired()) continue;
            const auto* def = effect.get_definition();
            if (def && def->prevents_action) return false;
        }
        return true;
    }
    
    // Check if entity can move
    bool can_move() const {
        for (const auto& effect : effects) {
            if (effect.is_expired()) continue;
            const auto* def = effect.get_definition();
            if (def && def->prevents_movement) return false;
        }
        return true;
    }
    
    // Get fear source (for fleeing AI)
    EntityId get_fear_source() const {
        for (const auto& effect : effects) {
            if (effect.effect_type_id == "fear" && !effect.is_expired()) {
                return effect.source_id;
            }
        }
        return 0;
    }
    
    // Get charm source (for charmed AI)
    EntityId get_charm_source() const {
        for (const auto& effect : effects) {
            if (effect.effect_type_id == "charm" && !effect.is_expired()) {
                return effect.source_id;
            }
        }
        return 0;
    }
    
    // Get list of active effect names for display
    std::vector<std::string> get_effect_names() const {
        std::vector<std::string> names;
        for (const auto& effect : effects) {
            if (!effect.is_expired()) {
                names.push_back(effect.get_name());
            }
        }
        return names;
    }
    
    // Get list of active effect info for detailed display
    std::vector<std::pair<std::string, int>> get_effect_info() const {
        std::vector<std::pair<std::string, int>> info;
        for (const auto& effect : effects) {
            if (!effect.is_expired()) {
                info.emplace_back(effect.effect_type_id, effect.get_color());
            }
        }
        return info;
    }
    
    // Clone implementation
    IMPLEMENT_CLONE(StatusEffectsComponent)
};
