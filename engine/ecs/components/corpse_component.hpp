#pragma once
#include "component_base.hpp"
#include "../types.hpp"
#include <string>

// ==================== CORPSE SYSTEM ====================
// Components for entities that leave corpses behind when they die

// Component marking that an entity leaves a corpse when it dies
struct LeavesCorpseComponent : Component {
    std::string corpse_name;        // Name for the corpse (e.g., "Goblin Corpse")
    std::string corpse_description; // Description of the corpse
    char corpse_symbol = '%';       // Symbol to display for corpse
    std::string corpse_color = "dark_red";  // Color of corpse
    bool transfer_inventory = true;  // Whether to transfer inventory to corpse
    bool transfer_anatomy = true;    // Whether to transfer anatomy (for butchering)
    bool transfer_equipment = true;  // Whether to transfer equipped items to corpse
    int decay_turns = -1;           // Turns until corpse decays (-1 = never)
    
    LeavesCorpseComponent() = default;
    
    LeavesCorpseComponent(const std::string& name, const std::string& desc = "")
        : corpse_name(name), corpse_description(desc) {}
    
    // Clone implementation
    IMPLEMENT_CLONE(LeavesCorpseComponent)
};

// Component marking an entity as a corpse (for identification and decay)
struct CorpseComponent : Component {
    std::string original_entity_name;  // Name of the creature that died
    std::string original_entity_id;    // Template ID of original entity (if any)
    EntityId original_entity = 0;      // Original entity ID (for reference)
    int death_turn = 0;                // Turn when the entity died
    int decay_turns_remaining = -1;    // Turns until decay (-1 = never decays)
    bool is_butchered = false;         // Whether the corpse has been butchered
    
    CorpseComponent() = default;
    
    CorpseComponent(const std::string& name, EntityId original = 0, int decay = -1)
        : original_entity_name(name), original_entity(original), decay_turns_remaining(decay) {}
    
    bool will_decay() const { return decay_turns_remaining > 0; }
    bool is_decayed() const { return decay_turns_remaining == 0; }
    
    // Tick decay - returns true if corpse should be removed
    bool tick_decay() {
        if (decay_turns_remaining > 0) {
            decay_turns_remaining--;
            return decay_turns_remaining == 0;
        }
        return false;
    }
    
    // Clone implementation
    IMPLEMENT_CLONE(CorpseComponent)
};

// Helper to generate corpse name from entity name
inline std::string make_corpse_name(const std::string& entity_name) {
    // Handle "a/an" prefixes
    if (entity_name.find("a ") == 0 || entity_name.find("an ") == 0) {
        size_t space = entity_name.find(' ');
        return entity_name.substr(0, space + 1) + entity_name.substr(space + 1) + " corpse";
    }
    return entity_name + " corpse";
}

// Helper to generate corpse description
inline std::string make_corpse_description(const std::string& entity_name) {
    return "The remains of " + entity_name + ".";
}
