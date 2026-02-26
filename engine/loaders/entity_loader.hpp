#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/serialization/enum_converters.hpp"
#include "../util/json.hpp"
#include "../systems/utility_ai.hpp"
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <functional>

// Entity template definition loaded from JSON
struct EntityTemplate {
    std::string id;
    std::string name;
    std::string description;
    std::string category;  // "enemy", "npc", "item", "structure", etc.
    
    // Component data stored as JSON for lazy parsing
    json::Object components;
    
    // Tags for filtering/searching
    std::vector<std::string> tags;
    
    bool valid = false;
};

// Entity Loader - loads entity templates from JSON files
class EntityLoader {
private:
    std::map<std::string, EntityTemplate> templates;
    std::string entities_path = "assets/entities";
    
    // Parse utility AI preset string
    static void apply_utility_preset(UtilityAIComponent& ai, const std::string& preset) {
        if (preset == "aggressive_melee") UtilityPresets::setup_aggressive_melee(ai);
        else if (preset == "defensive_guard") UtilityPresets::setup_defensive_guard(ai);
        else if (preset == "cowardly") UtilityPresets::setup_cowardly(ai);
        else if (preset == "ambush_predator") UtilityPresets::setup_ambush_predator(ai);
        else if (preset == "pack_hunter") UtilityPresets::setup_pack_hunter(ai);
        else if (preset == "predator") UtilityPresets::setup_predator(ai);
        else if (preset == "wildlife") UtilityPresets::setup_wildlife(ai);
        else if (preset == "undead") UtilityPresets::setup_undead(ai);
        else if (preset == "berserker") UtilityPresets::setup_berserker(ai);
        else if (preset == "boss") UtilityPresets::setup_boss(ai);
        else if (preset == "defensive_villager") UtilityPresets::setup_defensive_villager(ai);
        else if (preset == "peaceful_villager") UtilityPresets::setup_peaceful_villager(ai);
        else if (preset == "villager_with_home") UtilityPresets::setup_villager_with_home(ai);
        else if (preset == "town_guard") UtilityPresets::setup_town_guard(ai);
        else if (preset == "merchant") UtilityPresets::setup_merchant(ai);
        else if (preset == "wanderer") UtilityPresets::setup_wanderer(ai);
    }
    
    // Component parsers
    void parse_position_component(Entity& entity, const json::Value& data, int override_x, int override_y) {
        PositionComponent pos;
        pos.x = (override_x != -1) ? override_x : data["x"].get_int(0);
        pos.y = (override_y != -1) ? override_y : data["y"].get_int(0);
        pos.z = data["z"].get_int(0);
        entity.add_component(pos);
    }
    
    void parse_render_component(Entity& entity, const json::Value& data) {
        RenderComponent render;
        std::string ch_str = data["char"].get_string("?");
        render.ch = ch_str.empty() ? '?' : ch_str[0];
        render.color = data["color"].get_string("white");
        render.bg_color = data["bg_color"].get_string("");
        render.priority = data["priority"].get_int(5);
        entity.add_component(render);
    }
    
    void parse_stats_component(Entity& entity, const json::Value& data, int level_override) {
        StatsComponent stats;
        int level = (level_override > 0) ? level_override : data["level"].get_int(1);
        stats.level = level;
        stats.experience = data["experience"].get_int(0);
        
        // Support level scaling
        int hp_base = data["hp_base"].get_int(data["max_hp"].get_int(20));
        int hp_per_level = data["hp_per_level"].get_int(0);
        stats.max_hp = hp_base + (hp_per_level * (level - 1));
        stats.current_hp = data["current_hp"].get_int(stats.max_hp);
        
        int attack_base = data["attack_base"].get_int(data["attack"].get_int(5));
        int attack_per_level = data["attack_per_level"].get_int(0);
        stats.attack = attack_base + (attack_per_level * (level - 1));
        
        int defense_base = data["defense_base"].get_int(data["defense"].get_int(3));
        int defense_per_level = data["defense_per_level"].get_int(0);
        stats.defense = defense_base + (defense_per_level * (level - 1));
        
        stats.attribute_points = data["attribute_points"].get_int(0);
        entity.add_component(stats);
    }
    
