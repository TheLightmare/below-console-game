#pragma once
#include <string>
#include <ctime>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "../ecs/component.hpp"
#include "chunk_coord.hpp"

// Dungeon type enumeration for different procedural generation styles
enum class DungeonType {
    STANDARD_DUNGEON,   // Classic rooms and corridors
    CAVE,               // Organic cave with cellular automata
    MINE,               // Mine-like with tunnels
    CATACOMBS,          // Castle catacombs - narrow corridors
    LAIR,               // Dragon/boss lair - large central chamber
    TOMB                // Pharaoh's tomb - linear with traps
};

// Serialized effect data for item effects
struct SerializedEffect {
    int effect_type = 0;      // ItemEffectComponent::EffectType as int
    int trigger = 0;          // ItemEffectComponent::Trigger as int
    int magnitude = 0;
    int duration = 0;
    float chance = 1.0f;
    std::string message;
    std::string effect_id;    // Registered effect ID for custom effects
};

// Serialized item data for inventory transfer between scenes
struct SerializedItem {
    std::string name;
    std::string description;
    char symbol = '?';
    std::string color = "white";
    ItemComponent::Type type = ItemComponent::Type::MISC;
    ItemComponent::EquipSlot equip_slot = ItemComponent::EquipSlot::NONE;
    int value = 0;
    bool stackable = false;
    int stack_count = 1;
    int armor_bonus = 0;
    int attack_bonus = 0;
    int defense_bonus = 0;
    float cutting_chance = 0.0f;
    bool is_equipped = false;
    std::string equipped_to_limb;  // Which limb this item is equipped to (empty if not equipped)
    
    // Item effects (for consumables, weapons with on-hit effects, etc.)
    std::vector<SerializedEffect> effects;
    
    // ==================== NEW: Full component data ====================
    // Generic component JSON blob for complete item serialization
    bool has_full_components = false;
    std::string components_json;  // JSON string containing all component data
    int original_entity_id = 0;   // Original entity ID for reference remapping
};

// Serialized entity for saving world entities (NPCs, items, enemies)
struct SerializedEntity {
    // Entity type for reconstruction
    enum class EntityType {
        ITEM,           // Item on ground
        NPC,            // Friendly NPC
        ENEMY,          // Hostile creature
        INTERACTABLE,   // Chests, doors, etc.
        OTHER           // Fallback
    };
    
    EntityType entity_type = EntityType::OTHER;
    
    // Position
    int x = 0;
    int y = 0;
    int z = 0;
    
    // Render
    char symbol = '?';
    std::string color = "white";
    std::string bg_color = "";
    int render_priority = 0;
    
    // Name
    std::string name = "";
    std::string description = "";
    
    // Stats (for NPCs/enemies)
    bool has_stats = false;
    int level = 1;
    int current_hp = 20;
    int max_hp = 20;
    int attack = 5;
    int defense = 5;
    int experience = 0;
    
    // Utility AI (for enemies) - full AI is in components_json when has_full_components is true
    bool has_utility_ai = false;
    float aggro_range = 5.0f;
    float flee_threshold = 0.2f;
    std::string utility_ai_preset = "";  // Preset name for recreation (e.g. "aggressive_melee")
    
    // NPC data
    bool has_npc = false;
    int npc_disposition = 0;  // NPCComponent::Disposition as int
    int npc_role = 0;         // NPCComponent::Role as int
    std::string greeting = "";
    std::string farewell = "";
    int relationship = 0;
    
    // Item data (for items on ground)
    bool has_item = false;
    SerializedItem item_data;
    
    // Collision
    bool blocks_movement = false;
    bool blocks_sight = false;
    
    // Faction
    int faction_id = 0;
    
    // ==================== NEW: Full component data ====================
    // Generic component JSON blob for complete entity serialization
    // When this is set, the entity is fully reconstructed from components
    // This takes priority over the legacy fields above when deserializing
    bool has_full_components = false;
    std::string components_json;  // JSON string containing all component data
    int original_entity_id = 0;   // Original entity ID for reference remapping
};

