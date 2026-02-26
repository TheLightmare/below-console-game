#pragma once
// Component Serialization System
// This file provides JSON serialization/deserialization for all game components.
// Include this file when you need to save/load entities with full component state.

#include "component.hpp"
#include "component_manager.hpp"
#include "serialization/enum_converters.hpp"
#include "../systems/utility_ai.hpp"
#include "../util/json.hpp"
#include <typeindex>
#include <functional>

// ==================== Component Serialization Helpers ====================

namespace ComponentSerializer {

// Import enum converters from centralized location
using EnumConverters::limb_type_to_string;
using EnumConverters::string_to_limb_type;
using EnumConverters::item_type_to_string;
using EnumConverters::string_to_item_type;
using EnumConverters::equip_slot_to_string;
using EnumConverters::string_to_equip_slot;
using EnumConverters::disposition_to_string;
using EnumConverters::string_to_disposition;
using EnumConverters::role_to_string;
using EnumConverters::string_to_role;
using EnumConverters::ammo_type_to_string;
using EnumConverters::string_to_ammo_type;
using EnumConverters::utility_action_type_to_string;
using EnumConverters::string_to_utility_action_type;
using EnumConverters::curve_type_to_string;
using EnumConverters::string_to_curve_type;
using EnumConverters::input_type_to_string;
using EnumConverters::string_to_input_type;

// ==================== Component Serialization Functions ====================

// Serialize PositionComponent
inline json::Object serialize_position(const PositionComponent& comp) {
    json::Object obj;
    obj["_type"] = "position";
    obj["x"] = comp.x;
    obj["y"] = comp.y;
    obj["z"] = comp.z;
    return obj;
}

inline void deserialize_position(PositionComponent& comp, const json::Value& val) {
    comp.x = val["x"].get_int(0);
    comp.y = val["y"].get_int(0);
    comp.z = val["z"].get_int(0);
}

// Serialize RenderComponent
inline json::Object serialize_render(const RenderComponent& comp) {
    json::Object obj;
    obj["_type"] = "render";
    obj["ch"] = std::string(1, comp.ch);
    obj["color"] = comp.color;
    obj["bg_color"] = comp.bg_color;
    obj["priority"] = comp.priority;
    return obj;
}

inline void deserialize_render(RenderComponent& comp, const json::Value& val) {
    std::string ch_str = val["ch"].get_string("?");
    comp.ch = ch_str.empty() ? '?' : ch_str[0];
    comp.color = val["color"].get_string("white");
    comp.bg_color = val["bg_color"].get_string("");
    comp.priority = val["priority"].get_int(0);
}

// Serialize NameComponent
inline json::Object serialize_name(const NameComponent& comp) {
    json::Object obj;
    obj["_type"] = "name";
    obj["name"] = comp.name;
    obj["description"] = comp.description;
    return obj;
}

inline void deserialize_name(NameComponent& comp, const json::Value& val) {
    comp.name = val["name"].get_string("");
    comp.description = val["description"].get_string("");
}

// Serialize StatsComponent
inline json::Object serialize_stats(const StatsComponent& comp) {
    json::Object obj;
    obj["_type"] = "stats";
    obj["level"] = comp.level;
    obj["experience"] = comp.experience;
    obj["max_hp"] = comp.max_hp;
    obj["current_hp"] = comp.current_hp;
    obj["attack"] = comp.attack;
    obj["defense"] = comp.defense;
    obj["attribute_points"] = comp.attribute_points;
    return obj;
}

inline void deserialize_stats(StatsComponent& comp, const json::Value& val) {
    comp.level = val["level"].get_int(1);
    comp.experience = val["experience"].get_int(0);
    comp.max_hp = val["max_hp"].get_int(20);
    comp.current_hp = val["current_hp"].get_int(20);
    comp.attack = val["attack"].get_int(5);
    comp.defense = val["defense"].get_int(5);
    comp.attribute_points = val["attribute_points"].get_int(0);
}

// Serialize CollisionComponent
inline json::Object serialize_collision(const CollisionComponent& comp) {
    json::Object obj;
    obj["_type"] = "collision";
    obj["blocks_movement"] = comp.blocks_movement;
    obj["blocks_sight"] = comp.blocks_sight;
    return obj;
}

inline void deserialize_collision(CollisionComponent& comp, const json::Value& val) {
    comp.blocks_movement = val["blocks_movement"].get_bool(false);
    comp.blocks_sight = val["blocks_sight"].get_bool(false);
}

// Serialize PlayerControllerComponent
inline json::Object serialize_player_controller(const PlayerControllerComponent& comp) {
    json::Object obj;
    obj["_type"] = "player_controller";
    obj["can_move"] = comp.can_move;
    obj["can_attack"] = comp.can_attack;
    return obj;
}

inline void deserialize_player_controller(PlayerControllerComponent& comp, const json::Value& val) {
    comp.can_move = val["can_move"].get_bool(true);
    comp.can_attack = val["can_attack"].get_bool(true);
}

// Serialize InventoryComponent (stores entity IDs - need special handling for reconstruction)
inline json::Object serialize_inventory(const InventoryComponent& comp) {
    json::Object obj;
    obj["_type"] = "inventory";
    obj["max_items"] = comp.max_items;
    obj["gold"] = comp.gold;
    // Note: item entity IDs are serialized separately when manager is available
    // This basic version doesn't include items - use serialize_inventory_with_items for full serialization
    return obj;
}

// Forward declaration for recursive serialization
inline json::Object serialize_entity_components(EntityId entity_id, ComponentManager* manager);

// Serialize InventoryComponent WITH nested item entities
inline json::Object serialize_inventory_with_items(const InventoryComponent& comp, ComponentManager* manager) {
    json::Object obj;
    obj["_type"] = "inventory";
    obj["max_items"] = comp.max_items;
    obj["gold"] = comp.gold;
    
    // Serialize each item as a nested entity
    json::Array items_arr;
    for (EntityId item_id : comp.items) {
        if (item_id != 0 && manager->has_component<ItemComponent>(item_id)) {
            json::Object item_obj = serialize_entity_components(item_id, manager);
            items_arr.push_back(json::Value(std::move(item_obj)));
        }
    }
    obj["items"] = json::Value(std::move(items_arr));
    
    return obj;
}

inline void deserialize_inventory(InventoryComponent& comp, const json::Value& val) {
    comp.max_items = val["max_items"].get_int(20);
    comp.gold = val["gold"].get_int(0);
    // Items list is reconstructed by deserialize_inventory_with_items or the entity load system
}

// Forward declaration (default argument is in the actual definition below)
inline EntityId deserialize_entity_components(const json::Value& val, ComponentManager* manager, 
                                               std::unordered_map<int, EntityId>* id_remap);

// Deserialize inventory with nested item entities
inline void deserialize_inventory_with_items(InventoryComponent& comp, const json::Value& val, 
                                              ComponentManager* manager, 
                                              std::unordered_map<int, EntityId>* id_remap = nullptr) {
    comp.max_items = val["max_items"].get_int(20);
    comp.gold = val["gold"].get_int(0);
    comp.items.clear();
    
    // Deserialize nested item entities
    if (val.has("items") && val["items"].is_array()) {
        const auto& items_arr = val["items"].as_array();
        for (const auto& item_val : items_arr) {
            EntityId item_id = deserialize_entity_components(item_val, manager, id_remap);
            if (item_id != 0) {
                // Remove position component from inventory items
                if (manager->has_component<PositionComponent>(item_id)) {
                    manager->remove_component<PositionComponent>(item_id);
                }
                comp.items.push_back(item_id);
            }
        }
    }
}

// Serialize AnatomyComponent
inline json::Object serialize_anatomy(const AnatomyComponent& comp) {
    json::Object obj;
    obj["_type"] = "anatomy";
    
    json::Array limbs_arr;
    for (const auto& limb : comp.limbs) {
        json::Object limb_obj;
        limb_obj["type"] = limb_type_to_string(limb.type);
        limb_obj["name"] = limb.name;
        limb_obj["functional"] = limb.functional;
        limb_obj["severing_resistance"] = limb.severing_resistance;
        // Note: equipped_item is handled by EquipmentComponent
        limbs_arr.push_back(json::Value(std::move(limb_obj)));
    }
    obj["limbs"] = json::Value(std::move(limbs_arr));
    
    return obj;
}

inline void deserialize_anatomy(AnatomyComponent& comp, const json::Value& val) {
    comp.limbs.clear();
    
    if (val.has("limbs") && val["limbs"].is_array()) {
        const auto& limbs_arr = val["limbs"].as_array();
        for (const auto& limb_val : limbs_arr) {
            LimbType type = string_to_limb_type(limb_val["type"].get_string("other"));
            std::string name = limb_val["name"].get_string("limb");
            float sev_res = static_cast<float>(limb_val["severing_resistance"].get_number(0.5));
            
            Limb limb(type, name, sev_res);
            limb.functional = limb_val["functional"].get_bool(true);
            comp.limbs.push_back(limb);
        }
    }
}

// Serialize EquipmentComponent (basic version - stores IDs for remapping)
inline json::Object serialize_equipment(const EquipmentComponent& comp) {
    json::Object obj;
    obj["_type"] = "equipment";
    
    // Serialize equipped_items map as array of {limb, item_id} pairs
    // The actual items are serialized in the inventory; we store limb->item mappings
    json::Array equipped_arr;
    for (const auto& [limb_name, item_id] : comp.equipped_items) {
        if (item_id != 0) {
            json::Object equip_obj;
            equip_obj["limb"] = limb_name;
            equip_obj["item_id"] = static_cast<int>(item_id);  // Temporary ID for reconstruction
            equipped_arr.push_back(json::Value(std::move(equip_obj)));
        }
    }
    obj["equipped_items"] = json::Value(std::move(equipped_arr));
    
    // General slots
    obj["back"] = static_cast<int>(comp.back);
    obj["neck"] = static_cast<int>(comp.neck);
    obj["waist"] = static_cast<int>(comp.waist);
    
    return obj;
}

// Serialize EquipmentComponent WITH nested equipped item entities
inline json::Object serialize_equipment_with_items(const EquipmentComponent& comp, ComponentManager* manager) {
    json::Object obj;
    obj["_type"] = "equipment";
    
    // Serialize equipped_items with full item data
    json::Array equipped_arr;
    for (const auto& [limb_name, item_id] : comp.equipped_items) {
        if (item_id != 0 && manager->has_component<ItemComponent>(item_id)) {
            json::Object equip_obj;
            equip_obj["limb"] = limb_name;
            equip_obj["item_id"] = static_cast<int>(item_id);
            // Include full item data for reconstruction
            equip_obj["item_data"] = json::Value(serialize_entity_components(item_id, manager));
            equipped_arr.push_back(json::Value(std::move(equip_obj)));
        }
    }
    obj["equipped_items"] = json::Value(std::move(equipped_arr));
    
    // General slots with full item data
    auto serialize_slot = [&](const char* name, EntityId slot_id) {
        obj[name] = static_cast<int>(slot_id);
        if (slot_id != 0 && manager->has_component<ItemComponent>(slot_id)) {
            std::string data_name = std::string(name) + "_data";
            obj[data_name] = json::Value(serialize_entity_components(slot_id, manager));
        }
    };
    serialize_slot("back", comp.back);
    serialize_slot("neck", comp.neck);
    serialize_slot("waist", comp.waist);
    
    return obj;
}

inline void deserialize_equipment(EquipmentComponent& comp, const json::Value& val) {
    comp.equipped_items.clear();
    
    if (val.has("equipped_items") && val["equipped_items"].is_array()) {
        const auto& equipped_arr = val["equipped_items"].as_array();
        for (const auto& equip_val : equipped_arr) {
            std::string limb = equip_val["limb"].get_string("");
            EntityId item_id = static_cast<EntityId>(equip_val["item_id"].get_int(0));
            if (!limb.empty() && item_id != 0) {
                comp.equipped_items[limb] = item_id;
            }
        }
    }
    
    comp.back = static_cast<EntityId>(val["back"].get_int(0));
    comp.neck = static_cast<EntityId>(val["neck"].get_int(0));
    comp.waist = static_cast<EntityId>(val["waist"].get_int(0));
}

// Deserialize equipment with nested item entities
inline void deserialize_equipment_with_items(EquipmentComponent& comp, const json::Value& val,
                                              ComponentManager* manager,
                                              std::unordered_map<int, EntityId>* id_remap = nullptr) {
    comp.equipped_items.clear();
    
    if (val.has("equipped_items") && val["equipped_items"].is_array()) {
        const auto& equipped_arr = val["equipped_items"].as_array();
        for (const auto& equip_val : equipped_arr) {
            std::string limb = equip_val["limb"].get_string("");
            
            // Check if we have full item data
            if (equip_val.has("item_data") && equip_val["item_data"].is_object()) {
                // Recreate the item from full data
                EntityId new_item_id = deserialize_entity_components(equip_val["item_data"], manager, id_remap);
                if (new_item_id != 0 && !limb.empty()) {
                    // Remove position from equipped items
                    if (manager->has_component<PositionComponent>(new_item_id)) {
                        manager->remove_component<PositionComponent>(new_item_id);
                    }
                    comp.equipped_items[limb] = new_item_id;
                }
            } else {
                // Legacy: just use the item_id (will need remapping)
                EntityId item_id = static_cast<EntityId>(equip_val["item_id"].get_int(0));
                if (!limb.empty() && item_id != 0) {
                    comp.equipped_items[limb] = item_id;
                }
            }
        }
    }
    
    // Deserialize general slots
    auto deserialize_slot = [&](const char* name, EntityId& slot) {
        std::string data_name = std::string(name) + "_data";
        if (val.has(data_name) && val[data_name].is_object()) {
            EntityId new_id = deserialize_entity_components(val[data_name], manager, id_remap);
            if (new_id != 0 && manager->has_component<PositionComponent>(new_id)) {
                manager->remove_component<PositionComponent>(new_id);
            }
            slot = new_id;
        } else {
            slot = static_cast<EntityId>(val[name].get_int(0));
        }
    };
    
    deserialize_slot("back", comp.back);
    deserialize_slot("neck", comp.neck);
    deserialize_slot("waist", comp.waist);
}

// Serialize ItemComponent
inline json::Object serialize_item(const ItemComponent& comp) {
    json::Object obj;
    obj["_type"] = "item";
    obj["type"] = item_type_to_string(comp.type);
    obj["equip_slot"] = equip_slot_to_string(comp.equip_slot);
    obj["value"] = comp.value;
    obj["stackable"] = comp.stackable;
    obj["stack_count"] = comp.stack_count;
    obj["armor_bonus"] = comp.armor_bonus;
    obj["attack_bonus"] = comp.attack_bonus;
    obj["base_damage"] = comp.base_damage;
    obj["defense_bonus"] = comp.defense_bonus;
    obj["cutting_chance"] = comp.cutting_chance;
    obj["required_limbs"] = comp.required_limbs;
    obj["required_limb_type"] = limb_type_to_string(comp.required_limb_type);
    return obj;
}

inline void deserialize_item(ItemComponent& comp, const json::Value& val) {
    comp.type = string_to_item_type(val["type"].get_string("misc"));
    comp.equip_slot = string_to_equip_slot(val["equip_slot"].get_string("none"));
    comp.value = val["value"].get_int(0);
    comp.stackable = val["stackable"].get_bool(false);
    comp.stack_count = val["stack_count"].get_int(1);
    comp.armor_bonus = val["armor_bonus"].get_int(0);
    comp.attack_bonus = val["attack_bonus"].get_int(0);
    comp.base_damage = val["base_damage"].get_int(0);
    comp.defense_bonus = val["defense_bonus"].get_int(0);
    comp.cutting_chance = static_cast<float>(val["cutting_chance"].get_number(0.0));
    comp.required_limbs = val["required_limbs"].get_int(1);
    comp.required_limb_type = string_to_limb_type(val["required_limb_type"].get_string("hand"));
}

// Serialize ItemEffectComponent
inline json::Object serialize_item_effect(const ItemEffectComponent& comp) {
    json::Object obj;
    obj["_type"] = "item_effect";
    
    json::Array effects_arr;
    for (const auto& effect : comp.effects) {
        json::Object eff_obj;
        eff_obj["type"] = static_cast<int>(effect.type);
        eff_obj["trigger"] = static_cast<int>(effect.trigger);
        eff_obj["magnitude"] = effect.magnitude;
        eff_obj["duration"] = effect.duration;
        eff_obj["chance"] = effect.chance;
        eff_obj["message"] = effect.message;
        eff_obj["effect_id"] = effect.effect_id;
        effects_arr.push_back(json::Value(std::move(eff_obj)));
    }
    obj["effects"] = json::Value(std::move(effects_arr));
    
    return obj;
}

inline void deserialize_item_effect(ItemEffectComponent& comp, const json::Value& val) {
    comp.effects.clear();
    
    if (val.has("effects") && val["effects"].is_array()) {
        const auto& effects_arr = val["effects"].as_array();
        for (const auto& eff_val : effects_arr) {
            ItemEffectComponent::Effect effect;
            effect.type = static_cast<ItemEffectComponent::EffectType>(eff_val["type"].get_int(0));
            effect.trigger = static_cast<ItemEffectComponent::Trigger>(eff_val["trigger"].get_int(0));
            effect.magnitude = eff_val["magnitude"].get_int(0);
            effect.duration = eff_val["duration"].get_int(0);
            effect.chance = static_cast<float>(eff_val["chance"].get_number(1.0));
            effect.message = eff_val["message"].get_string("");
            effect.effect_id = eff_val["effect_id"].get_string("");
            // custom_callback is restored from EffectRegistry at runtime if effect_id is set
            comp.effects.push_back(effect);
        }
    }
}

// Serialize NPCComponent
inline json::Object serialize_npc(const NPCComponent& comp) {
    json::Object obj;
    obj["_type"] = "npc";
    obj["disposition"] = disposition_to_string(comp.disposition);
    obj["role"] = role_to_string(comp.role);
    obj["can_trade"] = comp.can_trade;
    obj["can_give_quests"] = comp.can_give_quests;
    obj["greeting"] = comp.greeting;
    obj["farewell"] = comp.farewell;
    obj["relationship"] = comp.relationship;
    return obj;
}

inline void deserialize_npc(NPCComponent& comp, const json::Value& val) {
    comp.disposition = string_to_disposition(val["disposition"].get_string("neutral"));
    comp.role = string_to_role(val["role"].get_string("villager"));
    comp.can_trade = val["can_trade"].get_bool(false);
    comp.can_give_quests = val["can_give_quests"].get_bool(false);
    comp.greeting = val["greeting"].get_string("");
    comp.farewell = val["farewell"].get_string("");
    comp.relationship = val["relationship"].get_int(0);
    comp.setup_default_interactions();  // Rebuild interactions based on role
}

// Serialize FactionComponent
inline json::Object serialize_faction(const FactionComponent& comp) {
    json::Object obj;
    obj["_type"] = "faction";
    obj["faction"] = static_cast<int>(comp.faction);
    obj["last_attacker"] = static_cast<int>(comp.last_attacker);
    
    // Serialize reputation modifiers
    json::Array rep_arr;
    for (const auto& [faction_id, modifier] : comp.reputation_modifiers) {
        json::Object rep_obj;
        rep_obj["faction"] = static_cast<int>(faction_id);
        rep_obj["modifier"] = modifier;
        rep_arr.push_back(json::Value(std::move(rep_obj)));
    }
    obj["reputation_modifiers"] = json::Value(std::move(rep_arr));
    
    // Serialize personal enemies/allies
    json::Array enemies_arr;
    for (EntityId id : comp.personal_enemies) {
        enemies_arr.push_back(json::Value(static_cast<int>(id)));
    }
    obj["personal_enemies"] = json::Value(std::move(enemies_arr));
    
    json::Array allies_arr;
    for (EntityId id : comp.personal_allies) {
        allies_arr.push_back(json::Value(static_cast<int>(id)));
    }
    obj["personal_allies"] = json::Value(std::move(allies_arr));
    
    return obj;
}

inline void deserialize_faction(FactionComponent& comp, const json::Value& val) {
    comp.faction = static_cast<FactionId>(val["faction"].get_int(0));
    comp.last_attacker = static_cast<EntityId>(val["last_attacker"].get_int(0));
    
    comp.reputation_modifiers.clear();
    if (val.has("reputation_modifiers") && val["reputation_modifiers"].is_array()) {
        const auto& rep_arr = val["reputation_modifiers"].as_array();
        for (const auto& rep_val : rep_arr) {
            FactionId faction_id = static_cast<FactionId>(rep_val["faction"].get_int(0));
            int modifier = rep_val["modifier"].get_int(0);
            comp.reputation_modifiers[faction_id] = modifier;
        }
    }
    
    comp.personal_enemies.clear();
    if (val.has("personal_enemies") && val["personal_enemies"].is_array()) {
        const auto& enemies_arr = val["personal_enemies"].as_array();
        for (const auto& e : enemies_arr) {
            comp.personal_enemies.push_back(static_cast<EntityId>(e.as_int()));
        }
    }
    
    comp.personal_allies.clear();
    if (val.has("personal_allies") && val["personal_allies"].is_array()) {
        const auto& allies_arr = val["personal_allies"].as_array();
        for (const auto& a : allies_arr) {
            comp.personal_allies.push_back(static_cast<EntityId>(a.as_int()));
        }
    }
}

// Serialize RangedWeaponComponent
inline json::Object serialize_ranged_weapon(const RangedWeaponComponent& comp) {
    json::Object obj;
    obj["_type"] = "ranged_weapon";
    obj["range"] = comp.range;
    obj["min_range"] = comp.min_range;
    obj["ammo_type"] = ammo_type_to_string(comp.ammo_type);
    obj["consumes_self"] = comp.consumes_self;
    obj["accuracy_bonus"] = comp.accuracy_bonus;
    obj["projectile_char"] = std::string(1, comp.projectile_char);
    obj["projectile_color"] = comp.projectile_color;
    return obj;
}

inline void deserialize_ranged_weapon(RangedWeaponComponent& comp, const json::Value& val) {
    comp.range = val["range"].get_int(5);
    comp.min_range = val["min_range"].get_int(1);
    comp.ammo_type = string_to_ammo_type(val["ammo_type"].get_string("none"));
    comp.consumes_self = val["consumes_self"].get_bool(false);
    comp.accuracy_bonus = val["accuracy_bonus"].get_int(0);
    std::string pc = val["projectile_char"].get_string("*");
    comp.projectile_char = pc.empty() ? '*' : pc[0];
    comp.projectile_color = val["projectile_color"].get_string("white");
}

// Serialize AmmoComponent
inline json::Object serialize_ammo(const AmmoComponent& comp) {
    json::Object obj;
    obj["_type"] = "ammo";
    obj["ammo_type"] = ammo_type_to_string(comp.ammo_type);
    obj["quantity"] = comp.quantity;
    obj["damage_bonus"] = comp.damage_bonus;
    return obj;
}

inline void deserialize_ammo(AmmoComponent& comp, const json::Value& val) {
    comp.ammo_type = string_to_ammo_type(val["ammo_type"].get_string("arrow"));
    comp.quantity = val["quantity"].get_int(1);
    comp.damage_bonus = val["damage_bonus"].get_int(0);
}

// Serialize DoorComponent
inline json::Object serialize_door(const DoorComponent& comp) {
    json::Object obj;
    obj["_type"] = "door";
    obj["is_open"] = comp.is_open;
    obj["closed_char"] = std::string(1, comp.closed_char);
    obj["open_char"] = std::string(1, comp.open_char);
    obj["closed_color"] = comp.closed_color;
    obj["open_color"] = comp.open_color;
    return obj;
}

inline void deserialize_door(DoorComponent& comp, const json::Value& val) {
    comp.is_open = val["is_open"].get_bool(false);
    std::string cc = val["closed_char"].get_string("+");
    comp.closed_char = cc.empty() ? '+' : cc[0];
    std::string oc = val["open_char"].get_string("-");
    comp.open_char = oc.empty() ? '-' : oc[0];
    comp.closed_color = val["closed_color"].get_string("brown");
    comp.open_color = val["open_color"].get_string("gray");
}

// Serialize ChestComponent
inline json::Object serialize_chest(const ChestComponent& comp) {
    json::Object obj;
    obj["_type"] = "chest";
    obj["is_open"] = comp.is_open;
    obj["closed_char"] = std::string(1, comp.closed_char);
    obj["open_char"] = std::string(1, comp.open_char);
    obj["closed_color"] = comp.closed_color;
    obj["open_color"] = comp.open_color;
    // Note: items are handled by the entity system, chest just stores IDs
    return obj;
}

inline void deserialize_chest(ChestComponent& comp, const json::Value& val) {
    comp.is_open = val["is_open"].get_bool(false);
    std::string cc = val["closed_char"].get_string("=");
    comp.closed_char = cc.empty() ? '=' : cc[0];
    std::string oc = val["open_char"].get_string("u");
    comp.open_char = oc.empty() ? 'u' : oc[0];
    comp.closed_color = val["closed_color"].get_string("yellow");
    comp.open_color = val["open_color"].get_string("dark_yellow");
}

// Serialize InteractableComponent
inline json::Object serialize_interactable(const InteractableComponent& comp) {
    json::Object obj;
    obj["_type"] = "interactable";
    obj["interaction_message"] = comp.interaction_message;
    return obj;
}

inline void deserialize_interactable(InteractableComponent& comp, const json::Value& val) {
    comp.interaction_message = val["interaction_message"].get_string("");
}

// Serialize HomeComponent
inline json::Object serialize_home(const HomeComponent& comp) {
    json::Object obj;
    obj["_type"] = "home";
    obj["home_x"] = comp.home_x;
    obj["home_y"] = comp.home_y;
    obj["bed_x"] = comp.bed_x;
    obj["bed_y"] = comp.bed_y;
    obj["is_home"] = comp.is_home;
    obj["is_sleeping"] = comp.is_sleeping;
    obj["home_structure_id"] = comp.home_structure_id;
    return obj;
}

inline void deserialize_home(HomeComponent& comp, const json::Value& val) {
    comp.home_x = val["home_x"].get_int(0);
    comp.home_y = val["home_y"].get_int(0);
    comp.bed_x = val["bed_x"].get_int(0);
    comp.bed_y = val["bed_y"].get_int(0);
    comp.is_home = val["is_home"].get_bool(false);
    comp.is_sleeping = val["is_sleeping"].get_bool(false);
    comp.home_structure_id = val["home_structure_id"].get_string("");
}

// Serialize ShopItem
inline json::Object serialize_shop_item(const ShopItem& item) {
    json::Object obj;
    obj["name"] = item.name;
    obj["description"] = item.description;
    obj["symbol"] = std::string(1, item.symbol);
    obj["color"] = item.color;
    obj["buy_price"] = item.buy_price;
    obj["sell_price"] = item.sell_price;
    obj["stock"] = item.stock;
    obj["available"] = item.available;
    obj["item_type"] = item_type_to_string(item.item_type);
    obj["equip_slot"] = equip_slot_to_string(item.equip_slot);
    obj["attack_bonus"] = item.attack_bonus;
    obj["defense_bonus"] = item.defense_bonus;
    obj["armor_bonus"] = item.armor_bonus;
    obj["is_consumable"] = item.is_consumable;
    obj["is_ranged"] = item.is_ranged;
    obj["range"] = item.range;
    obj["min_range"] = item.min_range;
    obj["ammo_type"] = static_cast<int>(item.ammo_type);
    obj["consumes_self"] = item.consumes_self;
    obj["accuracy_bonus"] = item.accuracy_bonus;
    obj["is_ammo"] = item.is_ammo;
    obj["ammo_quantity"] = item.ammo_quantity;
    obj["ammo_damage_bonus"] = item.ammo_damage_bonus;
    
    // Serialize effects for consumables
    if (item.is_consumable && !item.effects.empty()) {
        json::Array effects_arr;
        for (const auto& effect_comp : item.effects) {
            json::Object effect_obj = serialize_item_effect(effect_comp);
            effects_arr.push_back(json::Value(std::move(effect_obj)));
        }
        obj["effects"] = json::Value(std::move(effects_arr));
    }
    
    return obj;
}

inline ShopItem deserialize_shop_item(const json::Value& val) {
    ShopItem item;
    item.name = val["name"].get_string("");
    item.description = val["description"].get_string("");
    std::string sym = val["symbol"].get_string("?");
    item.symbol = sym.empty() ? '?' : sym[0];
    item.color = val["color"].get_string("white");
    item.buy_price = val["buy_price"].get_int(0);
    item.sell_price = val["sell_price"].get_int(0);
    item.stock = val["stock"].get_int(-1);
    item.available = val["available"].get_bool(true);
    item.item_type = string_to_item_type(val["item_type"].get_string("misc"));
    item.equip_slot = string_to_equip_slot(val["equip_slot"].get_string("none"));
    item.attack_bonus = val["attack_bonus"].get_int(0);
    item.defense_bonus = val["defense_bonus"].get_int(0);
    item.armor_bonus = val["armor_bonus"].get_int(0);
    item.is_consumable = val["is_consumable"].get_bool(false);
    item.is_ranged = val["is_ranged"].get_bool(false);
    item.range = val["range"].get_int(0);
    item.min_range = val["min_range"].get_int(1);
    item.ammo_type = static_cast<RangedWeaponComponent::AmmoType>(val["ammo_type"].get_int(0));
    item.consumes_self = val["consumes_self"].get_bool(false);
    item.accuracy_bonus = val["accuracy_bonus"].get_int(0);
    item.is_ammo = val["is_ammo"].get_bool(false);
    item.ammo_quantity = val["ammo_quantity"].get_int(0);
    item.ammo_damage_bonus = val["ammo_damage_bonus"].get_int(0);
    
    // Deserialize effects for consumables
    if (val.has("effects") && val["effects"].is_array()) {
        const auto& effects_arr = val["effects"].as_array();
        for (const auto& effect_val : effects_arr) {
            ItemEffectComponent effect_comp;
            deserialize_item_effect(effect_comp, effect_val);
            item.effects.push_back(effect_comp);
        }
    }
    
    return item;
}

// Serialize ShopInventoryComponent
inline json::Object serialize_shop_inventory(const ShopInventoryComponent& comp) {
    json::Object obj;
    obj["_type"] = "shop_inventory";
    obj["buy_multiplier"] = comp.buy_multiplier;
    obj["sell_multiplier"] = comp.sell_multiplier;
    obj["restocks"] = comp.restocks;
    obj["restock_interval"] = comp.restock_interval;
    obj["turns_since_restock"] = comp.turns_since_restock;
    
    json::Array items_arr;
    for (const auto& item : comp.items) {
        items_arr.push_back(json::Value(serialize_shop_item(item)));
    }
    obj["items"] = json::Value(std::move(items_arr));
    
    return obj;
}

inline void deserialize_shop_inventory(ShopInventoryComponent& comp, const json::Value& val) {
    comp.buy_multiplier = static_cast<float>(val["buy_multiplier"].get_number(1.0));
    comp.sell_multiplier = static_cast<float>(val["sell_multiplier"].get_number(0.5));
    comp.restocks = val["restocks"].get_bool(true);
    comp.restock_interval = val["restock_interval"].get_int(100);
    comp.turns_since_restock = val["turns_since_restock"].get_int(0);
    
    comp.items.clear();
    if (val.has("items") && val["items"].is_array()) {
        const auto& items_arr = val["items"].as_array();
        for (const auto& item_val : items_arr) {
            comp.items.push_back(deserialize_shop_item(item_val));
        }
    }
}

// Serialize DialogueOption
inline json::Object serialize_dialogue_option(const DialogueOption& opt) {
    json::Object obj;
    obj["text"] = opt.text;
    obj["next_node_id"] = opt.next_node_id;
    obj["action"] = opt.action;
    obj["condition"] = opt.condition;
    obj["ends_dialogue"] = opt.ends_dialogue;
    return obj;
}

inline DialogueOption deserialize_dialogue_option(const json::Value& val) {
    DialogueOption opt;
    opt.text = val["text"].get_string("");
    opt.next_node_id = val["next_node_id"].get_int(-1);
    opt.action = val["action"].get_string("");
    opt.condition = val["condition"].get_string("");
    opt.ends_dialogue = val["ends_dialogue"].get_bool(false);
    return opt;
}

// Serialize DialogueNode
inline json::Object serialize_dialogue_node(const DialogueNode& node) {
    json::Object obj;
    obj["id"] = node.id;
    obj["speaker_name"] = node.speaker_name;
    obj["text"] = node.text;
    
    json::Array options_arr;
    for (const auto& opt : node.options) {
        options_arr.push_back(json::Value(serialize_dialogue_option(opt)));
    }
    obj["options"] = json::Value(std::move(options_arr));
    
    return obj;
}

inline DialogueNode deserialize_dialogue_node(const json::Value& val) {
    DialogueNode node;
    node.id = val["id"].get_int(0);
    node.speaker_name = val["speaker_name"].get_string("");
    node.text = val["text"].get_string("");
    
    if (val.has("options") && val["options"].is_array()) {
        const auto& options_arr = val["options"].as_array();
        for (const auto& opt_val : options_arr) {
            node.options.push_back(deserialize_dialogue_option(opt_val));
        }
    }
    
    return node;
}

// Serialize DialogueComponent
inline json::Object serialize_dialogue(const DialogueComponent& comp) {
    json::Object obj;
    obj["_type"] = "dialogue";
    obj["start_node_id"] = comp.start_node_id;
    obj["current_node_id"] = comp.current_node_id;
    obj["has_talked"] = comp.has_talked;
    
    json::Array nodes_arr;
    for (const auto& node : comp.nodes) {
        nodes_arr.push_back(json::Value(serialize_dialogue_node(node)));
    }
    obj["nodes"] = json::Value(std::move(nodes_arr));
    
    return obj;
}

inline void deserialize_dialogue(DialogueComponent& comp, const json::Value& val) {
    comp.start_node_id = val["start_node_id"].get_int(0);
    comp.current_node_id = val["current_node_id"].get_int(0);
    comp.has_talked = val["has_talked"].get_bool(false);
    
    comp.nodes.clear();
    if (val.has("nodes") && val["nodes"].is_array()) {
        const auto& nodes_arr = val["nodes"].as_array();
        for (const auto& node_val : nodes_arr) {
            comp.nodes.push_back(deserialize_dialogue_node(node_val));
        }
    }
}

// Serialize ResponseCurve
inline json::Object serialize_response_curve(const ResponseCurve& curve) {
    json::Object obj;
    obj["curve_type"] = curve_type_to_string(curve.type);
    obj["slope"] = curve.slope;
    obj["exponent"] = curve.exponent;
    obj["x_shift"] = curve.x_shift;
    obj["y_shift"] = curve.y_shift;
    obj["threshold"] = curve.threshold;
    obj["clamp"] = curve.clamp;
    return obj;
}

inline ResponseCurve deserialize_response_curve(const json::Value& val) {
    ResponseCurve curve;
    curve.type = string_to_curve_type(val["curve_type"].get_string("linear"));
    curve.slope = static_cast<float>(val["slope"].get_number(1.0));
    curve.exponent = static_cast<float>(val["exponent"].get_number(1.0));
    curve.x_shift = static_cast<float>(val["x_shift"].get_number(0.0));
    curve.y_shift = static_cast<float>(val["y_shift"].get_number(0.0));
    curve.threshold = static_cast<float>(val["threshold"].get_number(0.5));
    curve.clamp = val["clamp"].get_bool(true);
    return curve;
}

// Serialize Consideration
inline json::Object serialize_consideration(const Consideration& cons) {
    json::Object obj;
    obj["name"] = cons.name;
    obj["input_type"] = input_type_to_string(cons.input_type);
    obj["curve"] = json::Value(serialize_response_curve(cons.curve));
    obj["weight"] = cons.weight;
    obj["bonus"] = cons.bonus;
    return obj;
}

inline Consideration deserialize_consideration(const json::Value& val) {
    Consideration cons;
    cons.name = val["name"].get_string("");
    cons.input_type = string_to_input_type(val["input_type"].get_string("health_percent"));
    if (val.has("curve")) {
        cons.curve = deserialize_response_curve(val["curve"]);
    }
    cons.weight = static_cast<float>(val["weight"].get_number(1.0));
    cons.bonus = static_cast<float>(val["bonus"].get_number(0.0));
    return cons;
}

// Serialize UtilityAction
inline json::Object serialize_utility_action(const UtilityAction& action) {
    json::Object obj;
    obj["name"] = action.name;
    obj["type"] = utility_action_type_to_string(action.type);
    obj["base_score"] = action.base_score;
    obj["score_multiplier"] = action.score_multiplier;
    obj["momentum_bonus"] = action.momentum_bonus;
    obj["target_id"] = static_cast<int>(action.target_id);
    obj["cooldown_turns"] = action.cooldown_turns;
    obj["current_cooldown"] = action.current_cooldown;
    
    json::Array considerations_arr;
    for (const auto& cons : action.considerations) {
        considerations_arr.push_back(json::Value(serialize_consideration(cons)));
    }
    obj["considerations"] = json::Value(std::move(considerations_arr));
    
    return obj;
}

inline UtilityAction deserialize_utility_action(const json::Value& val) {
    UtilityAction action;
    action.name = val["name"].get_string("");
    action.type = string_to_utility_action_type(val["type"].get_string("idle"));
    action.base_score = static_cast<float>(val["base_score"].get_number(0.0));
    action.score_multiplier = static_cast<float>(val["score_multiplier"].get_number(1.0));
    action.momentum_bonus = static_cast<float>(val["momentum_bonus"].get_number(0.0));
    action.target_id = static_cast<EntityId>(val["target_id"].get_int(0));
    action.cooldown_turns = val["cooldown_turns"].get_int(0);
    action.current_cooldown = val["current_cooldown"].get_int(0);
    
    if (val.has("considerations") && val["considerations"].is_array()) {
        const auto& cons_arr = val["considerations"].as_array();
        for (const auto& cons_val : cons_arr) {
            action.considerations.push_back(deserialize_consideration(cons_val));
        }
    }
    
    return action;
}

// Serialize UtilityAIComponent
inline json::Object serialize_utility_ai(const UtilityAIComponent& comp) {
    json::Object obj;
    obj["_type"] = "utility_ai";
    obj["score_threshold"] = comp.score_threshold;
    obj["randomness"] = comp.randomness;
    obj["use_momentum"] = comp.use_momentum;
    obj["momentum_value"] = comp.momentum_value;
    obj["last_action"] = utility_action_type_to_string(comp.last_action);
    obj["action_streak"] = comp.action_streak;
    obj["aggro_range"] = comp.aggro_range;
    obj["ally_range"] = comp.ally_range;
    
    json::Array actions_arr;
    for (const auto& action : comp.actions) {
        actions_arr.push_back(json::Value(serialize_utility_action(action)));
    }
    obj["actions"] = json::Value(std::move(actions_arr));
    
    return obj;
}

inline void deserialize_utility_ai(UtilityAIComponent& comp, const json::Value& val) {
    comp.score_threshold = static_cast<float>(val["score_threshold"].get_number(0.1));
    comp.randomness = static_cast<float>(val["randomness"].get_number(0.0));
    comp.use_momentum = val["use_momentum"].get_bool(true);
    comp.momentum_value = static_cast<float>(val["momentum_value"].get_number(0.1));
    comp.last_action = string_to_utility_action_type(val["last_action"].get_string("idle"));
    comp.action_streak = val["action_streak"].get_int(0);
    comp.aggro_range = static_cast<float>(val["aggro_range"].get_number(8.0));
    comp.ally_range = static_cast<float>(val["ally_range"].get_number(10.0));
    
    comp.actions.clear();
    if (val.has("actions") && val["actions"].is_array()) {
        const auto& actions_arr = val["actions"].as_array();
        for (const auto& action_val : actions_arr) {
            comp.actions.push_back(deserialize_utility_action(action_val));
        }
    }
}

// Serialize PropertiesComponent
inline json::Object serialize_properties(const PropertiesComponent& comp) {
    json::Object obj;
    obj["_type"] = "properties";
    json::Array arr;
    for (const auto& prop : comp.properties) {
        arr.push_back(json::Value(prop));
    }
    obj["properties"] = json::Value(std::move(arr));
    return obj;
}

inline void deserialize_properties(PropertiesComponent& comp, const json::Value& val) {
    comp.properties.clear();
    if (val.has("properties") && val["properties"].is_array()) {
        const auto& arr = val["properties"].as_array();
        for (const auto& v : arr) {
            comp.properties.push_back(v.get_string(""));
        }
    }
}

// ==================== Full Entity Serialization ====================

// Serialize all components of an entity to a JSON object
inline json::Object serialize_entity_components(EntityId entity_id, ComponentManager* manager) {
    json::Object obj;
    json::Array components_arr;
    
    // Serialize each component type if present
    if (auto* comp = manager->get_component<PositionComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_position(*comp)));
    }
    if (auto* comp = manager->get_component<RenderComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_render(*comp)));
    }
    if (auto* comp = manager->get_component<NameComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_name(*comp)));
    }
    if (auto* comp = manager->get_component<StatsComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_stats(*comp)));
    }
    if (auto* comp = manager->get_component<CollisionComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_collision(*comp)));
    }
    if (auto* comp = manager->get_component<InventoryComponent>(entity_id)) {
        // Use the version that serializes nested item entities
        components_arr.push_back(json::Value(serialize_inventory_with_items(*comp, manager)));
    }
    if (auto* comp = manager->get_component<AnatomyComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_anatomy(*comp)));
    }
    if (auto* comp = manager->get_component<EquipmentComponent>(entity_id)) {
        // Use the version that serializes nested equipped item entities
        components_arr.push_back(json::Value(serialize_equipment_with_items(*comp, manager)));
    }
    if (auto* comp = manager->get_component<ItemComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_item(*comp)));
    }
    if (auto* comp = manager->get_component<ItemEffectComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_item_effect(*comp)));
    }
    if (auto* comp = manager->get_component<NPCComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_npc(*comp)));
    }
    if (auto* comp = manager->get_component<FactionComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_faction(*comp)));
    }
    if (auto* comp = manager->get_component<RangedWeaponComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_ranged_weapon(*comp)));
    }
    if (auto* comp = manager->get_component<AmmoComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_ammo(*comp)));
    }
    if (auto* comp = manager->get_component<DoorComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_door(*comp)));
    }
    if (auto* comp = manager->get_component<ChestComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_chest(*comp)));
    }
    if (auto* comp = manager->get_component<InteractableComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_interactable(*comp)));
    }
    if (auto* comp = manager->get_component<HomeComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_home(*comp)));
    }
    if (auto* comp = manager->get_component<ShopInventoryComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_shop_inventory(*comp)));
    }
    if (auto* comp = manager->get_component<DialogueComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_dialogue(*comp)));
    }
    if (auto* comp = manager->get_component<UtilityAIComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_utility_ai(*comp)));
    }
    if (auto* comp = manager->get_component<PlayerControllerComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_player_controller(*comp)));
    }
    if (auto* comp = manager->get_component<PropertiesComponent>(entity_id)) {
        components_arr.push_back(json::Value(serialize_properties(*comp)));
    }
    
    obj["entity_id"] = static_cast<int>(entity_id);
    obj["components"] = json::Value(std::move(components_arr));
    
    return obj;
}

