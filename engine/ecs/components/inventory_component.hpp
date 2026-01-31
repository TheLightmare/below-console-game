#pragma once
#include "component_base.hpp"
#include "../types.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>

struct InventoryComponent : Component {
    std::vector<EntityId> items;
    int max_items = 20;
    int gold = 0;
    
    InventoryComponent() = default;
    InventoryComponent(int max_items, int starting_gold = 0)
        : max_items(max_items), gold(starting_gold) {}
    
    bool add_item(EntityId item_id) {
        if (items.size() >= static_cast<size_t>(max_items)) return false;
        items.push_back(item_id);
        return true;
    }
    
    bool remove_item(EntityId item_id) {
        auto it = std::find(items.begin(), items.end(), item_id);
        if (it != items.end()) {
            items.erase(it);
            return true;
        }
        return false;
    }
    
    bool has_item(EntityId item_id) const {
        return std::find(items.begin(), items.end(), item_id) != items.end();
    }
    IMPLEMENT_CLONE(InventoryComponent)
};

// Limb/Body Part definition
enum class LimbType {
    HEAD,
    TORSO,
    ARM,
    HAND,
    LEG,
    FOOT,
    OTHER
};

struct Limb {
    LimbType type;
    std::string name;
    EntityId equipped_item = 0;  // What's equipped on this limb
    bool functional = true;       // Is the limb still usable?
    float severing_resistance = 0.5f;  // 0.0 = easy to cut, 1.0 = very hard to cut
    
    Limb(LimbType type, const std::string& name, float severing_resistance = 0.5f) 
        : type(type), name(name), severing_resistance(severing_resistance) {}
};

// Anatomy component - defines what limbs/body parts an entity has
struct AnatomyComponent : Component {
    std::vector<Limb> limbs;
    
    AnatomyComponent() = default;
    
    // Add a limb to the anatomy
    void add_limb(LimbType type, const std::string& name, float severing_resistance = 0.5f) {
        limbs.emplace_back(type, name, severing_resistance);
    }
    
    // Get all limbs of a specific type
    std::vector<Limb*> get_limbs_of_type(LimbType type) {
        std::vector<Limb*> result;
        for (auto& limb : limbs) {
            if (limb.type == type && limb.functional) {
                result.push_back(&limb);
            }
        }
        return result;
    }
    
    // Get limb by name
    Limb* get_limb(const std::string& name) {
        for (auto& limb : limbs) {
            if (limb.name == name) {
                return &limb;
            }
        }
        return nullptr;
    }
    
    // Count functional limbs of a type
    int count_limbs(LimbType type) const {
        int count = 0;
        for (const auto& limb : limbs) {
            if (limb.type == type && limb.functional) {
                count++;
            }
        }
        return count;
    }
    IMPLEMENT_CLONE(AnatomyComponent)
};

// Equipment component - now uses dynamic slots based on anatomy
struct EquipmentComponent : Component {
    // Maps limb name to equipped item
    std::unordered_map<std::string, EntityId> equipped_items;
    
    // General equipment slots not tied to limbs
    EntityId back = 0;     // Backpack/cloak
    EntityId neck = 0;     // Amulet/necklace
    EntityId waist = 0;    // Belt
    
    EquipmentComponent() = default;
    
    // Equip item to a specific limb
    bool equip_to_limb(const std::string& limb_name, EntityId item_id) {
        equipped_items[limb_name] = item_id;
        return true;
    }
    
    // Unequip from limb
    EntityId unequip_from_limb(const std::string& limb_name) {
        auto it = equipped_items.find(limb_name);
        if (it != equipped_items.end()) {
            EntityId item = it->second;
            equipped_items.erase(it);
            return item;
        }
        return 0;
    }
    
    // Get equipped item on limb
    EntityId get_equipped(const std::string& limb_name) const {
        auto it = equipped_items.find(limb_name);
        return (it != equipped_items.end()) ? it->second : 0;
    }
    
    // Get all equipped items
    std::vector<EntityId> get_all_equipped() const {
        std::vector<EntityId> items;
        for (const auto& pair : equipped_items) {
            if (pair.second != 0) {
                items.push_back(pair.second);
            }
        }
        if (back != 0) items.push_back(back);
        if (neck != 0) items.push_back(neck);
        if (waist != 0) items.push_back(waist);
        return items;
    }
    IMPLEMENT_CLONE(EquipmentComponent)
};
