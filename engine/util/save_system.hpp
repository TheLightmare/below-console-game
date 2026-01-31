#pragma once
#include "json.hpp"
#include "../world/dungeon_state.hpp"
#include "../ecs/component.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>

// Number of available save slots
constexpr int MAX_SAVE_SLOTS = 5;

// SerializedEntity is now defined in dungeon_state.hpp

// Save slot metadata (shown in load/save menus)
struct SaveSlotInfo {
    int slot_number = 0;
    bool is_empty = true;
    std::string player_name = "";
    int player_level = 1;
    std::string location = "Unknown";
    std::string timestamp = "";
    int play_time_seconds = 0;  // Total play time
    
    std::string get_display_string() const {
        if (is_empty) {
            return "Slot " + std::to_string(slot_number) + ": [Empty]";
        }
        return "Slot " + std::to_string(slot_number) + ": " + player_name + 
               " Lv." + std::to_string(player_level) + " - " + location +
               " (" + timestamp + ")";
    }
};

// Complete game save data
struct GameSaveData {
    // Metadata
    std::string player_name = "Explorer";
    int play_time_seconds = 0;
    std::string save_timestamp = "";
    
    // Current scene
    std::string current_scene = "exploration";  // "exploration" or "dungeon"
    
    // World state
    unsigned int world_seed = 0;
    int player_x = 0;
    int player_y = 0;
    
    // Player stats
    int player_hp = 30;
    int player_max_hp = 30;
    int player_level = 1;
    int player_xp = 0;
    int player_attack = 8;
    int player_defense = 5;
    int player_gold = 0;
    
    // Full player component state (for components like Anatomy, Equipment, etc.)
    std::string player_components_json;  // JSON-serialized player components
    bool has_player_components = false;   // True if player_components_json is valid
    
    // Player inventory
    std::vector<SerializedItem> inventory;
    
    // Dungeon state (if in dungeon)
    bool in_dungeon = false;
    DungeonType dungeon_type = DungeonType::STANDARD_DUNGEON;
    std::string dungeon_name = "";
    unsigned int dungeon_seed = 0;
    int dungeon_depth = 1;
    int dungeon_max_depth = 3;
    int dungeon_difficulty = 1;
    int dungeon_return_x = 0;
    int dungeon_return_y = 0;
    
    // World entities (NPCs, items on ground, enemies in loaded chunks)
    std::vector<SerializedEntity> world_entities;
    
    // Visited chunks (to prevent respawning enemies)
    std::vector<std::pair<int, int>> visited_chunks;
    
    // Per-chunk entity persistence (entities in unloaded chunks)
    std::unordered_map<ChunkCoord, std::vector<SerializedEntity>> chunk_entities;
};

// Save System class
class SaveSystem {
private:
    std::string save_directory;
    
    std::string get_save_path(int slot) const {
        return save_directory + "/save_slot_" + std::to_string(slot) + ".json";
    }
    
