#include "../systems/inventory_system.hpp"
#include "../util/rng.hpp"

InventorySystem::InventorySystem(ComponentManager* mgr, CombatSystem* combat) 
    : manager(mgr), combat_system(combat) {}

void InventorySystem::set_message_callback(std::function<void(const std::string&)> callback) {
    message_callback = callback;
}

bool InventorySystem::pickup_items(EntityId entity_id) {
    PositionComponent* pos = manager->get_component<PositionComponent>(entity_id);
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    
    if (!pos || !inv) return false;
    
    bool picked_up_any = false;
    auto item_entities = manager->get_entities_with_component<ItemComponent>();
    
    for (EntityId item_id : item_entities) {
        PositionComponent* item_pos = manager->get_component<PositionComponent>(item_id);
        NameComponent* item_name = manager->get_component<NameComponent>(item_id);
        
        if (item_pos && item_pos->x == pos->x && item_pos->y == pos->y) {
            if (inv->add_item(item_id)) {
                std::string name = item_name ? item_name->name : "Item";
                if (message_callback) {
                    message_callback("Picked up " + name + ".");
                }
                
                manager->remove_component<PositionComponent>(item_id);
                picked_up_any = true;
            } else {
                if (message_callback) {
                    message_callback("Inventory full!");
                }
            }
        }
    }
    
    return picked_up_any;
}

void InventorySystem::apply_item_stats(EntityId entity_id, EntityId item_id, bool apply) {
    StatsComponent* stats = manager->get_component<StatsComponent>(entity_id);
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    
    if (!stats || !item) return;
    
    int multiplier = apply ? 1 : -1;
    stats->attack += item->attack_bonus * multiplier;
    stats->defense += item->defense_bonus * multiplier;
}

bool InventorySystem::equip_item(EntityId entity_id, int inventory_index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(entity_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    
    if (!inv || !anatomy || !equip) return false;
    if (inventory_index < 0 || inventory_index >= static_cast<int>(inv->items.size())) return false;
    
    EntityId item_id = inv->items[inventory_index];
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(item_id);
    
    if (!item) return false;
    
    if (item->equip_slot == ItemComponent::EquipSlot::NONE) {
        if (message_callback) {
            message_callback("This item cannot be equipped.");
        }
        return false;
    }
    
    std::vector<Limb*> available_limbs = anatomy->get_limbs_of_type(item->required_limb_type);
    
    if (available_limbs.empty()) {
        if (message_callback) {
            message_callback("You don't have the right limbs to equip this!");
        }
        return false;
    }
    
    if (static_cast<int>(available_limbs.size()) < item->required_limbs) {
        if (message_callback) {
            message_callback("You need " + std::to_string(item->required_limbs) + " limbs to equip this!");
        }
        return false;
    }
    
    std::vector<Limb*> limbs_to_use;
    for (auto* limb : available_limbs) {
        if (equip->get_equipped(limb->name) == 0) {
            limbs_to_use.push_back(limb);
            if (static_cast<int>(limbs_to_use.size()) >= item->required_limbs) break;
        }
    }
    
    if (static_cast<int>(limbs_to_use.size()) < item->required_limbs) {
        if (message_callback) {
            message_callback("Not enough free limbs! Unequip something first.");
        }
        return false;
    }
    
    for (auto* limb : limbs_to_use) {
        equip->equip_to_limb(limb->name, item_id);
    }
    
    inv->remove_item(item_id);
    
    std::string name = item_name ? item_name->name : "Item";
    if (message_callback) {
        message_callback("Equipped " + name + ".");
    }
    
    apply_item_stats(entity_id, item_id, true);
    
    if (effects && combat_system) {
        auto on_equip_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_EQUIP);
        for (const auto& effect : on_equip_effects) {
            combat_system->apply_effect(entity_id, effect);
        }
    }
    
    return true;
}

