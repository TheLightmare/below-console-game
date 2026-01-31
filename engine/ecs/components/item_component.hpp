#pragma once
#include "component_base.hpp"
#include "inventory_component.hpp"  // For LimbType
#include "../types.hpp"
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

// Item component
struct ItemComponent : Component {
    enum class Type {
        CONSUMABLE,
        WEAPON,
        ARMOR,
        HELMET,
        BOOTS,
        GLOVES,
        RING,
        SHIELD,
        QUEST,
        MISC
    };
    
    enum class EquipSlot {
        NONE,
        HEAD,
        TORSO,
        HAND,      // One-handed weapon/shield
        TWO_HAND,  // Two-handed weapon (requires 2 hands)
        FOOT,
        FINGER,
        BACK,
        NECK,
        WAIST
    };
    
    Type type = Type::MISC;
    EquipSlot equip_slot = EquipSlot::NONE;
    int value = 0;
    bool stackable = false;
    int stack_count = 1;
    
    // Equipment stats
    int armor_bonus = 0;
    int attack_bonus = 0;      // Bonus to attack stat when equipped
    int base_damage = 0;        // Base weapon damage (melee damage is weapon-dependent)
    int defense_bonus = 0;
    float cutting_chance = 0.0f;  // 0.0 = no cutting, 1.0 = always cuts on hit
    
    // Requirements
    int required_limbs = 1;  // How many limbs needed to use (e.g., 2 for two-handed sword)
    LimbType required_limb_type = LimbType::HAND;  // What type of limb is needed
    
    ItemComponent() = default;
    ItemComponent(Type type, int value, bool stackable = false)
        : type(type), value(value), stackable(stackable) {}
    IMPLEMENT_CLONE(ItemComponent)
};

// Ranged weapon component - for bows, crossbows, thrown weapons, etc.
struct RangedWeaponComponent : Component {
    enum class AmmoType {
        NONE,       // Thrown weapons (no ammo needed, weapon IS the ammo)
        ARROW,      // For bows
        BOLT,       // For crossbows
        STONE,      // For slings
        BULLET,     // For guns
        MAGIC       // For magic staves (uses mana if implemented)
    };
    
    int range = 5;              // Maximum attack range in tiles
    int min_range = 1;          // Minimum range (0 = can attack adjacent)
    AmmoType ammo_type = AmmoType::NONE;
    bool consumes_self = false; // True for thrown weapons (dagger, javelin)
    int accuracy_bonus = 0;     // Bonus to hit chance
    char projectile_char = '*'; // Character shown during projectile animation
    std::string projectile_color = "white";
    
    RangedWeaponComponent() = default;
    RangedWeaponComponent(int range, AmmoType ammo = AmmoType::NONE, bool consumes_self = false)
        : range(range), ammo_type(ammo), consumes_self(consumes_self) {}
    
    bool requires_ammo() const { return ammo_type != AmmoType::NONE && ammo_type != AmmoType::MAGIC; }
    IMPLEMENT_CLONE(RangedWeaponComponent)
};

// Ammo component - for arrows, bolts, stones, etc.
struct AmmoComponent : Component {
    RangedWeaponComponent::AmmoType ammo_type = RangedWeaponComponent::AmmoType::ARROW;
    int quantity = 1;
    int damage_bonus = 0;  // Additional damage from ammo quality
    
    AmmoComponent() = default;
    AmmoComponent(RangedWeaponComponent::AmmoType type, int qty = 1, int dmg_bonus = 0)
        : ammo_type(type), quantity(qty), damage_bonus(dmg_bonus) {}
    IMPLEMENT_CLONE(AmmoComponent)
};

// Forward declaration for effect registry
struct ItemEffectComponent;

// Effect callback signature: (target_entity_id, user_entity_id, component_manager, message_callback) -> success
// user_id is the entity that used/threw the item (0 if self-use or unknown)
using CustomEffectCallback = std::function<bool(EntityId target_id, EntityId user_id, ComponentManager*, std::function<void(const std::string&)>)>;

// Effect Registry - allows custom effects to be serialized by ID
// Effects are registered by name and can be looked up at runtime for deserialization
class EffectRegistry {
public:
    static EffectRegistry& instance() {
        static EffectRegistry registry;
        return registry;
    }
    
    // Register a custom effect callback with a unique name
    void register_effect(const std::string& effect_id, CustomEffectCallback callback) {
        effects_[effect_id] = callback;
    }
    
