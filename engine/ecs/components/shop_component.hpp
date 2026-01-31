#pragma once
#include "component_base.hpp"
#include "item_component.hpp"  // For ItemComponent types, RangedWeaponComponent::AmmoType
#include <vector>
#include <string>

// Shop item entry - represents an item for sale
struct ShopItem {
    EntityId item_template_id = 0;  // Reference item (or 0 for procedural)
    std::string template_id = "";   // Template ID to create item from (e.g., "health_potion")
    std::string name;
    std::string description;
    char symbol = '?';
    std::string color = "white";
    int buy_price = 0;      // Price player pays to buy
    int sell_price = 0;     // Price player gets when selling
    int stock = -1;         // -1 = infinite, otherwise limited stock
    bool available = true;
    
    // Item stats (for creating the item when bought)
    ItemComponent::Type item_type = ItemComponent::Type::MISC;
    ItemComponent::EquipSlot equip_slot = ItemComponent::EquipSlot::NONE;
    int attack_bonus = 0;
    int defense_bonus = 0;
    int armor_bonus = 0;
    
    // For consumables
    bool is_consumable = false;
    std::vector<ItemEffectComponent> effects = {};
    
    // For ranged weapons
    bool is_ranged = false;
    int range = 0;
    int min_range = 1;
    RangedWeaponComponent::AmmoType ammo_type = RangedWeaponComponent::AmmoType::NONE;
    bool consumes_self = false;
    int accuracy_bonus = 0;
    
    // For ammo
    bool is_ammo = false;
    int ammo_quantity = 0;
    int ammo_damage_bonus = 0;
    
    ShopItem() = default;
    ShopItem(const std::string& n, const std::string& desc, char sym, const std::string& col,
             int buy, int sell, int s = -1)
        : name(n), description(desc), symbol(sym), color(col), 
          buy_price(buy), sell_price(sell), stock(s) {}
    
    // Create a weapon shop item
    static ShopItem weapon(const std::string& name, const std::string& desc, char sym, 
                           int buy_price, int attack, ItemComponent::EquipSlot slot = ItemComponent::EquipSlot::HAND) {
        ShopItem item(name, desc, sym, "cyan", buy_price, buy_price / 2);
        item.item_type = ItemComponent::Type::WEAPON;
        item.equip_slot = slot;
        item.attack_bonus = attack;
        return item;
    }
    
    // Create an armor shop item
    static ShopItem armor(const std::string& name, const std::string& desc, char sym,
                          int buy_price, int defense, ItemComponent::EquipSlot slot) {
        ShopItem item(name, desc, sym, "gray", buy_price, buy_price / 2);
        item.item_type = ItemComponent::Type::ARMOR;
        item.equip_slot = slot;
        item.defense_bonus = defense;
        return item;
    }
    
    // Create a consumable shop item
    static ShopItem consumable(const std::string& name, const std::string& desc, char sym,
                               int buy_price, std::vector<ItemEffectComponent> effects, int stock = -1) {
        ShopItem item(name, desc, sym, "magenta", buy_price, buy_price / 2, stock);
        item.item_type = ItemComponent::Type::CONSUMABLE;
        item.is_consumable = true;
        item.effects = effects;
        return item;
    }
    
    // Create a ranged weapon shop item
    static ShopItem ranged_weapon(const std::string& name, const std::string& desc, char sym,
                                  int buy_price, int attack, int range, int min_range,
                                  RangedWeaponComponent::AmmoType ammo_type,
                                  ItemComponent::EquipSlot slot = ItemComponent::EquipSlot::TWO_HAND) {
        ShopItem item(name, desc, sym, "brown", buy_price, buy_price / 2);
        item.item_type = ItemComponent::Type::WEAPON;
        item.equip_slot = slot;
        item.attack_bonus = attack;
        item.is_ranged = true;
        item.range = range;
        item.min_range = min_range;
        item.ammo_type = ammo_type;
        return item;
    }
    