bool InventorySystem::use_item(EntityId entity_id, int inventory_index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    if (!inv) return false;
    if (inventory_index < 0 || inventory_index >= static_cast<int>(inv->items.size())) return false;
    
    EntityId item_id = inv->items[inventory_index];
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(item_id);
    
    if (!item) return false;
    
    if (item->type == ItemComponent::Type::CONSUMABLE) {
        std::string name = item_name ? item_name->name : "Item";
        
        if (effects && combat_system) {
            auto on_use_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_USE);
            for (const auto& effect : on_use_effects) {
                if (effect.chance < 1.0f) {
                    if (!chance(effect.chance)) continue;
                }
                
                combat_system->apply_effect(entity_id, effect);
            }
        } else {
            if (message_callback) {
                message_callback("Used " + name + ".");
            }
        }
        
        inv->remove_item(item_id);
        manager->destroy_entity(item_id);
        return true;
    } else {
        if (message_callback) {
            message_callback("This item cannot be used directly. Try equipping it.");
        }
        return false;
    }
}

bool InventorySystem::drop_item(EntityId entity_id, int inventory_index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    PositionComponent* entity_pos = manager->get_component<PositionComponent>(entity_id);
    
    if (!inv || !entity_pos) return false;
    if (inventory_index < 0 || inventory_index >= static_cast<int>(inv->items.size())) return false;
    
    EntityId item_id = inv->items[inventory_index];
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    
    std::string name = item_name ? item_name->name : "Item";
    if (message_callback) {
        message_callback("Dropped " + name + ".");
    }
    
    inv->remove_item(item_id);
    
    if (!manager->get_component<PositionComponent>(item_id)) {
        manager->add_component(item_id, PositionComponent{entity_pos->x, entity_pos->y, 0});
    } else {
        PositionComponent* pos = manager->get_component<PositionComponent>(item_id);
        pos->x = entity_pos->x;
        pos->y = entity_pos->y;
    }
    
    if (!manager->get_component<RenderComponent>(item_id)) {
        manager->add_component(item_id, RenderComponent{'?', "cyan", "", 3});
    }
    
    return true;
}

bool InventorySystem::unequip_item(EntityId entity_id, const std::string& limb_name) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    
    if (!inv || !equip) return false;
    
    // Check if there's an item equipped on this limb (or special slot)
    EntityId item_id = 0;
    bool is_special_slot = false;
    
    if (limb_name == "back") {
        item_id = equip->back;
        is_special_slot = true;
    } else if (limb_name == "neck") {
        item_id = equip->neck;
        is_special_slot = true;
    } else if (limb_name == "waist") {
        item_id = equip->waist;
        is_special_slot = true;
    } else {
        item_id = equip->get_equipped(limb_name);
    }
    
    if (item_id == 0) {
        if (message_callback) {
            message_callback("Nothing equipped on " + limb_name + ".");
        }
        return false;
    }
    
    // Check if inventory has space
    if (inv->items.size() >= static_cast<size_t>(inv->max_items)) {
        if (message_callback) {
            message_callback("Inventory is full! Cannot unequip.");
        }
        return false;
    }
    
    // Get item info
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(item_id);
    
    // Handle special slots
    if (is_special_slot) {
        if (limb_name == "back") equip->back = 0;
        else if (limb_name == "neck") equip->neck = 0;
        else if (limb_name == "waist") equip->waist = 0;
    } else {
        // Remove from all limbs that have this item equipped (for multi-limb items)
        std::vector<std::string> limbs_to_clear;
        for (const auto& pair : equip->equipped_items) {
            if (pair.second == item_id) {
                limbs_to_clear.push_back(pair.first);
            }
        }
        
        for (const auto& limb : limbs_to_clear) {
            equip->unequip_from_limb(limb);
        }
    }
    
    // Remove stats
    apply_item_stats(entity_id, item_id, false);
    
    // Handle on-unequip effects
    if (effects && combat_system) {
        auto on_unequip_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_UNEQUIP);
        for (const auto& effect : on_unequip_effects) {
            combat_system->apply_effect(entity_id, effect);
        }
    }
    
    // Add back to inventory
    inv->add_item(item_id);
    
    std::string name = item_name ? item_name->name : "Item";
    if (message_callback) {
        message_callback("Unequipped " + name + ".");
    }
    
    return true;
}
