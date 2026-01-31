#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "combat_system.hpp"
#include <string>
#include <functional>

// Inventory system - handles all inventory and equipment logic
class InventorySystem {
private:
    ComponentManager* manager;
    CombatSystem* combat_system;
    std::function<void(const std::string&)> message_callback;

public:
    InventorySystem(ComponentManager* mgr, CombatSystem* combat);
    
    void set_message_callback(std::function<void(const std::string&)> callback);
    
    // Pickup items at position
    bool pickup_items(EntityId entity_id);
    
    // Apply item stat bonuses when equipped
    void apply_item_stats(EntityId entity_id, EntityId item_id, bool apply);
    
    // Equip item from inventory
    bool equip_item(EntityId entity_id, int inventory_index);
    
    // Use consumable item from inventory
    bool use_item(EntityId entity_id, int inventory_index);
    
    // Drop item from inventory to world
    bool drop_item(EntityId entity_id, int inventory_index);
    
    // Unequip item from a specific limb
    bool unequip_item(EntityId entity_id, const std::string& limb_name);
};