    // Create a throwing weapon shop item
    static ShopItem throwing_weapon(const std::string& name, const std::string& desc, char sym,
                                    int buy_price, int attack, int range, int quantity = 5) {
        ShopItem item(name, desc, sym, "silver", buy_price, buy_price / 2);
        item.item_type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::HAND;
        item.attack_bonus = attack;
        item.is_ranged = true;
        item.range = range;
        item.min_range = 1;
        item.ammo_type = RangedWeaponComponent::AmmoType::NONE;
        item.consumes_self = true;
        item.stock = quantity;
        return item;
    }
    
    // Create an ammo shop item
    static ShopItem ammo(const std::string& name, const std::string& desc, char sym,
                         int buy_price, RangedWeaponComponent::AmmoType ammo_type,
                         int quantity = 20, int damage_bonus = 0, int stock = -1) {
        ShopItem item(name, desc, sym, "brown", buy_price, buy_price / 2, stock);
        item.item_type = ItemComponent::Type::CONSUMABLE;
        item.is_ammo = true;
        item.ammo_type = ammo_type;
        item.ammo_quantity = quantity;
        item.ammo_damage_bonus = damage_bonus;
        return item;
    }
};

// Shop inventory component - for merchants
struct ShopInventoryComponent : Component {
    std::vector<ShopItem> items;
    float buy_multiplier = 1.0f;   // Price multiplier for buying from shop
    float sell_multiplier = 0.5f;  // Price multiplier for selling to shop
    bool restocks = true;          // Does the shop restock?
    int restock_interval = 100;    // Turns between restocks
    int turns_since_restock = 0;
    
    ShopInventoryComponent() = default;
    
    void add_item(const ShopItem& item) {
        items.push_back(item);
    }
    
    void add_weapon(const std::string& name, const std::string& desc, char sym,
                   int price, int attack, ItemComponent::EquipSlot slot = ItemComponent::EquipSlot::HAND) {
        items.push_back(ShopItem::weapon(name, desc, sym, price, attack, slot));
    }
    
    void add_ranged_weapon(const std::string& name, const std::string& desc, char sym,
                           int price, int attack, int range, int min_range,
                           RangedWeaponComponent::AmmoType ammo_type,
                           ItemComponent::EquipSlot slot = ItemComponent::EquipSlot::TWO_HAND) {
        items.push_back(ShopItem::ranged_weapon(name, desc, sym, price, attack, range, min_range, ammo_type, slot));
    }
    
    void add_throwing_weapon(const std::string& name, const std::string& desc, char sym,
                             int price, int attack, int range, int quantity = 5) {
        items.push_back(ShopItem::throwing_weapon(name, desc, sym, price, attack, range, quantity));
    }
    
    void add_ammo(const std::string& name, const std::string& desc, char sym,
                  int price, RangedWeaponComponent::AmmoType ammo_type,
                  int quantity = 20, int damage_bonus = 0, int stock = -1) {
        items.push_back(ShopItem::ammo(name, desc, sym, price, ammo_type, quantity, damage_bonus, stock));
    }
    
    void add_armor(const std::string& name, const std::string& desc, char sym,
                  int price, int defense, ItemComponent::EquipSlot slot) {
        items.push_back(ShopItem::armor(name, desc, sym, price, defense, slot));
    }
    
    void add_consumable(const std::string& name, const std::string& desc, char sym,
                       int price, std::vector<ItemEffectComponent> effects, int stock = -1) {
        items.push_back(ShopItem::consumable(name, desc, sym, price, effects, stock));
    }
    
    // Get items player can afford
    std::vector<size_t> get_affordable_items(int player_gold) const {
        std::vector<size_t> affordable;
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].available && (items[i].stock != 0) && 
                static_cast<int>(items[i].buy_price * buy_multiplier) <= player_gold) {
                affordable.push_back(i);
            }
        }
        return affordable;
    }
    
    // Purchase an item (returns true if successful)
    bool purchase(size_t index) {
        if (index >= items.size()) return false;
        if (!items[index].available) return false;
        if (items[index].stock == 0) return false;
        
        if (items[index].stock > 0) {
            items[index].stock--;
            if (items[index].stock == 0) {
                items[index].available = false;
            }
        }
        return true;
    }
    IMPLEMENT_CLONE(ShopInventoryComponent)
};