    void parse_name_component(Entity& entity, const json::Value& data, const std::string& override_name) {
        NameComponent name;
        name.name = override_name.empty() ? data["name"].get_string("Unknown") : override_name;
        name.description = data["description"].get_string("");
        entity.add_component(name);
    }
    
    void parse_collision_component(Entity& entity, const json::Value& data) {
        CollisionComponent collision;
        collision.blocks_movement = data["blocks_movement"].get_bool(true);
        collision.blocks_sight = data["blocks_sight"].get_bool(false);
        entity.add_component(collision);
    }
    
    void parse_faction_component(Entity& entity, const json::Value& data) {
        FactionComponent faction;
        faction.faction = EnumConverters::string_to_faction(data["faction"].get_string("none"));
        entity.add_component(faction);
    }
    
    void parse_inventory_component(Entity& entity, const json::Value& data) {
        InventoryComponent inv;
        inv.max_items = data["max_items"].get_int(20);
        inv.gold = data["gold"].get_int(0);
        entity.add_component(inv);
    }
    
    void parse_anatomy_component(Entity& entity, const json::Value& data) {
        AnatomyComponent anatomy;
        
        // Check for preset anatomy types
        std::string preset = data["preset"].get_string("");
        if (preset == "humanoid") {
            anatomy.add_limb(LimbType::HEAD, "head");
            anatomy.add_limb(LimbType::TORSO, "torso");
            anatomy.add_limb(LimbType::ARM, "left_arm");
            anatomy.add_limb(LimbType::ARM, "right_arm");
            anatomy.add_limb(LimbType::HAND, "left_hand");
            anatomy.add_limb(LimbType::HAND, "right_hand");
            anatomy.add_limb(LimbType::LEG, "left_leg");
            anatomy.add_limb(LimbType::LEG, "right_leg");
            anatomy.add_limb(LimbType::FOOT, "left_foot");
            anatomy.add_limb(LimbType::FOOT, "right_foot");
        } else if (preset == "quadruped") {
            anatomy.add_limb(LimbType::HEAD, "head");
            anatomy.add_limb(LimbType::TORSO, "torso");
            anatomy.add_limb(LimbType::LEG, "front_left_leg");
            anatomy.add_limb(LimbType::LEG, "front_right_leg");
            anatomy.add_limb(LimbType::LEG, "back_left_leg");
            anatomy.add_limb(LimbType::LEG, "back_right_leg");
            anatomy.add_limb(LimbType::FOOT, "front_left_foot");
            anatomy.add_limb(LimbType::FOOT, "front_right_foot");
            anatomy.add_limb(LimbType::FOOT, "back_left_foot");
            anatomy.add_limb(LimbType::FOOT, "back_right_foot");
        } else if (preset == "spider") {
            anatomy.add_limb(LimbType::HEAD, "head");
            anatomy.add_limb(LimbType::TORSO, "abdomen");
            for (int i = 1; i <= 8; ++i) {
                anatomy.add_limb(LimbType::LEG, "leg_" + std::to_string(i));
            }
        }
        
        // Parse additional/custom limbs
        if (data.has("limbs") && data["limbs"].is_array()) {
            for (size_t i = 0; i < data["limbs"].size(); ++i) {
                const json::Value& limb = data["limbs"][i];
                LimbType type = EnumConverters::string_to_limb_type(limb["type"].get_string("other"));
                std::string name = limb["name"].get_string("limb");
                float resistance = static_cast<float>(limb["resistance"].get_number(0.5));
                anatomy.add_limb(type, name, resistance);
            }
        }
        
        entity.add_component(anatomy);
    }
    
