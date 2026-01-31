#include "../systems/interaction_system.hpp"

InteractionSystem::InteractionSystem(ComponentManager* mgr) : manager(mgr) {}

void InteractionSystem::set_message_callback(std::function<void(const std::string&)> callback) {
    message_callback = callback;
}

std::vector<EntityId> InteractionSystem::get_nearby_entities(int range) {
    std::vector<EntityId> nearby;
    if (player_id == 0) return nearby;
    
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    if (!player_pos) return nearby;
    
    auto entities = manager->get_entities_with_component<PositionComponent>();
    
    for (EntityId id : entities) {
        if (id == player_id) continue;
        
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        if (!pos) continue;
        
        int dx = std::abs(pos->x - player_pos->x);
        int dy = std::abs(pos->y - player_pos->y);
        
        if (dx <= range && dy <= range) {
            if (is_interactable(id)) {
                nearby.push_back(id);
            }
        }
    }
    
    return nearby;
}

bool InteractionSystem::is_interactable(EntityId entity_id) {
    // Any entity with a name, stats, or specific interactable components can be interacted with
    return manager->get_component<NameComponent>(entity_id) ||
           manager->get_component<NPCComponent>(entity_id) ||
           manager->get_component<InteractableComponent>(entity_id) ||
           manager->get_component<DoorComponent>(entity_id) ||
           manager->get_component<ChestComponent>(entity_id) ||
           manager->get_component<ItemComponent>(entity_id) ||
           manager->get_component<StatsComponent>(entity_id);
}

std::vector<std::string> InteractionSystem::get_entity_info(EntityId target_id) {
    std::vector<std::string> info;
    
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "something";
    
    // Show name and description
    if (name && !name->description.empty()) {
        info.push_back(name->description);
    } else {
        info.push_back("You examine " + target_name + ".");
    }
    
    // Show faction and relationship
    FactionComponent* faction = manager->get_component<FactionComponent>(target_id);
    if (faction) {
        FactionManager& fm = get_faction_manager();
        
        // Get the player's actual faction (for soul swap support)
        FactionComponent* player_faction = manager->get_component<FactionComponent>(player_id);
        FactionId player_faction_id = player_faction ? player_faction->faction : FactionId::PLAYER;
        
        FactionRelation rel = fm.get_relation(player_faction_id, faction->faction);
        std::string rel_str;
        switch (rel) {
            case FactionRelation::ALLY: rel_str = "Allied"; break;
            case FactionRelation::FRIENDLY: rel_str = "Friendly"; break;
            case FactionRelation::NEUTRAL: rel_str = "Neutral"; break;
            case FactionRelation::UNFRIENDLY: rel_str = "Unfriendly"; break;
            case FactionRelation::HOSTILE: rel_str = "Hostile"; break;
            default: rel_str = "Unknown"; break;
        }
        info.push_back("Faction: " + get_faction_name(faction->faction) + " (" + rel_str + ")");
    }
    
    // Show stats if available (creature/NPC)
    StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
    if (stats) {
        info.push_back("Level " + std::to_string(stats->level) + 
                      " | HP: " + std::to_string(stats->current_hp) + "/" + std::to_string(stats->max_hp) +
                      " | ATK: " + std::to_string(stats->attack) + " DEF: " + std::to_string(stats->defense));
    }
    
    // Show item details if it's an item
    ItemComponent* item = manager->get_component<ItemComponent>(target_id);
    if (item) {
        std::string item_info = get_item_type_name(item->type);
        if (item->attack_bonus > 0) item_info += " | ATK +" + std::to_string(item->attack_bonus);
        if (item->defense_bonus > 0) item_info += " | DEF +" + std::to_string(item->defense_bonus);
        if (item->value > 0) item_info += " | Value: " + std::to_string(item->value) + "g";
        info.push_back(item_info);
    }
    
    // Show equipment if creature has any
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(target_id);
    if (equip && !equip->equipped_items.empty()) {
        std::string equipped_list = "Equipped: ";
        bool first = true;
        for (const auto& pair : equip->equipped_items) {
            NameComponent* eq_name = manager->get_component<NameComponent>(pair.second);
            if (eq_name) {
                if (!first) equipped_list += ", ";
                equipped_list += eq_name->name;
                first = false;
            }
        }
        if (!first) info.push_back(equipped_list);
    }
    
    // Show door state
    DoorComponent* door = manager->get_component<DoorComponent>(target_id);
    if (door) {
        info.push_back(door->is_open ? "The door is open." : "The door is closed.");
    }
    
    // Show chest contents hint
    ChestComponent* chest = manager->get_component<ChestComponent>(target_id);
    if (chest) {
        if (chest->items.empty()) {
            info.push_back("The chest appears to be empty.");
        } else {
            info.push_back("The chest contains " + std::to_string(chest->items.size()) + " item(s).");
        }
    }
    
    return info;
}

std::string InteractionSystem::get_faction_name(FactionId faction) {
    switch (faction) {
        case FactionId::PLAYER: return "Player";
        case FactionId::VILLAGER: return "Villager";
        case FactionId::GUARD: return "Guard";
        case FactionId::WILDLIFE: return "Wildlife";
        case FactionId::PREDATOR: return "Predator";
        case FactionId::GOBLIN: return "Goblin";
        case FactionId::UNDEAD: return "Undead";
        case FactionId::BANDIT: return "Bandit";
        case FactionId::DEMON: return "Demon";
        case FactionId::DRAGON: return "Dragon";
        case FactionId::SPIDER: return "Spider";
        case FactionId::ORC: return "Orc";
        case FactionId::ELEMENTAL: return "Elemental";
        case FactionId::BEAST: return "Beast";
        default: return "Unknown";
    }
}

std::string InteractionSystem::get_item_type_name(ItemComponent::Type type) {
    switch (type) {
        case ItemComponent::Type::WEAPON: return "Weapon";
        case ItemComponent::Type::ARMOR: return "Armor";
        case ItemComponent::Type::HELMET: return "Helmet";
        case ItemComponent::Type::BOOTS: return "Boots";
        case ItemComponent::Type::GLOVES: return "Gloves";
        case ItemComponent::Type::RING: return "Ring";
        case ItemComponent::Type::SHIELD: return "Shield";
        case ItemComponent::Type::CONSUMABLE: return "Consumable";
        case ItemComponent::Type::QUEST: return "Quest Item";
        case ItemComponent::Type::MISC: return "Misc";
        default: return "Item";
    }
}