// Deserialize components from JSON and add them to a new entity
inline EntityId deserialize_entity_components(const json::Value& val, ComponentManager* manager, 
                                               std::unordered_map<int, EntityId>* id_remap = nullptr) {
    EntityId entity_id = manager->create_entity();
    
    // Store ID mapping for cross-references (equipped items, inventory, etc.)
    if (id_remap) {
        int old_id = val["entity_id"].get_int(0);
        (*id_remap)[old_id] = entity_id;
    }
    
    if (!val.has("components") || !val["components"].is_array()) {
        return entity_id;
    }
    
    const auto& components_arr = val["components"].as_array();
    for (const auto& comp_val : components_arr) {
        std::string type = comp_val["_type"].get_string("");
        
        if (type == "position") {
            auto& comp = manager->add_component<PositionComponent>(entity_id);
            deserialize_position(comp, comp_val);
        }
        else if (type == "render") {
            auto& comp = manager->add_component<RenderComponent>(entity_id);
            deserialize_render(comp, comp_val);
        }
        else if (type == "name") {
            auto& comp = manager->add_component<NameComponent>(entity_id);
            deserialize_name(comp, comp_val);
        }
        else if (type == "stats") {
            auto& comp = manager->add_component<StatsComponent>(entity_id);
            deserialize_stats(comp, comp_val);
        }
        else if (type == "collision") {
            auto& comp = manager->add_component<CollisionComponent>(entity_id);
            deserialize_collision(comp, comp_val);
        }
        else if (type == "inventory") {
            auto& comp = manager->add_component<InventoryComponent>(entity_id);
            // Use the version that deserializes nested item entities
            deserialize_inventory_with_items(comp, comp_val, manager, id_remap);
        }
        else if (type == "anatomy") {
            auto& comp = manager->add_component<AnatomyComponent>(entity_id);
            deserialize_anatomy(comp, comp_val);
        }
        else if (type == "equipment") {
            auto& comp = manager->add_component<EquipmentComponent>(entity_id);
            // Use the version that deserializes nested equipped item entities
            deserialize_equipment_with_items(comp, comp_val, manager, id_remap);
        }
        else if (type == "item") {
            auto& comp = manager->add_component<ItemComponent>(entity_id);
            deserialize_item(comp, comp_val);
        }
        else if (type == "item_effect") {
            auto& comp = manager->add_component<ItemEffectComponent>(entity_id);
            deserialize_item_effect(comp, comp_val);
        }
        else if (type == "npc") {
            auto& comp = manager->add_component<NPCComponent>(entity_id);
            deserialize_npc(comp, comp_val);
        }
        else if (type == "faction") {
            auto& comp = manager->add_component<FactionComponent>(entity_id);
            deserialize_faction(comp, comp_val);
        }
        else if (type == "ranged_weapon") {
            auto& comp = manager->add_component<RangedWeaponComponent>(entity_id);
            deserialize_ranged_weapon(comp, comp_val);
        }
        else if (type == "ammo") {
            auto& comp = manager->add_component<AmmoComponent>(entity_id);
            deserialize_ammo(comp, comp_val);
        }
        else if (type == "door") {
            auto& comp = manager->add_component<DoorComponent>(entity_id);
            deserialize_door(comp, comp_val);
        }
        else if (type == "chest") {
            auto& comp = manager->add_component<ChestComponent>(entity_id);
            deserialize_chest(comp, comp_val);
        }
        else if (type == "interactable") {
            auto& comp = manager->add_component<InteractableComponent>(entity_id);
            deserialize_interactable(comp, comp_val);
        }
        else if (type == "home") {
            auto& comp = manager->add_component<HomeComponent>(entity_id);
            deserialize_home(comp, comp_val);
        }
        else if (type == "shop_inventory") {
            auto& comp = manager->add_component<ShopInventoryComponent>(entity_id);
            deserialize_shop_inventory(comp, comp_val);
        }
        else if (type == "dialogue") {
            auto& comp = manager->add_component<DialogueComponent>(entity_id);
            deserialize_dialogue(comp, comp_val);
        }
        else if (type == "utility_ai") {
            auto& comp = manager->add_component<UtilityAIComponent>(entity_id);
            deserialize_utility_ai(comp, comp_val);
        }
        else if (type == "player_controller") {
            auto& comp = manager->add_component<PlayerControllerComponent>(entity_id);
            deserialize_player_controller(comp, comp_val);
        }
        else if (type == "properties") {
            auto& comp = manager->add_component<PropertiesComponent>(entity_id);
            deserialize_properties(comp, comp_val);
        }
    }
    
    return entity_id;
}