    void parse_utility_ai_component(Entity& entity, const json::Value& data) {
        UtilityAIComponent utility_ai;
        
        // Apply preset if specified
        std::string preset = data["preset"].get_string("");
        if (!preset.empty()) {
            apply_utility_preset(utility_ai, preset);
        }
        
        // Override specific values
        if (data.has("aggro_range")) {
            utility_ai.aggro_range = static_cast<float>(data["aggro_range"].get_number(6.0));
        }
        
        entity.add_component(utility_ai);
    }
    
    void parse_item_component(Entity& entity, const json::Value& data) {
        ItemComponent item;
        item.type = EnumConverters::string_to_item_type(data["type"].get_string("misc"));
        item.equip_slot = EnumConverters::string_to_equip_slot(data["equip_slot"].get_string("none"));
        item.value = data["value"].get_int(0);
        item.stackable = data["stackable"].get_bool(false);
        item.stack_count = data["stack_count"].get_int(1);
        item.armor_bonus = data["armor_bonus"].get_int(0);
        item.attack_bonus = data["attack_bonus"].get_int(0);
        item.base_damage = data["base_damage"].get_int(0);
        item.defense_bonus = data["defense_bonus"].get_int(0);
        item.cutting_chance = static_cast<float>(data["cutting_chance"].get_number(0.0));
        item.required_limbs = data["required_limbs"].get_int(1);
        item.required_limb_type = EnumConverters::string_to_limb_type(data["required_limb_type"].get_string("hand"));
        entity.add_component(item);
    }
    
    void parse_ranged_weapon_component(Entity& entity, const json::Value& data) {
        RangedWeaponComponent ranged;
        ranged.range = data["range"].get_int(5);
        ranged.min_range = data["min_range"].get_int(1);
        ranged.ammo_type = EnumConverters::string_to_ammo_type(data["ammo_type"].get_string("none"));
        ranged.consumes_self = data["consumes_self"].get_bool(false);
        ranged.accuracy_bonus = data["accuracy_bonus"].get_int(0);
        std::string proj_char = data["projectile_char"].get_string("*");
        ranged.projectile_char = proj_char.empty() ? '*' : proj_char[0];
        ranged.projectile_color = data["projectile_color"].get_string("white");
        entity.add_component(ranged);
    }
    
    void parse_ammo_component(Entity& entity, const json::Value& data) {
        AmmoComponent ammo;
        ammo.ammo_type = EnumConverters::string_to_ammo_type(data["ammo_type"].get_string("arrow"));
        ammo.quantity = data["quantity"].get_int(1);
        ammo.damage_bonus = data["damage_bonus"].get_int(0);
        entity.add_component(ammo);
    }
    
    void parse_item_effect_component(Entity& entity, const json::Value& data) {
        ItemEffectComponent effects;
        
        if (data.has("effects") && data["effects"].is_array()) {
            for (size_t i = 0; i < data["effects"].size(); ++i) {
                const json::Value& eff = data["effects"][i];
                
                ItemEffectComponent::EffectType type = EnumConverters::string_to_effect_type(eff["type"].get_string("custom"));
                ItemEffectComponent::Trigger trigger = EnumConverters::string_to_effect_trigger(eff["trigger"].get_string("on_use"));
                int magnitude = eff["magnitude"].get_int(0);
                int duration = eff["duration"].get_int(0);
                float chance = static_cast<float>(eff["chance"].get_number(1.0));
                std::string message = eff["message"].get_string("");
                
                // Check for registered effect ID (for custom serializable effects)
                std::string effect_id = eff["effect_id"].get_string("");
                if (!effect_id.empty()) {
                    effects.add_registered_effect(trigger, effect_id, message);
                } else {
                    effects.add_effect(type, trigger, magnitude, duration, chance, message);
                }
            }
        }
        
        entity.add_component(effects);
    }
    
