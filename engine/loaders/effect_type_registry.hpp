#pragma once
#include "../util/json.hpp"
#include "../render/console.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>

// ==================== DATA-DRIVEN EFFECT TYPE SYSTEM ====================
// Effect types are defined in JSON files rather than hardcoded enums.
// This allows adding new effect types without recompiling.

// Definition of an effect type loaded from JSON
struct EffectTypeDefinition {
    std::string id;                     // Unique identifier (e.g., "poison", "buff_attack")
    std::string name;                   // Display name (e.g., "Poisoned", "Attack Boost")
    std::string description;            // Description of the effect
    std::string category;               // Category: instant, damage_over_time, healing_over_time, stat_modifier, control, misc
    std::string color;                  // Color name for UI
    unsigned int color_hex = 0xFFFFFF;  // Hex color value
    
    // Behavior flags
    bool is_instant = false;            // True for instant effects (heal, damage, gain_xp)
    bool is_negative = false;           // True for debuffs/harmful effects
    
    // Tick behavior (for DoT/HoT effects)
    std::string tick_behavior;          // "damage", "heal", or empty
    
    // Stat modifier properties
    std::string stat;                   // "attack", "defense", "speed"
    std::string modifier_type;          // "add", "subtract", "multiply"
    
    // Control effect flags
    bool prevents_action = false;       // Stun - can't do anything
    bool prevents_movement = false;     // Paralysis - can't move
    bool reduces_vision = false;        // Blind
    bool randomizes_movement = false;   // Confusion
    bool causes_flee = false;           // Fear
    bool changes_allegiance = false;    // Charm
    
    // Misc flags
    bool grants_invisibility = false;
    bool grants_invulnerability = false;
    bool attacks_anyone = false;        // Berserk
    
    // Get Color enum from color string
    Color get_color_enum() const {
        static const std::unordered_map<std::string, Color> color_map = {
            {"black", Color::BLACK},
            {"dark_blue", Color::DARK_BLUE},
            {"dark_green", Color::DARK_GREEN},
            {"dark_cyan", Color::DARK_CYAN},
            {"dark_red", Color::DARK_RED},
            {"dark_magenta", Color::DARK_MAGENTA},
            {"dark_yellow", Color::DARK_YELLOW},
            {"gray", Color::GRAY},
            {"dark_gray", Color::DARK_GRAY},
            {"blue", Color::BLUE},
            {"green", Color::GREEN},
            {"cyan", Color::CYAN},
            {"red", Color::RED},
            {"magenta", Color::MAGENTA},
            {"yellow", Color::YELLOW},
            {"white", Color::WHITE},
            {"orange", Color::RED}  // Fallback for orange
        };
        auto it = color_map.find(color);
        return (it != color_map.end()) ? it->second : Color::WHITE;
    }
};

// Registry that loads and manages effect type definitions
class EffectTypeRegistry {
private:
    std::unordered_map<std::string, EffectTypeDefinition> effect_types;
    bool loaded = false;
    
    // Singleton instance
    EffectTypeRegistry() = default;
    
public:
    static EffectTypeRegistry& instance() {
        static EffectTypeRegistry inst;
        return inst;
    }
    
    // Non-copyable
    EffectTypeRegistry(const EffectTypeRegistry&) = delete;
    EffectTypeRegistry& operator=(const EffectTypeRegistry&) = delete;
    
    // Load effect types from JSON file
    bool load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();
        
        json::Value root = json::parse(json_str);
        if (root.is_null() || !root.has("effect_types")) {
            return false;
        }
        
        const json::Value& types = root["effect_types"];
        if (!types.is_array()) {
            return false;
        }
        
        for (size_t i = 0; i < types.size(); ++i) {
            const json::Value& eff = types[i];
            
            EffectTypeDefinition def;
            def.id = eff["id"].get_string("");
            if (def.id.empty()) continue;
            
            def.name = eff["name"].get_string(def.id);
            def.description = eff["description"].get_string("");
            def.category = eff["category"].get_string("misc");
            def.color = eff["color"].get_string("white");
            
            // Parse hex color
            std::string hex_str = eff["color_hex"].get_string("FFFFFF");
            def.color_hex = std::stoul(hex_str, nullptr, 16);
            
            // Behavior flags
            def.is_instant = eff["is_instant"].get_bool(false);
            def.is_negative = eff["is_negative"].get_bool(false);
            def.tick_behavior = eff["tick_behavior"].get_string("");
            
            // Stat modifier
            def.stat = eff["stat"].get_string("");
            def.modifier_type = eff["modifier_type"].get_string("");
            
            // Control flags
            def.prevents_action = eff["prevents_action"].get_bool(false);
            def.prevents_movement = eff["prevents_movement"].get_bool(false);
            def.reduces_vision = eff["reduces_vision"].get_bool(false);
            def.randomizes_movement = eff["randomizes_movement"].get_bool(false);
            def.causes_flee = eff["causes_flee"].get_bool(false);
            def.changes_allegiance = eff["changes_allegiance"].get_bool(false);
            
            // Misc flags
            def.grants_invisibility = eff["grants_invisibility"].get_bool(false);
            def.grants_invulnerability = eff["grants_invulnerability"].get_bool(false);
            def.attacks_anyone = eff["attacks_anyone"].get_bool(false);
            
            effect_types[def.id] = def;
        }
        
        loaded = true;
        return true;
    }
    
    // Check if registry has been loaded
    bool is_loaded() const { return loaded; }
    
    // Get effect type definition by ID
    const EffectTypeDefinition* get(const std::string& id) const {
        auto it = effect_types.find(id);
        return (it != effect_types.end()) ? &it->second : nullptr;
    }
    
    // Check if effect type exists
    bool exists(const std::string& id) const {
        return effect_types.find(id) != effect_types.end();
    }
    
    // Get all effect type IDs
    std::vector<std::string> get_all_ids() const {
        std::vector<std::string> ids;
        ids.reserve(effect_types.size());
        for (const auto& [id, def] : effect_types) {
            ids.push_back(id);
        }
        return ids;
    }
    
    // Get display name for effect type
    std::string get_name(const std::string& id) const {
        const EffectTypeDefinition* def = get(id);
        return def ? def->name : id;
    }
    
    // Get color for effect type
    Color get_color(const std::string& id) const {
        const EffectTypeDefinition* def = get(id);
        return def ? def->get_color_enum() : Color::WHITE;
    }
    
    // Get hex color for effect type
    unsigned int get_color_hex(const std::string& id) const {
        const EffectTypeDefinition* def = get(id);
        return def ? def->color_hex : 0xFFFFFF;
    }
    
    // Check if effect type is instant
    bool is_instant(const std::string& id) const {
        const EffectTypeDefinition* def = get(id);
        return def ? def->is_instant : false;
    }
    
    // Check if effect type is negative/harmful
    bool is_negative(const std::string& id) const {
        const EffectTypeDefinition* def = get(id);
        return def ? def->is_negative : false;
    }
    
    // Register a new effect type at runtime (for mods, etc.)
    void register_effect_type(const EffectTypeDefinition& def) {
        if (!def.id.empty()) {
            effect_types[def.id] = def;
        }
    }
    
    // Get number of registered effect types
    size_t size() const { return effect_types.size(); }
};

// Helper function to load effect types - call during game initialization
inline bool load_effect_types(const std::string& filepath = "assets/effects/effects.json") {
    return EffectTypeRegistry::instance().load_from_file(filepath);
}
