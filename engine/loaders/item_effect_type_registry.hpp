#pragma once
#include "../util/json.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

// ==================== DATA-DRIVEN ITEM EFFECT TYPE SYSTEM ====================
// Item effect types are defined in JSON files rather than hardcoded switch statements.
// This allows adding new effect types without recompiling.

// Definition of an item effect type loaded from JSON
struct ItemEffectTypeDefinition {
    std::string id;                     // Unique identifier (e.g., "heal_hp", "regeneration")
    std::string name;                   // Display name
    std::string description;            // Description of the effect
    
    // Behavior type determines how the effect is processed
    // Values: "instant_heal", "instant_damage", "instant_stat", "stat_modifier", "apply_status", "custom"
    std::string behavior;
    
    // For stat-based effects
    std::string stat;                   // "hp", "attack", "defense", "experience"
    std::string modifier_type;          // "add", "subtract", "multiply", "set"
    bool is_percent = false;            // For heal: use percent of max HP
    
    // For status effect application
    std::string status_effect_id;       // ID in EffectTypeRegistry (e.g., "poison", "regeneration")
    int default_duration = 5;           // Default duration if not specified
    
    // For custom effects
    std::string custom_handler;         // Name of custom handler function
    
    // Message templates
    std::string message_player;         // Message when affecting player (use {amount}, {target})
    std::string message_other;          // Message when affecting other entities
};

// Registry that loads and manages item effect type definitions
class ItemEffectTypeRegistry {
private:
    std::unordered_map<std::string, ItemEffectTypeDefinition> effect_types;
    bool loaded = false;
    
    ItemEffectTypeRegistry() = default;
    
public:
    static ItemEffectTypeRegistry& instance() {
        static ItemEffectTypeRegistry inst;
        return inst;
    }
    
    // Non-copyable
    ItemEffectTypeRegistry(const ItemEffectTypeRegistry&) = delete;
    ItemEffectTypeRegistry& operator=(const ItemEffectTypeRegistry&) = delete;
    
    // Load item effect types from JSON file
    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();
        
        json::Value root = json::parse(json_str);
        if (root.is_null() || !root.has("item_effect_types")) {
            return false;
        }
        
        const json::Value& types = root["item_effect_types"];
        if (!types.is_array()) {
            return false;
        }
        
        for (size_t i = 0; i < types.size(); ++i) {
            const json::Value& eff = types[i];
            
            ItemEffectTypeDefinition def;
            def.id = eff["id"].get_string("");
            if (def.id.empty()) continue;
            
            def.name = eff["name"].get_string(def.id);
            def.description = eff["description"].get_string("");
            def.behavior = eff["behavior"].get_string("custom");
            
            // Stat-based properties
            def.stat = eff["stat"].get_string("");
            def.modifier_type = eff["modifier_type"].get_string("add");
            def.is_percent = eff["is_percent"].get_bool(false);
            
            // Status effect properties
            def.status_effect_id = eff["status_effect_id"].get_string("");
            def.default_duration = eff["default_duration"].get_int(5);
            
            // Custom handler
            def.custom_handler = eff["custom_handler"].get_string("");
            
            // Messages
            def.message_player = eff["message_player"].get_string("");
            def.message_other = eff["message_other"].get_string("");
            
            effect_types[def.id] = def;
        }
        
        loaded = true;
        return true;
    }
    
    bool is_loaded() const { return loaded; }
    
    const ItemEffectTypeDefinition* get(const std::string& id) const {
        auto it = effect_types.find(id);
        return (it != effect_types.end()) ? &it->second : nullptr;
    }
    
    bool exists(const std::string& id) const {
        return effect_types.find(id) != effect_types.end();
    }
    
    std::vector<std::string> get_all_ids() const {
        std::vector<std::string> ids;
        ids.reserve(effect_types.size());
        for (const auto& [id, def] : effect_types) {
            ids.push_back(id);
        }
        return ids;
    }
    
    std::string get_name(const std::string& id) const {
        const ItemEffectTypeDefinition* def = get(id);
        return def ? def->name : id;
    }
    
    // Format a message template with actual values
    static std::string format_message(const std::string& tmpl, const std::string& target, int amount) {
        std::string result = tmpl;
        
        // Replace {target}
        size_t pos = result.find("{target}");
        while (pos != std::string::npos) {
            result.replace(pos, 8, target);
            pos = result.find("{target}", pos + target.length());
        }
        
        // Replace {amount}
        std::string amount_str = std::to_string(amount);
        pos = result.find("{amount}");
        while (pos != std::string::npos) {
            result.replace(pos, 8, amount_str);
            pos = result.find("{amount}", pos + amount_str.length());
        }
        
        return result;
    }
};

// Convenience function to load item effect types
inline bool load_item_effect_types(const std::string& filepath = "assets/effects/item_effects.json") {
    return ItemEffectTypeRegistry::instance().load_from_file(filepath);
}

// Convert legacy enum to string ID
inline std::string item_effect_type_to_string(int enum_value) {
    // Maps ItemEffectComponent::EffectType enum values to string IDs
    static const char* const names[] = {
        "heal_hp",           // 0: HEAL_HP
        "restore_hp_percent", // 1: RESTORE_HP_PERCENT
        "damage_hp",         // 2: DAMAGE_HP
        "buff_attack",       // 3: BUFF_ATTACK
        "buff_defense",      // 4: BUFF_DEFENSE
        "buff_armor",        // 5: BUFF_ARMOR
        "debuff_attack",     // 6: DEBUFF_ATTACK
        "debuff_defense",    // 7: DEBUFF_DEFENSE
        "gain_xp",           // 8: GAIN_XP
        "teleport",          // 9: TELEPORT
        "reveal_map",        // 10: REVEAL_MAP
        "poison",            // 11: POISON
        "regeneration",      // 12: REGENERATION
        "custom"             // 13: CUSTOM
    };
    
    if (enum_value >= 0 && enum_value < 14) {
        return names[enum_value];
    }
    return "custom";
}