    void parse_npc_component(Entity& entity, const json::Value& data) {
        NPCComponent npc;
        npc.disposition = EnumConverters::string_to_disposition(data["disposition"].get_string("neutral"));
        npc.role = EnumConverters::string_to_role(data["role"].get_string("villager"));
        npc.greeting = data["greeting"].get_string("");
        npc.farewell = data["farewell"].get_string("");
        npc.can_trade = data["can_trade"].get_bool(false);
        npc.can_give_quests = data["can_give_quests"].get_bool(false);
        
        // Setup default interactions based on role (Talk, Trade, Steal, Attack, etc.)
        npc.setup_default_interactions();
        
        entity.add_component(npc);
    }
    
    void parse_door_component(Entity& entity, const json::Value& data) {
        DoorComponent door;
        door.is_open = data["is_open"].get_bool(false);
        std::string closed_ch = data["closed_char"].get_string("+");
        std::string open_ch = data["open_char"].get_string("-");
        door.closed_char = closed_ch.empty() ? '+' : closed_ch[0];
        door.open_char = open_ch.empty() ? '-' : open_ch[0];
        door.closed_color = data["closed_color"].get_string("brown");
        door.open_color = data["open_color"].get_string("gray");
        entity.add_component(door);
    }
    
    void parse_chest_component(Entity& entity, const json::Value& data) {
        ChestComponent chest;
        chest.is_open = data["is_open"].get_bool(false);
        std::string closed_ch = data["closed_char"].get_string("=");
        std::string open_ch = data["open_char"].get_string("u");
        chest.closed_char = closed_ch.empty() ? '=' : closed_ch[0];
        chest.open_char = open_ch.empty() ? 'u' : open_ch[0];
        chest.closed_color = data["closed_color"].get_string("yellow");
        chest.open_color = data["open_color"].get_string("dark_yellow");
        entity.add_component(chest);
    }
    
    void parse_interactable_component(Entity& entity, const json::Value& data) {
        InteractableComponent interact;
        interact.interaction_message = data["message"].get_string("Press E to interact");
        entity.add_component(interact);
    }
    
    void parse_equipment_component(Entity& entity, const json::Value& /*data*/) {
        entity.add_component(EquipmentComponent{});
    }
    
    void parse_player_controller_component(Entity& entity, const json::Value& data) {
        PlayerControllerComponent controller;
        controller.can_move = data["can_move"].get_bool(true);
        controller.can_attack = data["can_attack"].get_bool(true);
        entity.add_component(controller);
    }
    
    void parse_dialogue_component(Entity& entity, const json::Value& data) {
        DialogueComponent dialogue;
        dialogue.start_node_id = data["start_node_id"].get_int(0);
        
        if (data.has("nodes") && data["nodes"].is_array()) {
            for (size_t i = 0; i < data["nodes"].size(); ++i) {
                const json::Value& node_data = data["nodes"][i];
                
                DialogueNode node;
                node.id = node_data["id"].get_int(static_cast<int>(i));
                node.speaker_name = node_data["speaker"].get_string("");
                node.text = node_data["text"].get_string("");
                
                if (node_data.has("options") && node_data["options"].is_array()) {
                    for (size_t j = 0; j < node_data["options"].size(); ++j) {
                        const json::Value& opt_data = node_data["options"][j];
                        DialogueOption opt;
                        opt.text = opt_data["text"].get_string("");
                        opt.next_node_id = opt_data["next"].get_int(-1);
                        opt.ends_dialogue = opt_data["ends"].get_bool(opt.next_node_id == -1);
                        opt.action = opt_data["action"].get_string("");
                        opt.condition = opt_data["condition"].get_string("");
                        node.options.push_back(opt);
                    }
                }
                
                dialogue.add_node(node);
            }
        }
        
        entity.add_component(dialogue);
    }
    