// Shared state for dungeon transitions between exploration and dungeon scenes
struct DungeonState {
    // Dungeon generation parameters
    bool active = false;                // Is there an active dungeon instance?
    DungeonType type = DungeonType::STANDARD_DUNGEON;
    std::string name = "Unknown Dungeon";
    unsigned int seed = 0;              // Seed for deterministic generation
    int difficulty = 1;                 // Affects enemy count and level
    int depth = 1;                      // Current floor depth
    int max_depth = 3;                  // How many floors until reaching the end
    
    // Return coordinates (overworld position)
    int return_x = 0;
    int return_y = 0;
    bool has_return_point = false;
    
    // Current player position (for save/load)
    int player_x = 0;
    int player_y = 0;
    bool in_dungeon = false;            // True if currently in dungeon, false if exploration
    unsigned int world_seed = 0;        // Overworld generation seed
    bool loading_from_save = false;     // True when loading a saved game
    
    // Player state preservation (carried from overworld)
    int player_hp = 0;
    int player_max_hp = 0;
    int player_level = 1;
    int player_xp = 0;
    int player_attack = 8;
    int player_defense = 5;
    int player_gold = 0;
    
    // Full player component state (for components like Anatomy, Equipment, etc.)
    std::string player_components_json;  // JSON-serialized player components
    bool has_player_components = false;   // True if player_components_json is valid
    
    // Inventory transfer
    std::vector<SerializedItem> saved_inventory;
    
    // World entities (NPCs, items on ground, enemies in loaded chunks)
    std::vector<SerializedEntity> saved_entities;
    
    // Per-chunk entity persistence for overworld
    // Maps chunk coordinates to the list of entities that were in that chunk when unloaded
    std::unordered_map<ChunkCoord, std::vector<SerializedEntity>> chunk_entities;
    
    // Set of chunks that have been visited (to distinguish from never-generated chunks)
    std::unordered_set<ChunkCoord> visited_chunks;
    
    // Map sizing based on dungeon type
    int map_width = 80;
    int map_height = 60;
    
    // Death tracking for game over screen
    std::string death_reason;  // Describes what killed the player (e.g., "Slain by a Goblin")
    
    // Reset to defaults
    void reset() {
        active = false;
        type = DungeonType::STANDARD_DUNGEON;
        name = "Unknown Dungeon";
        seed = 0;
        difficulty = 1;
        depth = 1;
        max_depth = 3;
        return_x = 0;
        return_y = 0;
        has_return_point = false;
        player_x = 0;
        player_y = 0;
        in_dungeon = false;
        world_seed = 0;
        loading_from_save = false;
        player_hp = 0;
        player_max_hp = 0;
        player_level = 1;
        player_xp = 0;
        player_attack = 8;
        player_defense = 5;
        player_gold = 0;
        player_components_json.clear();
        has_player_components = false;
        saved_inventory.clear();
        saved_entities.clear();
        death_reason.clear();
        chunk_entities.clear();
        visited_chunks.clear();
        map_width = 80;
        map_height = 60;
    }
    
    // Configure dungeon based on type
    void configure_for_type(DungeonType t, int base_difficulty = 1) {
        type = t;
        difficulty = base_difficulty;
        seed = static_cast<unsigned int>(time(nullptr));
        
        switch (type) {
            case DungeonType::CAVE:
                name = "Dark Cave";
                map_width = 100;
                map_height = 80;
                max_depth = 2;
                break;
                
            case DungeonType::MINE:
                name = "Abandoned Mine";
                map_width = 90;
                map_height = 70;
                max_depth = 3;
                break;
                
            case DungeonType::CATACOMBS:
                name = "Castle Catacombs";
                map_width = 70;
                map_height = 70;
                max_depth = 4;
                break;
                
            case DungeonType::LAIR:
                name = "Dragon's Den";
                map_width = 100;
                map_height = 100;
                max_depth = 1;  // Single large floor with boss
                difficulty += 2;  // Harder enemies
                break;
                
            case DungeonType::TOMB:
                name = "Pharaoh's Tomb";
                map_width = 80;
                map_height = 80;
                max_depth = 3;
                break;
                
            default:  // STANDARD_DUNGEON
                name = "Ancient Dungeon";
                map_width = 80;
                map_height = 60;
                max_depth = 3;
                break;
        }
    }
};