    std::string get_current_timestamp() const {
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);
        return std::string(buffer);
    }
    
    // Convert DungeonType to string
    static std::string dungeon_type_to_string(DungeonType type) {
        switch (type) {
            case DungeonType::STANDARD_DUNGEON: return "standard";
            case DungeonType::CAVE: return "cave";
            case DungeonType::MINE: return "mine";
            case DungeonType::CATACOMBS: return "catacombs";
            case DungeonType::LAIR: return "lair";
            case DungeonType::TOMB: return "tomb";
            default: return "standard";
        }
    }
    
    // Convert string to DungeonType
    static DungeonType string_to_dungeon_type(const std::string& str) {
        if (str == "cave") return DungeonType::CAVE;
        if (str == "mine") return DungeonType::MINE;
        if (str == "catacombs") return DungeonType::CATACOMBS;
        if (str == "lair") return DungeonType::LAIR;
        if (str == "tomb") return DungeonType::TOMB;
        return DungeonType::STANDARD_DUNGEON;
    }
    
    // Convert ItemComponent::Type to string
    static std::string item_type_to_string(ItemComponent::Type type) {
        switch (type) {
            case ItemComponent::Type::WEAPON: return "weapon";
            case ItemComponent::Type::ARMOR: return "armor";
            case ItemComponent::Type::CONSUMABLE: return "consumable";
            case ItemComponent::Type::QUEST: return "quest";
            case ItemComponent::Type::HELMET: return "helmet";
            case ItemComponent::Type::BOOTS: return "boots";
            case ItemComponent::Type::GLOVES: return "gloves";
            case ItemComponent::Type::RING: return "ring";
            case ItemComponent::Type::SHIELD: return "shield";
            default: return "misc";
        }
    }
    
    // Convert string to ItemComponent::Type
    static ItemComponent::Type string_to_item_type(const std::string& str) {
        if (str == "weapon") return ItemComponent::Type::WEAPON;
        if (str == "armor") return ItemComponent::Type::ARMOR;
        if (str == "consumable") return ItemComponent::Type::CONSUMABLE;
        if (str == "quest") return ItemComponent::Type::QUEST;
        if (str == "helmet") return ItemComponent::Type::HELMET;
        if (str == "boots") return ItemComponent::Type::BOOTS;
        if (str == "gloves") return ItemComponent::Type::GLOVES;
        if (str == "ring") return ItemComponent::Type::RING;
        if (str == "shield") return ItemComponent::Type::SHIELD;
        return ItemComponent::Type::MISC;
    }
    
    // Convert ItemComponent::EquipSlot to string
    static std::string equip_slot_to_string(ItemComponent::EquipSlot slot) {
        switch (slot) {
            case ItemComponent::EquipSlot::HEAD: return "head";
            case ItemComponent::EquipSlot::TORSO: return "torso";
            case ItemComponent::EquipSlot::HAND: return "hand";
            case ItemComponent::EquipSlot::TWO_HAND: return "two_hand";
            case ItemComponent::EquipSlot::FOOT: return "foot";
            case ItemComponent::EquipSlot::FINGER: return "finger";
            case ItemComponent::EquipSlot::BACK: return "back";
            case ItemComponent::EquipSlot::NECK: return "neck";
            case ItemComponent::EquipSlot::WAIST: return "waist";
            default: return "none";
        }
    }
    
    // Convert string to ItemComponent::EquipSlot
    static ItemComponent::EquipSlot string_to_equip_slot(const std::string& str) {
        if (str == "head") return ItemComponent::EquipSlot::HEAD;
        if (str == "torso") return ItemComponent::EquipSlot::TORSO;
        if (str == "hand") return ItemComponent::EquipSlot::HAND;
        if (str == "two_hand") return ItemComponent::EquipSlot::TWO_HAND;
        if (str == "foot") return ItemComponent::EquipSlot::FOOT;
        if (str == "finger") return ItemComponent::EquipSlot::FINGER;
        if (str == "back") return ItemComponent::EquipSlot::BACK;
        if (str == "neck") return ItemComponent::EquipSlot::NECK;
        if (str == "waist") return ItemComponent::EquipSlot::WAIST;
        return ItemComponent::EquipSlot::NONE;
    }
    
    // Serialize a SerializedEffect to JSON
    static json::Object serialize_effect(const SerializedEffect& effect) {
        json::Object obj;
        obj["effect_type"] = effect.effect_type;
        obj["trigger"] = effect.trigger;
        obj["magnitude"] = effect.magnitude;
        obj["duration"] = effect.duration;
        obj["chance"] = effect.chance;
        obj["message"] = effect.message;
        obj["effect_id"] = effect.effect_id;
        return obj;
    }
    
    // Deserialize a SerializedEffect from JSON
    static SerializedEffect deserialize_effect(const json::Value& val) {
        SerializedEffect effect;
        effect.effect_type = val["effect_type"].get_int(0);
        effect.trigger = val["trigger"].get_int(0);
        effect.magnitude = val["magnitude"].get_int(0);
        effect.duration = val["duration"].get_int(0);
        effect.chance = static_cast<float>(val["chance"].get_number(1.0));
        effect.message = val["message"].get_string("");
        effect.effect_id = val["effect_id"].get_string("");
        return effect;
    }
    
    // Convert SerializedEntity::EntityType to string
    static std::string entity_type_to_string(SerializedEntity::EntityType type) {
        switch (type) {
            case SerializedEntity::EntityType::ITEM: return "item";
            case SerializedEntity::EntityType::NPC: return "npc";
            case SerializedEntity::EntityType::ENEMY: return "enemy";
            case SerializedEntity::EntityType::INTERACTABLE: return "interactable";
            default: return "other";
        }
    }
    
    // Convert string to SerializedEntity::EntityType
    static SerializedEntity::EntityType string_to_entity_type(const std::string& str) {
        if (str == "item") return SerializedEntity::EntityType::ITEM;
        if (str == "npc") return SerializedEntity::EntityType::NPC;
        if (str == "enemy") return SerializedEntity::EntityType::ENEMY;
        if (str == "interactable") return SerializedEntity::EntityType::INTERACTABLE;
        return SerializedEntity::EntityType::OTHER;
    }
    
    // Serialize a SerializedEntity to JSON object
    static json::Object serialize_entity(const SerializedEntity& ent) {
        json::Object obj;
        obj["entity_type"] = entity_type_to_string(ent.entity_type);
        obj["x"] = ent.x;
        obj["y"] = ent.y;
        obj["z"] = ent.z;
        obj["symbol"] = std::string(1, ent.symbol);
        obj["color"] = ent.color;
        obj["bg_color"] = ent.bg_color;
        obj["render_priority"] = ent.render_priority;
        obj["name"] = ent.name;
        obj["description"] = ent.description;
        obj["blocks_movement"] = ent.blocks_movement;
        obj["blocks_sight"] = ent.blocks_sight;
        obj["faction_id"] = ent.faction_id;
        
        // NEW: Full component data for complete entity serialization
        if (ent.has_full_components && !ent.components_json.empty()) {
            obj["has_full_components"] = true;
            // Parse the JSON string and store as actual JSON object (avoids escape characters)
            json::Value comp_val;
            if (json::try_parse(ent.components_json, comp_val)) {
                obj["components_json"] = comp_val;
            }
            obj["original_entity_id"] = ent.original_entity_id;
        }
        
        if (ent.has_stats) {
            obj["has_stats"] = true;
            obj["level"] = ent.level;
            obj["current_hp"] = ent.current_hp;
            obj["max_hp"] = ent.max_hp;
            obj["attack"] = ent.attack;
            obj["defense"] = ent.defense;
            obj["experience"] = ent.experience;
        }
        
        if (ent.has_utility_ai) {
            obj["has_utility_ai"] = true;
            obj["aggro_range"] = ent.aggro_range;
            obj["flee_threshold"] = ent.flee_threshold;
            obj["utility_ai_preset"] = ent.utility_ai_preset;
        }
        
        if (ent.has_npc) {
            obj["has_npc"] = true;
            obj["npc_disposition"] = ent.npc_disposition;
            obj["npc_role"] = ent.npc_role;
            obj["greeting"] = ent.greeting;
            obj["farewell"] = ent.farewell;
            obj["relationship"] = ent.relationship;
        }
        
        if (ent.has_item) {
            obj["has_item"] = true;
            json::Object item_obj;
            item_obj["name"] = ent.item_data.name;
            item_obj["description"] = ent.item_data.description;
            item_obj["symbol"] = std::string(1, ent.item_data.symbol);
            item_obj["color"] = ent.item_data.color;
            item_obj["type"] = item_type_to_string(ent.item_data.type);
            item_obj["equip_slot"] = equip_slot_to_string(ent.item_data.equip_slot);
            item_obj["value"] = ent.item_data.value;
            item_obj["stackable"] = ent.item_data.stackable;
            item_obj["stack_count"] = ent.item_data.stack_count;
            item_obj["armor_bonus"] = ent.item_data.armor_bonus;
            item_obj["attack_bonus"] = ent.item_data.attack_bonus;
            item_obj["defense_bonus"] = ent.item_data.defense_bonus;
            item_obj["cutting_chance"] = ent.item_data.cutting_chance;
            obj["item_data"] = json::Value(std::move(item_obj));
        }
        
        return obj;
    }
    
    // Deserialize a SerializedEntity from JSON
    static SerializedEntity deserialize_entity(const json::Value& val) {
        SerializedEntity ent;
        ent.entity_type = string_to_entity_type(val["entity_type"].get_string("other"));
        ent.x = val["x"].get_int(0);
        ent.y = val["y"].get_int(0);
        ent.z = val["z"].get_int(0);
        std::string sym = val["symbol"].get_string("?");
        ent.symbol = sym.empty() ? '?' : sym[0];
        ent.color = val["color"].get_string("white");
        ent.bg_color = val["bg_color"].get_string("");
        ent.render_priority = val["render_priority"].get_int(0);
        ent.name = val["name"].get_string("");
        ent.description = val["description"].get_string("");
        ent.blocks_movement = val["blocks_movement"].get_bool(false);
        ent.blocks_sight = val["blocks_sight"].get_bool(false);
        ent.faction_id = val["faction_id"].get_int(0);
        
        // NEW: Full component data for complete entity deserialization
        ent.has_full_components = val["has_full_components"].get_bool(false);
        if (ent.has_full_components) {
            // Handle both string format (legacy) and object format (new)
            if (val.has("components_json")) {
                if (val["components_json"].is_string()) {
                    ent.components_json = val["components_json"].get_string("");
                } else if (val["components_json"].is_object()) {
                    ent.components_json = json::stringify(val["components_json"]);
                }
            }
            ent.original_entity_id = val["original_entity_id"].get_int(0);
        }
        
        ent.has_stats = val["has_stats"].get_bool(false);
        if (ent.has_stats) {
            ent.level = val["level"].get_int(1);
            ent.current_hp = val["current_hp"].get_int(20);
            ent.max_hp = val["max_hp"].get_int(20);
            ent.attack = val["attack"].get_int(5);
            ent.defense = val["defense"].get_int(5);
            ent.experience = val["experience"].get_int(0);
        }
        
        // Support both old has_ai and new has_utility_ai formats for backward compatibility
        ent.has_utility_ai = val["has_utility_ai"].get_bool(val["has_ai"].get_bool(false));
        if (ent.has_utility_ai) {
            ent.aggro_range = static_cast<float>(val["aggro_range"].get_number(5.0));
            ent.flee_threshold = static_cast<float>(val["flee_threshold"].get_number(0.2));
            ent.utility_ai_preset = val["utility_ai_preset"].get_string("aggressive_melee");
        }
        
        ent.has_npc = val["has_npc"].get_bool(false);
        if (ent.has_npc) {
            ent.npc_disposition = val["npc_disposition"].get_int(0);
            ent.npc_role = val["npc_role"].get_int(0);
            ent.greeting = val["greeting"].get_string("");
            ent.farewell = val["farewell"].get_string("");
            ent.relationship = val["relationship"].get_int(0);
        }
        
        ent.has_item = val["has_item"].get_bool(false);
        if (ent.has_item && val.has("item_data")) {
            const auto& item_json = val["item_data"];
            ent.item_data.name = item_json["name"].get_string("");
            ent.item_data.description = item_json["description"].get_string("");
            std::string item_sym = item_json["symbol"].get_string("?");
            ent.item_data.symbol = item_sym.empty() ? '?' : item_sym[0];
            ent.item_data.color = item_json["color"].get_string("white");
            ent.item_data.type = string_to_item_type(item_json["type"].get_string("misc"));
            ent.item_data.equip_slot = string_to_equip_slot(item_json["equip_slot"].get_string("none"));
            ent.item_data.value = item_json["value"].get_int(0);
            ent.item_data.stackable = item_json["stackable"].get_bool(false);
            ent.item_data.stack_count = item_json["stack_count"].get_int(1);
            ent.item_data.armor_bonus = item_json["armor_bonus"].get_int(0);
            ent.item_data.attack_bonus = item_json["attack_bonus"].get_int(0);
            ent.item_data.defense_bonus = item_json["defense_bonus"].get_int(0);
            ent.item_data.cutting_chance = static_cast<float>(item_json["cutting_chance"].get_number(0.0));
        }
        
        return ent;
    }
    