    void parse_shop_inventory_component(Entity& entity, const json::Value& data) {
        ShopInventoryComponent shop;
        shop.buy_multiplier = static_cast<float>(data["buy_multiplier"].get_number(1.0));
        shop.sell_multiplier = static_cast<float>(data["sell_multiplier"].get_number(0.5));
        shop.restocks = data["restocks"].get_bool(true);
        shop.restock_interval = data["restock_interval"].get_int(100);
        
        if (data.has("items") && data["items"].is_array()) {
            for (size_t i = 0; i < data["items"].size(); ++i) {
                const json::Value& item_data = data["items"][i];
                
                ShopItem item;
                
                // Check if this is a template reference
                item.template_id = item_data["template_id"].get_string("");
                
                item.name = item_data["name"].get_string("Unknown Item");
                item.description = item_data["description"].get_string("");
                std::string sym = item_data["symbol"].get_string("?");
                item.symbol = sym.empty() ? '?' : sym[0];
                item.color = item_data["color"].get_string("white");
                item.buy_price = item_data["price"].get_int(item_data["buy_price"].get_int(10));
                item.sell_price = item_data["sell_price"].get_int(item.buy_price / 2);
                item.stock = item_data["stock"].get_int(-1);
                item.available = item_data["available"].get_bool(true);
                
                // Item type and slot
                item.item_type = EnumConverters::string_to_item_type(item_data["type"].get_string("misc"));
                item.equip_slot = EnumConverters::string_to_equip_slot(item_data["equip_slot"].get_string("none"));
                
                // Combat bonuses
                item.attack_bonus = item_data["attack"].get_int(item_data["attack_bonus"].get_int(0));
                item.defense_bonus = item_data["defense"].get_int(item_data["defense_bonus"].get_int(0));
                item.armor_bonus = item_data["armor"].get_int(item_data["armor_bonus"].get_int(0));
                
                // Consumable
                item.is_consumable = item_data["is_consumable"].get_bool(item.item_type == ItemComponent::Type::CONSUMABLE);
                
                // Parse effects for consumables
                if (item_data.has("effects") && item_data["effects"].is_array()) {
                    ItemEffectComponent effect_comp;
                    for (size_t j = 0; j < item_data["effects"].size(); ++j) {
                        const json::Value& eff = item_data["effects"][j];
                        ItemEffectComponent::EffectType type = EnumConverters::string_to_effect_type(eff["type"].get_string("custom"));
                        ItemEffectComponent::Trigger trigger = EnumConverters::string_to_effect_trigger(eff["trigger"].get_string("on_use"));
                        int magnitude = eff["magnitude"].get_int(0);
                        int duration = eff["duration"].get_int(0);
                        float chance = static_cast<float>(eff["chance"].get_number(1.0));
                        std::string message = eff["message"].get_string("");
                        
                        std::string effect_id = eff["effect_id"].get_string("");
                        if (!effect_id.empty()) {
                            effect_comp.add_registered_effect(trigger, effect_id, message);
                        } else {
                            effect_comp.add_effect(type, trigger, magnitude, duration, chance, message);
                        }
                    }
                    item.effects.push_back(effect_comp);
                }
                
                // Ranged weapon properties
                item.is_ranged = item_data["is_ranged"].get_bool(false);
                item.range = item_data["range"].get_int(0);
                item.min_range = item_data["min_range"].get_int(1);
                item.ammo_type = EnumConverters::string_to_ammo_type(item_data["ammo_type"].get_string("none"));
                item.consumes_self = item_data["consumes_self"].get_bool(false);
                item.accuracy_bonus = item_data["accuracy"].get_int(item_data["accuracy_bonus"].get_int(0));
                
                // Ammo properties
                item.is_ammo = item_data["is_ammo"].get_bool(false);
                item.ammo_quantity = item_data["ammo_quantity"].get_int(item_data["quantity"].get_int(1));
                item.ammo_damage_bonus = item_data["ammo_damage_bonus"].get_int(item_data["damage_bonus"].get_int(0));
                
                shop.add_item(item);
            }
        }
        
        entity.add_component(shop);
    }
    