    // Get a registered effect by name (returns nullptr if not found)
    CustomEffectCallback get_effect(const std::string& effect_id) const {
        auto it = effects_.find(effect_id);
        return (it != effects_.end()) ? it->second : nullptr;
    }
    
    // Check if an effect is registered
    bool has_effect(const std::string& effect_id) const {
        return effects_.find(effect_id) != effects_.end();
    }
    
private:
    EffectRegistry() = default;
    std::unordered_map<std::string, CustomEffectCallback> effects_;
};

// Item effect component - tags items with effects that trigger on use/equip
struct ItemEffectComponent : Component {
    enum class EffectType {
        HEAL_HP,
        RESTORE_HP_PERCENT,
        DAMAGE_HP,
        BUFF_ATTACK,
        BUFF_DEFENSE,
        BUFF_ARMOR,
        DEBUFF_ATTACK,
        DEBUFF_DEFENSE,
        GAIN_XP,
        TELEPORT,
        REVEAL_MAP,
        POISON,
        REGENERATION,
        CUSTOM
    };
    
    enum class Trigger {
        ON_USE,      // Consumable items
        ON_EQUIP,    // When equipped
        ON_UNEQUIP,  // When unequipped
        ON_HIT,      // When used to hit enemy
        PASSIVE      // Continuous effect while equipped
    };
    
    struct Effect {
        EffectType type;
        Trigger trigger;
        int magnitude = 0;        // Amount (HP healed, damage, stat bonus, etc.)
        int duration = 0;         // Turns (0 = instant, -1 = permanent)
        float chance = 1.0f;      // Probability to trigger (0.0-1.0)
        std::string message;      // Message to display when triggered
        std::string effect_id;    // Serializable ID for custom effects (looked up in EffectRegistry)
        CustomEffectCallback custom_callback;  // Lambda for CUSTOM effect type (transient, not serialized)
        
        Effect() = default;
        Effect(EffectType t, Trigger trig, int mag = 0, int dur = 0, float ch = 1.0f, const std::string& msg = "")
            : type(t), trigger(trig), magnitude(mag), duration(dur), chance(ch), message(msg), effect_id(""), custom_callback(nullptr) {}
        
        // Constructor with custom callback (inline lambda - use effect_id for serialization)
        Effect(EffectType t, Trigger trig, CustomEffectCallback callback, const std::string& msg = "")
            : type(t), trigger(trig), magnitude(0), duration(0), chance(1.0f), message(msg), effect_id(""), custom_callback(callback) {}
        
        // Constructor with effect_id for serializable custom effects
        Effect(EffectType t, Trigger trig, const std::string& eff_id, const std::string& msg = "")
            : type(t), trigger(trig), magnitude(0), duration(0), chance(1.0f), message(msg), effect_id(eff_id), custom_callback(nullptr) {}
        
        // Get the callback for this effect (from registry if effect_id is set, else from custom_callback)
        CustomEffectCallback get_callback() const {
            if (!effect_id.empty()) {
                return EffectRegistry::instance().get_effect(effect_id);
            }
            return custom_callback;
        }
    };
    
    std::vector<Effect> effects;
    
    ItemEffectComponent() = default;
    
    void add_effect(EffectType type, Trigger trigger, int magnitude = 0, int duration = 0, 
                   float chance = 1.0f, const std::string& message = "") {
        effects.emplace_back(type, trigger, magnitude, duration, chance, message);
    }
    
    // Add a custom effect with a lambda callback (not serializable - use registered effects for save/load)
    void add_custom_effect(Trigger trigger, CustomEffectCallback callback, const std::string& message = "") {
        effects.emplace_back(EffectType::CUSTOM, trigger, callback, message);
    }
    
    // Add a custom effect using a registered effect ID (serializable)
    void add_registered_effect(Trigger trigger, const std::string& effect_id, const std::string& message = "") {
        effects.emplace_back(EffectType::CUSTOM, trigger, effect_id, message);
    }
    
    // Get all effects of a specific trigger type
    std::vector<Effect> get_effects_by_trigger(Trigger trigger) const {
        std::vector<Effect> result;
        for (const auto& effect : effects) {
            if (effect.trigger == trigger) {
                result.push_back(effect);
            }
        }
        return result;
    }
    IMPLEMENT_CLONE(ItemEffectComponent)
};