public:
    SaveSystem(const std::string& save_dir = "saves") : save_directory(save_dir) {
        // Create save directory if it doesn't exist
        std::filesystem::create_directories(save_directory);
    }
    
    // Get info for all save slots
    std::vector<SaveSlotInfo> get_all_slot_info() const {
        std::vector<SaveSlotInfo> slots;
        
        for (int i = 1; i <= MAX_SAVE_SLOTS; ++i) {
            SaveSlotInfo info;
            info.slot_number = i;
            
            json::Value save_data;
            if (json::try_parse_file(get_save_path(i), save_data)) {
                info.is_empty = false;
                info.player_name = save_data["player_name"].get_string("Unknown");
                info.player_level = save_data["player_level"].get_int(1);
                info.location = save_data["current_scene"].get_string("exploration");
                if (info.location == "dungeon" && save_data.has("dungeon_name")) {
                    info.location = save_data["dungeon_name"].get_string("Dungeon");
                } else if (info.location == "exploration") {
                    info.location = "Overworld";
                }
                info.timestamp = save_data["save_timestamp"].get_string("");
                info.play_time_seconds = save_data["play_time_seconds"].get_int(0);
            }
            
            slots.push_back(info);
        }
        
        return slots;
    }
    
    // Check if a slot has a save
    bool slot_has_save(int slot) const {
        return std::filesystem::exists(get_save_path(slot));
    }
    
    // Save game to slot
    bool save_game(int slot, const GameSaveData& data) {
        json::Object save_obj;
        
        // Metadata
        save_obj["player_name"] = data.player_name;
        save_obj["play_time_seconds"] = data.play_time_seconds;
        save_obj["save_timestamp"] = get_current_timestamp();
        save_obj["save_version"] = 1;  // For future compatibility
        
        // Scene state
        save_obj["current_scene"] = data.current_scene;
        save_obj["world_seed"] = static_cast<int>(data.world_seed);
        save_obj["player_x"] = data.player_x;
        save_obj["player_y"] = data.player_y;
        
        // Player stats
        save_obj["player_hp"] = data.player_hp;
        save_obj["player_max_hp"] = data.player_max_hp;
        save_obj["player_level"] = data.player_level;
        save_obj["player_xp"] = data.player_xp;
        save_obj["player_attack"] = data.player_attack;
        save_obj["player_defense"] = data.player_defense;
        save_obj["player_gold"] = data.player_gold;
        
        // Player full component state
        if (data.has_player_components && !data.player_components_json.empty()) {
            // Parse the JSON string and store as actual JSON object (avoids escape characters)
            json::Value comp_val;
            if (json::try_parse(data.player_components_json, comp_val)) {
                save_obj["player_components_json"] = comp_val;
            }
            save_obj["has_player_components"] = true;
        }
        
        // Inventory
        json::Array inventory_arr;
        for (const auto& item : data.inventory) {
            json::Object item_obj;
            item_obj["name"] = item.name;
            item_obj["description"] = item.description;
            item_obj["symbol"] = std::string(1, item.symbol);
            item_obj["color"] = item.color;
            item_obj["type"] = item_type_to_string(item.type);
            item_obj["equip_slot"] = equip_slot_to_string(item.equip_slot);
            item_obj["value"] = item.value;
            item_obj["stackable"] = item.stackable;
            item_obj["stack_count"] = item.stack_count;
            item_obj["armor_bonus"] = item.armor_bonus;
            item_obj["attack_bonus"] = item.attack_bonus;
            item_obj["defense_bonus"] = item.defense_bonus;
            item_obj["cutting_chance"] = item.cutting_chance;
            item_obj["is_equipped"] = item.is_equipped;
            item_obj["equipped_to_limb"] = item.equipped_to_limb;
            
            // Serialize item effects
            if (!item.effects.empty()) {
                json::Array effects_arr;
                for (const auto& effect : item.effects) {
                    effects_arr.push_back(json::Value(serialize_effect(effect)));
                }
                item_obj["effects"] = json::Value(std::move(effects_arr));
            }
            
            // NEW: Full component data for complete item serialization
            if (item.has_full_components && !item.components_json.empty()) {
                item_obj["has_full_components"] = true;
                // Parse the JSON string and store as actual JSON object (avoids escape characters)
                json::Value comp_val;
                if (json::try_parse(item.components_json, comp_val)) {
                    item_obj["components_json"] = comp_val;
                }
                item_obj["original_entity_id"] = item.original_entity_id;
            }
            
            inventory_arr.push_back(json::Value(std::move(item_obj)));
        }
        save_obj["inventory"] = json::Value(std::move(inventory_arr));
        
        // Dungeon state
        save_obj["in_dungeon"] = data.in_dungeon;
        if (data.in_dungeon) {
            save_obj["dungeon_type"] = dungeon_type_to_string(data.dungeon_type);
            save_obj["dungeon_name"] = data.dungeon_name;
            save_obj["dungeon_seed"] = static_cast<int>(data.dungeon_seed);
            save_obj["dungeon_depth"] = data.dungeon_depth;
            save_obj["dungeon_max_depth"] = data.dungeon_max_depth;
            save_obj["dungeon_difficulty"] = data.dungeon_difficulty;
            save_obj["dungeon_return_x"] = data.dungeon_return_x;
            save_obj["dungeon_return_y"] = data.dungeon_return_y;
        }
        
        // World entities
        json::Array entities_arr;
        for (const auto& ent : data.world_entities) {
            entities_arr.push_back(json::Value(serialize_entity(ent)));
        }
        save_obj["world_entities"] = json::Value(std::move(entities_arr));
        
        // Visited chunks (prevents enemy respawning)
        json::Array visited_chunks_arr;
        for (const auto& coord : data.visited_chunks) {
            json::Object chunk_obj;
            chunk_obj["x"] = coord.first;
            chunk_obj["y"] = coord.second;
            visited_chunks_arr.push_back(json::Value(std::move(chunk_obj)));
        }
        save_obj["visited_chunks"] = json::Value(std::move(visited_chunks_arr));
        
        // Per-chunk entity persistence (entities in unloaded chunks)
        json::Array chunk_entities_arr;
        for (const auto& [coord, entities] : data.chunk_entities) {
            json::Object chunk_data;
            chunk_data["chunk_x"] = coord.x;
            chunk_data["chunk_y"] = coord.y;
            
            json::Array chunk_ents_arr;
            for (const auto& ent : entities) {
                chunk_ents_arr.push_back(json::Value(serialize_entity(ent)));
            }
            chunk_data["entities"] = json::Value(std::move(chunk_ents_arr));
            chunk_entities_arr.push_back(json::Value(std::move(chunk_data)));
        }
        save_obj["chunk_entities"] = json::Value(std::move(chunk_entities_arr));
        
        return json::write_file(get_save_path(slot), json::Value(std::move(save_obj)));
    }
    
    // Load game from slot
    bool load_game(int slot, GameSaveData& data) {
        json::Value save_data;
        if (!json::try_parse_file(get_save_path(slot), save_data)) {
            return false;
        }
        
        // Metadata
        data.player_name = save_data["player_name"].get_string("Explorer");
        data.play_time_seconds = save_data["play_time_seconds"].get_int(0);
        data.save_timestamp = save_data["save_timestamp"].get_string("");
        
        // Scene state
        data.current_scene = save_data["current_scene"].get_string("exploration");
        data.world_seed = static_cast<unsigned int>(save_data["world_seed"].get_int(0));
        data.player_x = save_data["player_x"].get_int(0);
        data.player_y = save_data["player_y"].get_int(0);
        
        // Player stats
        data.player_hp = save_data["player_hp"].get_int(30);
        data.player_max_hp = save_data["player_max_hp"].get_int(30);
        data.player_level = save_data["player_level"].get_int(1);
        data.player_xp = save_data["player_xp"].get_int(0);
        data.player_attack = save_data["player_attack"].get_int(8);
        data.player_defense = save_data["player_defense"].get_int(5);
        data.player_gold = save_data["player_gold"].get_int(0);
        
        // Player full component state
        data.has_player_components = save_data["has_player_components"].get_bool(false);
        if (data.has_player_components && save_data.has("player_components_json")) {
            // Handle both string format (legacy) and object format (new)
            if (save_data["player_components_json"].is_string()) {
                data.player_components_json = save_data["player_components_json"].get_string("");
            } else if (save_data["player_components_json"].is_object()) {
                data.player_components_json = json::stringify(save_data["player_components_json"]);
            }
        }
        
        // Inventory
        data.inventory.clear();
        if (save_data.has("inventory") && save_data["inventory"].is_array()) {
            for (size_t i = 0; i < save_data["inventory"].size(); ++i) {
                const auto& item_json = save_data["inventory"][i];
                SerializedItem item;
                item.name = item_json["name"].get_string("");
                item.description = item_json["description"].get_string("");
                std::string sym_str = item_json["symbol"].get_string("?");
                item.symbol = sym_str.empty() ? '?' : sym_str[0];
                item.color = item_json["color"].get_string("white");
                item.type = string_to_item_type(item_json["type"].get_string("misc"));
                item.equip_slot = string_to_equip_slot(item_json["equip_slot"].get_string("none"));
                item.value = item_json["value"].get_int(0);
                item.stackable = item_json["stackable"].get_bool(false);
                item.stack_count = item_json["stack_count"].get_int(1);
                item.armor_bonus = item_json["armor_bonus"].get_int(0);
                item.attack_bonus = item_json["attack_bonus"].get_int(0);
                item.defense_bonus = item_json["defense_bonus"].get_int(0);
                item.cutting_chance = static_cast<float>(item_json["cutting_chance"].get_number(0.0));
                item.is_equipped = item_json["is_equipped"].get_bool(false);
                item.equipped_to_limb = item_json["equipped_to_limb"].get_string("");
                
                // Deserialize item effects
                if (item_json.has("effects") && item_json["effects"].is_array()) {
                    for (size_t j = 0; j < item_json["effects"].size(); ++j) {
                        item.effects.push_back(deserialize_effect(item_json["effects"][j]));
                    }
                }
                
                // NEW: Full component data for complete item deserialization
                item.has_full_components = item_json["has_full_components"].get_bool(false);
                if (item.has_full_components && item_json.has("components_json")) {
                    // Handle both string format (legacy) and object format (new)
                    if (item_json["components_json"].is_string()) {
                        item.components_json = item_json["components_json"].get_string("");
                    } else if (item_json["components_json"].is_object()) {
                        item.components_json = stringify(item_json["components_json"]);
                    } else if (item_json["components_json"].is_array()) {
                        // Unexpected but handle it
                        item.components_json = stringify(item_json["components_json"]);
                    }
                    item.original_entity_id = item_json["original_entity_id"].get_int(0);
                }
                
                data.inventory.push_back(item);
            }
        }
        
        // Dungeon state
        data.in_dungeon = save_data["in_dungeon"].get_bool(false);
        if (data.in_dungeon) {
            data.dungeon_type = string_to_dungeon_type(save_data["dungeon_type"].get_string("standard"));
            data.dungeon_name = save_data["dungeon_name"].get_string("Dungeon");
            data.dungeon_seed = static_cast<unsigned int>(save_data["dungeon_seed"].get_int(0));
            data.dungeon_depth = save_data["dungeon_depth"].get_int(1);
            data.dungeon_max_depth = save_data["dungeon_max_depth"].get_int(3);
            data.dungeon_difficulty = save_data["dungeon_difficulty"].get_int(1);
            data.dungeon_return_x = save_data["dungeon_return_x"].get_int(0);
            data.dungeon_return_y = save_data["dungeon_return_y"].get_int(0);
        }
        
        // World entities
        data.world_entities.clear();
        if (save_data.has("world_entities") && save_data["world_entities"].is_array()) {
            for (size_t i = 0; i < save_data["world_entities"].size(); ++i) {
                data.world_entities.push_back(deserialize_entity(save_data["world_entities"][i]));
            }
        }
        
        // Visited chunks
        data.visited_chunks.clear();
        if (save_data.has("visited_chunks") && save_data["visited_chunks"].is_array()) {
            for (size_t i = 0; i < save_data["visited_chunks"].size(); ++i) {
                const auto& chunk_json = save_data["visited_chunks"][i];
                int cx = chunk_json["x"].get_int(0);
                int cy = chunk_json["y"].get_int(0);
                data.visited_chunks.push_back({cx, cy});
            }
        }
        
        // Per-chunk entity persistence
        data.chunk_entities.clear();
        if (save_data.has("chunk_entities") && save_data["chunk_entities"].is_array()) {
            for (size_t i = 0; i < save_data["chunk_entities"].size(); ++i) {
                const auto& chunk_data = save_data["chunk_entities"][i];
                int cx = chunk_data["chunk_x"].get_int(0);
                int cy = chunk_data["chunk_y"].get_int(0);
                ChunkCoord coord(cx, cy);
                
                if (chunk_data.has("entities") && chunk_data["entities"].is_array()) {
                    std::vector<SerializedEntity> entities;
                    for (size_t j = 0; j < chunk_data["entities"].size(); ++j) {
                        entities.push_back(deserialize_entity(chunk_data["entities"][j]));
                    }
                    data.chunk_entities[coord] = std::move(entities);
                }
            }
        }
        
        return true;
    }
    
    // Delete a save slot
    bool delete_save(int slot) {
        std::string path = get_save_path(slot);
        if (std::filesystem::exists(path)) {
            return std::filesystem::remove(path);
        }
        return true;  // Already doesn't exist
    }
    
    // Get singleton instance
    static SaveSystem& instance() {
        static SaveSystem system;
        return system;
    }
};