    void parse_leaves_corpse_component(Entity& entity, const json::Value& data) {
        LeavesCorpseComponent leaves_corpse;
        leaves_corpse.corpse_name = data["corpse_name"].get_string("");
        leaves_corpse.corpse_description = data["corpse_description"].get_string("");
        std::string sym = data["corpse_symbol"].get_string("%");
        leaves_corpse.corpse_symbol = sym.empty() ? '%' : sym[0];
        leaves_corpse.corpse_color = data["corpse_color"].get_string("dark_red");
        leaves_corpse.transfer_inventory = data["transfer_inventory"].get_bool(true);
        leaves_corpse.transfer_anatomy = data["transfer_anatomy"].get_bool(true);
        leaves_corpse.transfer_equipment = data["transfer_equipment"].get_bool(true);
        leaves_corpse.decay_turns = data["decay_turns"].get_int(-1);
        entity.add_component(leaves_corpse);
    }
    
    void parse_corpse_component(Entity& entity, const json::Value& data) {
        CorpseComponent corpse;
        corpse.original_entity_name = data["original_entity_name"].get_string("");
        corpse.original_entity_id = data["original_entity_id"].get_string("");
        corpse.decay_turns_remaining = data["decay_turns"].get_int(-1);
        corpse.is_butchered = data["is_butchered"].get_bool(false);
        entity.add_component(corpse);
    }

    void parse_properties_component(Entity& entity, const json::Value& data) {
        PropertiesComponent props;
        if (data.is_array()) {
            // Shorthand: "properties": ["trait1", "trait2"]
            for (size_t i = 0; i < data.size(); ++i) {
                props.add_property(data[i].get_string(""));
            }
        } else if (data.is_object()) {
            // Object form: "properties": { "list": ["trait1", "trait2"] }
            if (data.has("list") && data["list"].is_array()) {
                for (size_t i = 0; i < data["list"].size(); ++i) {
                    props.add_property(data["list"][i].get_string(""));
                }
            }
        }
        entity.add_component(props);
    }

public:
    EntityLoader() = default;
    
    void set_entities_path(const std::string& path) {
        entities_path = path;
    }
    