// Remap entity IDs in Equipment and Inventory components after all entities are loaded
inline void remap_entity_references(ComponentManager* manager, 
                                     const std::unordered_map<int, EntityId>& id_remap) {
    // Remap Equipment components
    auto equipment_entities = manager->get_entities_with_component<EquipmentComponent>();
    for (EntityId eid : equipment_entities) {
        auto* equip = manager->get_component<EquipmentComponent>(eid);
        if (!equip) continue;
        
        // Remap equipped_items
        std::unordered_map<std::string, EntityId> new_equipped;
        for (const auto& [limb, old_id] : equip->equipped_items) {
            auto it = id_remap.find(static_cast<int>(old_id));
            if (it != id_remap.end()) {
                // Found in remap - use new ID
                new_equipped[limb] = it->second;
            } else {
                // Not in remap - keep original (already correct)
                new_equipped[limb] = old_id;
            }
        }
        equip->equipped_items = std::move(new_equipped);
        
        // Remap general slots
        auto remap_slot = [&id_remap](EntityId& slot) {
            auto it = id_remap.find(static_cast<int>(slot));
            if (it != id_remap.end()) {
                slot = it->second;
            }
        };
        remap_slot(equip->back);
        remap_slot(equip->neck);
        remap_slot(equip->waist);
    }
    
    // Remap Inventory components
    auto inventory_entities = manager->get_entities_with_component<InventoryComponent>();
    for (EntityId eid : inventory_entities) {
        auto* inv = manager->get_component<InventoryComponent>(eid);
        if (!inv) continue;
        
        std::vector<EntityId> new_items;
        for (EntityId old_id : inv->items) {
            auto it = id_remap.find(static_cast<int>(old_id));
            if (it != id_remap.end()) {
                // Found in remap - use new ID
                new_items.push_back(it->second);
            } else {
                // Not in remap - keep original (already correct)
                new_items.push_back(old_id);
            }
        }
        inv->items = std::move(new_items);
    }
    
    // Remap Faction components (personal enemies/allies)
    auto faction_entities = manager->get_entities_with_component<FactionComponent>();
    for (EntityId eid : faction_entities) {
        auto* faction = manager->get_component<FactionComponent>(eid);
        if (!faction) continue;
        
        // Remap last_attacker
        auto it = id_remap.find(static_cast<int>(faction->last_attacker));
        if (it != id_remap.end()) {
            faction->last_attacker = it->second;
        }
        
        // Remap personal enemies
        std::vector<EntityId> new_enemies;
        for (EntityId old_id : faction->personal_enemies) {
            auto it2 = id_remap.find(static_cast<int>(old_id));
            if (it2 != id_remap.end()) {
                new_enemies.push_back(it2->second);
            } else {
                new_enemies.push_back(old_id);
            }
        }
        faction->personal_enemies = std::move(new_enemies);
        
        // Remap personal allies
        std::vector<EntityId> new_allies;
        for (EntityId old_id : faction->personal_allies) {
            auto it2 = id_remap.find(static_cast<int>(old_id));
            if (it2 != id_remap.end()) {
                new_allies.push_back(it2->second);
            } else {
                new_allies.push_back(old_id);
            }
        }
        faction->personal_allies = std::move(new_allies);
    }
    
    // Remap Chest components
    auto chest_entities = manager->get_entities_with_component<ChestComponent>();
    for (EntityId eid : chest_entities) {
        auto* chest = manager->get_component<ChestComponent>(eid);
        if (!chest) continue;
        
        std::vector<EntityId> new_items;
        for (EntityId old_id : chest->items) {
            auto it = id_remap.find(static_cast<int>(old_id));
            if (it != id_remap.end()) {
                new_items.push_back(it->second);
            } else {
                new_items.push_back(old_id);
            }
        }
        chest->items = std::move(new_items);
    }
}

} // namespace ComponentSerializer