    // Load a single entity template from JSON file
    bool load_template(const std::string& filepath) {
        try {
            json::Value root = json::parse_file(filepath);
            
            EntityTemplate tmpl;
            tmpl.id = root["id"].get_string("");
            if (tmpl.id.empty()) {
                // Use filename as ID if not specified
                std::filesystem::path p(filepath);
                tmpl.id = p.stem().string();
            }
            
            tmpl.name = root["name"].get_string(tmpl.id);
            tmpl.description = root["description"].get_string("");
            tmpl.category = root["category"].get_string("misc");
            
            // Store components as JSON object for lazy parsing
            if (root.has("components") && root["components"].is_object()) {
                tmpl.components = root["components"].as_object();
            }
            
            // Parse tags
            if (root.has("tags") && root["tags"].is_array()) {
                for (size_t i = 0; i < root["tags"].size(); ++i) {
                    tmpl.tags.push_back(root["tags"][i].get_string(""));
                }
            }
            
            tmpl.valid = true;
            templates[tmpl.id] = tmpl;
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }
    
    // Load all entity templates from directory
    int load_templates_from_directory(const std::string& directory) {
        int count = 0;
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    if (load_template(entry.path().string())) {
                        count++;
                    }
                }
            }
        } catch (const std::exception&) {
            // Directory doesn't exist or can't be read
        }
        
        return count;
    }
    
    // Get a template by ID
    const EntityTemplate* get_template(const std::string& id) const {
        auto it = templates.find(id);
        return (it != templates.end()) ? &it->second : nullptr;
    }
    
    // Get all template IDs
    std::vector<std::string> get_template_ids() const {
        std::vector<std::string> ids;
        for (const auto& pair : templates) {
            ids.push_back(pair.first);
        }
        return ids;
    }
    
    // Get templates by category
    std::vector<const EntityTemplate*> get_templates_by_category(const std::string& category) const {
        std::vector<const EntityTemplate*> result;
        for (const auto& pair : templates) {
            if (pair.second.category == category) {
                result.push_back(&pair.second);
            }
        }
        return result;
    }
    
    // Get templates by tag
    std::vector<const EntityTemplate*> get_templates_by_tag(const std::string& tag) const {
        std::vector<const EntityTemplate*> result;
        for (const auto& pair : templates) {
            for (const auto& t : pair.second.tags) {
                if (t == tag) {
                    result.push_back(&pair.second);
                    break;
                }
            }
        }
        return result;
    }
    
    // Create an entity from a template
    // Returns entity with id 0 if template not found
    Entity create_entity_from_template(
        ComponentManager* manager,
        const std::string& template_id,
        int x = -1,
        int y = -1,
        int level_override = 0,
        const std::string& name_override = ""
    ) {
        const EntityTemplate* tmpl = get_template(template_id);
        if (!tmpl || !tmpl->valid) {
            return Entity(0, manager);
        }
        
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        
        // Parse and add each component from the template
        for (const auto& pair : tmpl->components) {
            const std::string& comp_name = pair.first;
            const json::Value& comp_data = pair.second;
            
            if (comp_name == "position") {
                parse_position_component(entity, comp_data, x, y);
            } else if (comp_name == "render") {
                parse_render_component(entity, comp_data);
            } else if (comp_name == "stats") {
                parse_stats_component(entity, comp_data, level_override);
            } else if (comp_name == "name") {
                parse_name_component(entity, comp_data, name_override);
            } else if (comp_name == "collision") {
                parse_collision_component(entity, comp_data);
            } else if (comp_name == "faction") {
                parse_faction_component(entity, comp_data);
            } else if (comp_name == "inventory") {
                parse_inventory_component(entity, comp_data);
            } else if (comp_name == "anatomy") {
                parse_anatomy_component(entity, comp_data);
            } else if (comp_name == "utility_ai") {
                parse_utility_ai_component(entity, comp_data);
            } else if (comp_name == "item") {
                parse_item_component(entity, comp_data);
            } else if (comp_name == "ranged_weapon") {
                parse_ranged_weapon_component(entity, comp_data);
            } else if (comp_name == "ammo") {
                parse_ammo_component(entity, comp_data);
            } else if (comp_name == "item_effect") {
                parse_item_effect_component(entity, comp_data);
            } else if (comp_name == "npc") {
                parse_npc_component(entity, comp_data);
            } else if (comp_name == "door") {
                parse_door_component(entity, comp_data);
            } else if (comp_name == "chest") {
                parse_chest_component(entity, comp_data);
            } else if (comp_name == "interactable") {
                parse_interactable_component(entity, comp_data);
            } else if (comp_name == "equipment") {
                parse_equipment_component(entity, comp_data);
            } else if (comp_name == "player_controller") {
                parse_player_controller_component(entity, comp_data);
            } else if (comp_name == "dialogue") {
                parse_dialogue_component(entity, comp_data);
            } else if (comp_name == "shop" || comp_name == "shop_inventory") {
                parse_shop_inventory_component(entity, comp_data);
            } else if (comp_name == "leaves_corpse") {
                parse_leaves_corpse_component(entity, comp_data);
            } else if (comp_name == "corpse") {
                parse_corpse_component(entity, comp_data);
            } else if (comp_name == "properties") {
                parse_properties_component(entity, comp_data);
            }
        }
        
        // If position wasn't in template but coordinates provided, add it
        // Note: We use x != -1 check since -1 is the default "no position" value
        if (!entity.get_component<PositionComponent>() && (x != -1 || y != -1)) {
            entity.add_component(PositionComponent{x, y, 0});
        }
        
        return entity;
    }
    
    // Check if a template exists
    bool has_template(const std::string& id) const {
        return templates.find(id) != templates.end();
    }
    
    // Get template count
    size_t template_count() const {
        return templates.size();
    }
    
    // Clear all loaded templates
    void clear() {
        templates.clear();
    }
};
