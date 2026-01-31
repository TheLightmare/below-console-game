#pragma once
#include "../../engine/ecs/entity.hpp"
#include "../../engine/ecs/component_manager.hpp"
#include "../../engine/ecs/component.hpp"
#include "../../engine/loaders/entity_loader.hpp"
#include "../../engine/systems/utility_ai.hpp"
#include <cstdlib>

// Helper namespace for creating item effects easily
namespace ItemEffects {
    // Type alias for custom effect callbacks
    using EffectCallback = CustomEffectCallback;
    
    // Create a single-effect component for healing
    inline std::vector<ItemEffectComponent> heal(int amount, const std::string& msg = "You feel healthier!") {
        ItemEffectComponent effect;
        effect.add_effect(ItemEffectComponent::EffectType::HEAL_HP, 
                         ItemEffectComponent::Trigger::ON_USE, amount, 0, 1.0f, msg);
        return {effect};
    }
    
    // Create a single-effect component for percent HP restore
    inline std::vector<ItemEffectComponent> heal_percent(int percent, const std::string& msg = "You feel fully restored!") {
        ItemEffectComponent effect;
        effect.add_effect(ItemEffectComponent::EffectType::RESTORE_HP_PERCENT, 
                         ItemEffectComponent::Trigger::ON_USE, percent, 0, 1.0f, msg);
        return {effect};
    }
    
    // Create a buff effect (attack boost)
    inline std::vector<ItemEffectComponent> buff_attack(int amount, int duration, const std::string& msg = "You feel stronger!") {
        ItemEffectComponent effect;
        effect.add_effect(ItemEffectComponent::EffectType::BUFF_ATTACK, 
                         ItemEffectComponent::Trigger::ON_USE, amount, duration, 1.0f, msg);
        return {effect};
    }
    
    // Create a buff effect (defense boost)
    inline std::vector<ItemEffectComponent> buff_defense(int amount, int duration, const std::string& msg = "You feel tougher!") {
        ItemEffectComponent effect;
        effect.add_effect(ItemEffectComponent::EffectType::BUFF_DEFENSE, 
                         ItemEffectComponent::Trigger::ON_USE, amount, duration, 1.0f, msg);
        return {effect};
    }
    
    // Create a custom effect with a lambda callback (NOT SERIALIZABLE - use registered() for save/load support)
    // The callback receives: (target_entity_id, component_manager, message_callback)
    inline std::vector<ItemEffectComponent> custom(EffectCallback callback, const std::string& msg = "") {
        ItemEffectComponent effect;
        effect.add_custom_effect(ItemEffectComponent::Trigger::ON_USE, callback, msg);
        return {effect};
    }
    
    // Create a custom effect using a registered effect ID (SERIALIZABLE)
    // The effect must be registered in EffectRegistry before use
    inline std::vector<ItemEffectComponent> registered(const std::string& effect_id, const std::string& msg = "") {
        ItemEffectComponent effect;
        effect.add_registered_effect(ItemEffectComponent::Trigger::ON_USE, effect_id, msg);
        return {effect};
    }
    
    // Create a simple message-only custom effect (no actual game logic)
    inline std::vector<ItemEffectComponent> custom_message(const std::string& msg) {
        ItemEffectComponent effect;
        effect.add_effect(ItemEffectComponent::EffectType::CUSTOM, 
                         ItemEffectComponent::Trigger::ON_USE, 0, 0, 1.0f, msg);
        return {effect};
    }
    
    // Create empty effects (for items that don't have standard effects)
    inline std::vector<ItemEffectComponent> none() {
        return {};
    }
}

// Game-specific entity factory implementation
// Uses JSON templates exclusively for entity creation
class EntityFactory : public BaseEntityFactory {
private:
    EntityLoader entity_loader;
    bool templates_loaded = false;
    
public:
    EntityFactory(ComponentManager* mgr) : BaseEntityFactory(mgr) {}
    
    // Load entity templates from JSON files
    int load_entity_templates(const std::string& directory = "assets/entities") {
        int count = entity_loader.load_templates_from_directory(directory);
        templates_loaded = (count > 0);
        return count;
    }
    
    // Check if templates have been loaded
    bool are_templates_loaded() const { return templates_loaded; }
    
    // Get the entity loader for direct access
    EntityLoader& get_entity_loader() { return entity_loader; }
    const EntityLoader& get_entity_loader() const { return entity_loader; }
    
    // Create entity from JSON template by ID
    Entity create_from_template(const std::string& template_id, int x, int y, 
                                int level = 0, const std::string& name_override = "") {
        return entity_loader.create_entity_from_template(manager, template_id, x, y, level, name_override);
    }
    
    // Get all available template IDs
    std::vector<std::string> get_template_ids() const {
        return entity_loader.get_template_ids();
    }
    
    // Get templates by category (enemy, npc, item, structure)
    std::vector<const EntityTemplate*> get_templates_by_category(const std::string& category) const {
        return entity_loader.get_templates_by_category(category);
    }

    // ==================== Core Entity Creation ====================
    
    // Implement required virtual methods
    Entity create_player(int x, int y, const std::string& name = "Hero") override {
        return entity_loader.create_entity_from_template(manager, "player", x, y, 1, name);
    }

    Entity create_enemy(int x, int y, const std::string& name = "Goblin", int level = 1) override {
        // Map common enemy names to template IDs
        std::string template_id = "goblin"; // default
        if (name.find("Skeleton") != std::string::npos) template_id = "skeleton";
        else if (name.find("Zombie") != std::string::npos) template_id = "zombie";
        else if (name.find("Orc") != std::string::npos) template_id = "orc_warrior";
        else if (name.find("Bandit") != std::string::npos) template_id = "bandit";
        else if (name.find("Demon") != std::string::npos) template_id = "demon";
        
        return entity_loader.create_entity_from_template(manager, template_id, x, y, level, name);
    }
    
    // Game-specific creature types
    Entity create_spider(int x, int y, int level = 1) {
        return entity_loader.create_entity_from_template(manager, "giant_spider", x, y, level);
    }
    
    Entity create_wolf(int x, int y, int level = 1) {
        return entity_loader.create_entity_from_template(manager, "wolf", x, y, level);
    }
    
    Entity create_multi_headed_hydra(int x, int y, int level = 3) {
        return entity_loader.create_entity_from_template(manager, "hydra", x, y, level);
    }
    
    Entity create_four_armed_ogre(int x, int y, int level = 2) {
        return entity_loader.create_entity_from_template(manager, "four_armed_ogre", x, y, level);
    }

public:
    
    struct EnemyParams {
        int x = 0;
        int y = 0;
        char symbol = 'e';
        std::string color = "red";
        std::string name = "Enemy";
        std::string description = "A hostile creature";
        int level = 1;
        int hp_multiplier = 15;
        int base_attack = 5;
        int base_defense = 3;
        FactionId faction = FactionId::GOBLIN;
        bool is_humanoid = true;
        float detection_range = 6.0f;
    };
    
    Entity create_custom_enemy(const EnemyParams& params) {
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        
        entity.add_component(PositionComponent{params.x, params.y, 0});
        entity.add_component(RenderComponent{params.symbol, params.color, "", 5});
        entity.add_component(StatsComponent{
            params.level, 0,
            params.hp_multiplier * params.level,
            params.hp_multiplier * params.level,
            params.base_attack + params.level,
            params.base_defense + params.level
        });
        entity.add_component(NameComponent{params.name, params.description});
        entity.add_component(CollisionComponent{true, false});
        entity.add_component(FactionComponent{params.faction});
        
        // Add Utility AI with behavior based on faction/type
        UtilityAIComponent utility_ai;
        utility_ai.aggro_range = params.detection_range;
        switch (params.faction) {
            case FactionId::UNDEAD:
                UtilityPresets::setup_undead(utility_ai);
                break;
            case FactionId::SPIDER:
                UtilityPresets::setup_ambush_predator(utility_ai);
                break;
            case FactionId::PREDATOR:
            case FactionId::BEAST:
                UtilityPresets::setup_predator(utility_ai);
                break;
            case FactionId::WILDLIFE:
                UtilityPresets::setup_wildlife(utility_ai);
                break;
            case FactionId::DRAGON:
                UtilityPresets::setup_boss(utility_ai);
                break;
            case FactionId::ORC:
                UtilityPresets::setup_berserker(utility_ai);
                break;
            default:
                // Default aggressive behavior for goblins, bandits, demons, etc.
                UtilityPresets::setup_aggressive_melee(utility_ai);
                break;
        }
        utility_ai.aggro_range = params.detection_range;
        entity.add_component(utility_ai);
        
        if (params.is_humanoid) {
            add_humanoid_anatomy(entity);
        } else {
            add_quadruped_anatomy(entity);
        }
        
        return entity;
    }
    
    // Convenience overload for quick enemy creation
    Entity spawn_enemy(int x, int y, char symbol, const std::string& color,
                       const std::string& name, const std::string& desc,
                       int level, int hp_mult, int atk, int def,
                       FactionId faction = FactionId::GOBLIN, bool humanoid = true) {
        EnemyParams params;
        params.x = x;
        params.y = y;
        params.symbol = symbol;
        params.color = color;
        params.name = name;
        params.description = desc;
        params.level = level;
        params.hp_multiplier = hp_mult;
        params.base_attack = atk;
        params.base_defense = def;
        params.faction = faction;
        params.is_humanoid = humanoid;
        return create_custom_enemy(params);
    }

    Entity create_wall(int x, int y) {
        if (entity_loader.has_template("stone_wall")) {
            return entity_loader.create_entity_from_template(manager, "stone_wall", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'#', "gray", "", 1});
        entity.add_component(CollisionComponent{true, true});
        entity.add_component(NameComponent{"Wall", "A solid stone wall"});
        return entity;
    }

    Entity create_door(int x, int y, bool is_open = false) {
        if (entity_loader.has_template("wooden_door")) {
            Entity e = entity_loader.create_entity_from_template(manager, "wooden_door", x, y);
            if (is_open) {
                if (auto* door = e.get_component<DoorComponent>()) door->is_open = true;
                if (auto* col = e.get_component<CollisionComponent>()) col->blocks_movement = false;
                if (auto* render = e.get_component<RenderComponent>()) render->ch = '-';
            }
            return e;
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{is_open ? '-' : '+', is_open ? "gray" : "brown", "", 2});
        entity.add_component(CollisionComponent{!is_open, false});
        entity.add_component(NameComponent{is_open ? "Open Door" : "Closed Door", "A wooden door"});
        entity.add_component(DoorComponent{is_open});
        entity.add_component(InteractableComponent{"Press E to open/close"});
        return entity;
    }

    Entity create_chest(int x, int y, bool is_open = false) {
        if (entity_loader.has_template("treasure_chest")) {
            Entity e = entity_loader.create_entity_from_template(manager, "treasure_chest", x, y);
            if (is_open) {
                if (auto* chest = e.get_component<ChestComponent>()) chest->is_open = true;
                if (auto* render = e.get_component<RenderComponent>()) render->ch = 'u';
            }
            return e;
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{is_open ? 'u' : '=', is_open ? "dark_yellow" : "yellow", "", 3});
        entity.add_component(CollisionComponent{true, false});
        entity.add_component(NameComponent{is_open ? "Open Chest" : "Closed Chest", "A wooden treasure chest"});
        entity.add_component(ChestComponent{is_open});
        entity.add_component(InteractableComponent{"Press E to open/loot"});
        return entity;
    }

    Entity create_item(int x, int y, const std::string& name, char symbol = 'i') {
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{symbol, "cyan", "", 3});
        entity.add_component(ItemComponent{ItemComponent::Type::MISC, 10, false});
        entity.add_component(NameComponent{name, "An item on the ground"});
        return entity;
    }

    Entity create_potion(int x, int y) {
        if (entity_loader.has_template("health_potion")) {
            return entity_loader.create_entity_from_template(manager, "health_potion", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'!', "magenta", "", 3});
        entity.add_component(ItemComponent{ItemComponent::Type::CONSUMABLE, 25, true});
        entity.add_component(NameComponent{"Health Potion", "Restores 20 HP"});
        ItemEffectComponent effects;
        effects.add_effect(ItemEffectComponent::EffectType::HEAL_HP, 
                          ItemEffectComponent::Trigger::ON_USE, 
                          20, 0, 1.0f, "You feel healthier!");
        entity.add_component(effects);
        return entity;
    }
    
    // Equipment creation
    Entity create_sword(int x, int y) {
        if (entity_loader.has_template("iron_sword")) {
            return entity_loader.create_entity_from_template(manager, "iron_sword", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'/', "cyan", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::HAND;
        item.value = 50;
        item.base_damage = 8;
        item.attack_bonus = 1;
        item.cutting_chance = 0.15f;
        item.required_limbs = 1;
        item.required_limb_type = LimbType::HAND;
        entity.add_component(item);
        entity.add_component(NameComponent{"Iron Sword", "A one-handed blade"});
        return entity;
    }
    
    Entity create_two_handed_sword(int x, int y) {
        if (entity_loader.has_template("great_sword")) {
            return entity_loader.create_entity_from_template(manager, "great_sword", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'/', "white", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::TWO_HAND;
        item.value = 120;
        item.base_damage = 18;    // High base damage for two-handed
        item.attack_bonus = 2;    // Small bonus to attack stat
        item.cutting_chance = 0.25f;  // 25% chance to sever a limb
        item.required_limbs = 2;
        item.required_limb_type = LimbType::HAND;
        entity.add_component(item);
        
        entity.add_component(NameComponent{"Great Sword", "A massive two-handed blade"});

        return entity;
    }
    
    // ==================== Ranged Weapons ====================
    
    Entity create_bow(int x, int y) {
        if (entity_loader.has_template("hunting_bow")) {
            return entity_loader.create_entity_from_template(manager, "hunting_bow", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{')', "brown", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::TWO_HAND;
        item.value = 60;
        item.base_damage = 6;
        item.attack_bonus = 0;
        item.required_limbs = 2;
        item.required_limb_type = LimbType::HAND;
        entity.add_component(item);
        RangedWeaponComponent ranged;
        ranged.range = 8;
        ranged.min_range = 2;
        ranged.ammo_type = RangedWeaponComponent::AmmoType::ARROW;
        ranged.accuracy_bonus = 0;
        ranged.projectile_char = '-';
        ranged.projectile_color = "brown";
        entity.add_component(ranged);
        entity.add_component(NameComponent{"Hunting Bow", "A simple but reliable bow"});
        return entity;
    }
    
    Entity create_crossbow(int x, int y) {
        if (entity_loader.has_template("crossbow")) {
            return entity_loader.create_entity_from_template(manager, "crossbow", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{')', "gray", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::TWO_HAND;
        item.value = 120;
        item.base_damage = 4;
        item.attack_bonus = 0;
        item.required_limbs = 2;
        item.required_limb_type = LimbType::HAND;
        entity.add_component(item);
        RangedWeaponComponent ranged;
        ranged.range = 10;
        ranged.min_range = 2;
        ranged.ammo_type = RangedWeaponComponent::AmmoType::BOLT;
        ranged.accuracy_bonus = 2;
        ranged.projectile_char = '-';
        ranged.projectile_color = "white";
        entity.add_component(ranged);
        entity.add_component(NameComponent{"Crossbow", "A powerful mechanical bow"});
        return entity;
    }
    
    Entity create_throwing_knife(int x, int y, int quantity = 5) {
        if (entity_loader.has_template("throwing_knife")) {
            Entity e = entity_loader.create_entity_from_template(manager, "throwing_knife", x, y);
            if (auto* item = e.get_component<ItemComponent>()) item->stack_count = quantity;
            return e;
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'/', "silver", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::HAND;
        item.value = 15;
        item.base_damage = 4;
        item.attack_bonus = 0;
        item.required_limbs = 1;
        item.required_limb_type = LimbType::HAND;
        item.stackable = true;
        item.stack_count = quantity;
        entity.add_component(item);
        RangedWeaponComponent ranged;
        ranged.range = 5;
        ranged.min_range = 1;
        ranged.ammo_type = RangedWeaponComponent::AmmoType::NONE;
        ranged.consumes_self = true;
        ranged.accuracy_bonus = 1;
        ranged.projectile_char = '/';
        ranged.projectile_color = "silver";
        entity.add_component(ranged);
        entity.add_component(NameComponent{"Throwing Knife", "A balanced blade for throwing"});
        return entity;
    }
    
    Entity create_sling(int x, int y) {
        if (entity_loader.has_template("sling")) {
            return entity_loader.create_entity_from_template(manager, "sling", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'(', "brown", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::WEAPON;
        item.equip_slot = ItemComponent::EquipSlot::HAND;
        item.value = 20;
        item.base_damage = 2;
        item.attack_bonus = 0;
        item.required_limbs = 1;
        item.required_limb_type = LimbType::HAND;
        entity.add_component(item);
        RangedWeaponComponent ranged;
        ranged.range = 6;
        ranged.min_range = 1;
        ranged.ammo_type = RangedWeaponComponent::AmmoType::STONE;
        ranged.accuracy_bonus = -1;
        ranged.projectile_char = '*';
        ranged.projectile_color = "gray";
        entity.add_component(ranged);
        entity.add_component(NameComponent{"Sling", "A simple but effective ranged weapon"});
        return entity;
    }
    
    // ==================== Ammo ====================
    
    Entity create_arrows(int x, int y, int quantity = 20) {
        if (entity_loader.has_template("arrows")) {
            Entity e = entity_loader.create_entity_from_template(manager, "arrows", x, y);
            if (auto* item = e.get_component<ItemComponent>()) item->stack_count = quantity;
            if (auto* ammo = e.get_component<AmmoComponent>()) ammo->quantity = quantity;
            return e;
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'|', "brown", "", 2});
        ItemComponent item;
        item.type = ItemComponent::Type::CONSUMABLE;
        item.value = quantity;
        item.stackable = true;
        item.stack_count = quantity;
        entity.add_component(item);
        
        AmmoComponent ammo;
        ammo.ammo_type = RangedWeaponComponent::AmmoType::ARROW;
        ammo.quantity = quantity;
        ammo.damage_bonus = 0;
        entity.add_component(ammo);
        
        entity.add_component(NameComponent{"Arrows", "Standard wooden arrows"});

        return entity;
    }
    
    Entity create_bolts(int x, int y, int quantity = 15) {
        if (entity_loader.has_template("crossbow_bolts")) {
            Entity e = entity_loader.create_entity_from_template(manager, "crossbow_bolts", x, y);
            if (auto* item = e.get_component<ItemComponent>()) item->stack_count = quantity;
            if (auto* ammo = e.get_component<AmmoComponent>()) ammo->quantity = quantity;
            return e;
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'|', "gray", "", 2});
        ItemComponent item;
        item.type = ItemComponent::Type::CONSUMABLE;
        item.value = quantity * 2;
        item.stackable = true;
        item.stack_count = quantity;
        entity.add_component(item);
        AmmoComponent ammo;
        ammo.ammo_type = RangedWeaponComponent::AmmoType::BOLT;
        ammo.quantity = quantity;
        ammo.damage_bonus = 2;
        entity.add_component(ammo);
        entity.add_component(NameComponent{"Crossbow Bolts", "Heavy iron-tipped bolts"});
        return entity;
    }
    
    Entity create_stones(int x, int y, int quantity = 30) {
        if (entity_loader.has_template("sling_stones")) {
            Entity e = entity_loader.create_entity_from_template(manager, "sling_stones", x, y);
            if (auto* item = e.get_component<ItemComponent>()) item->stack_count = quantity;
            if (auto* ammo = e.get_component<AmmoComponent>()) ammo->quantity = quantity;
            return e;
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'*', "gray", "", 1});
        ItemComponent item;
        item.type = ItemComponent::Type::CONSUMABLE;
        item.value = 1;
        item.stackable = true;
        item.stack_count = quantity;
        entity.add_component(item);
        AmmoComponent ammo;
        ammo.ammo_type = RangedWeaponComponent::AmmoType::STONE;
        ammo.quantity = quantity;
        ammo.damage_bonus = -1;
        entity.add_component(ammo);
        entity.add_component(NameComponent{"Sling Stones", "Smooth stones for slinging"});
        return entity;
    }
    
    Entity create_helmet(int x, int y) {
        if (entity_loader.has_template("iron_helmet")) {
            return entity_loader.create_entity_from_template(manager, "iron_helmet", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'^', "gray", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::HELMET;
        item.equip_slot = ItemComponent::EquipSlot::HEAD;
        item.value = 40;
        item.defense_bonus = 3;
        item.required_limbs = 1;
        item.required_limb_type = LimbType::HEAD;
        entity.add_component(item);
        entity.add_component(NameComponent{"Iron Helmet", "Protects your head"});
        return entity;
    }
    
    Entity create_armor(int x, int y) {
        if (entity_loader.has_template("iron_armor")) {
            return entity_loader.create_entity_from_template(manager, "iron_armor", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'[', "gray", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::ARMOR;
        item.equip_slot = ItemComponent::EquipSlot::TORSO;
        item.value = 80;
        item.armor_bonus = 5;
        item.defense_bonus = 5;
        item.required_limbs = 1;
        item.required_limb_type = LimbType::TORSO;
        entity.add_component(item);
        entity.add_component(NameComponent{"Iron Armor", "Sturdy chest protection"});
        return entity;
    }
    
    Entity create_boots(int x, int y) {
        if (entity_loader.has_template("leather_boots")) {
            return entity_loader.create_entity_from_template(manager, "leather_boots", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{']', "brown", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::BOOTS;
        item.equip_slot = ItemComponent::EquipSlot::FOOT;
        item.value = 30;
        item.defense_bonus = 2;
        item.required_limbs = 2;
        item.required_limb_type = LimbType::FOOT;
        entity.add_component(item);
        entity.add_component(NameComponent{"Leather Boots", "Comfortable footwear"});
        return entity;
    }
    
    Entity create_shield(int x, int y) {
        if (entity_loader.has_template("wooden_shield")) {
            return entity_loader.create_entity_from_template(manager, "wooden_shield", x, y);
        }
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{'O', "blue", "", 3});
        ItemComponent item;
        item.type = ItemComponent::Type::SHIELD;
        item.equip_slot = ItemComponent::EquipSlot::HAND;
        item.value = 60;
        item.defense_bonus = 8;
        item.required_limbs = 1;
        item.required_limb_type = LimbType::HAND;
        entity.add_component(item);
        entity.add_component(NameComponent{"Wooden Shield", "A sturdy defensive tool"});
        return entity;
    }
    
    // NPC base creation helper - kept as internal since NPCs have complex state
    Entity create_npc_base(int x, int y, const std::string& name, char symbol, 
                          const std::string& color, NPCComponent::Disposition disposition,
                          NPCComponent::Role role, FactionId faction = FactionId::VILLAGER,
                          int level = 1, int base_hp = 20, int base_attack = 4, int base_defense = 2) {
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        entity.add_component(PositionComponent{x, y, 0});
        entity.add_component(RenderComponent{symbol, color, "", 6});
        entity.add_component(NameComponent{name, "A person you can talk to"});
        entity.add_component(CollisionComponent{true, false});
        entity.add_component(FactionComponent{faction});
        int hp = base_hp + (level - 1) * 5;
        int attack = base_attack + (level - 1) * 2;
        int defense = base_defense + (level - 1);
        entity.add_component(StatsComponent{level, 0, hp, hp, attack, defense});
        NPCComponent npc_comp(disposition, role);
        entity.add_component(npc_comp);
        add_humanoid_anatomy(entity);
        
        // Add empty dialogue (to be filled later)
        entity.add_component(DialogueComponent{});
        
        // Add Utility AI based on role
        UtilityAIComponent utility_ai;
        switch (role) {
            case NPCComponent::Role::GUARD:
                UtilityPresets::setup_town_guard(utility_ai);
                break;
            case NPCComponent::Role::MERCHANT:
            case NPCComponent::Role::INNKEEPER:
                UtilityPresets::setup_merchant(utility_ai);
                break;
            case NPCComponent::Role::WANDERER:
                UtilityPresets::setup_wanderer(utility_ai);
                break;
            default:
                UtilityPresets::setup_defensive_villager(utility_ai);
                break;
        }
        entity.add_component(utility_ai);
        
        return entity;
    }
    
    // Create a generic villager NPC
    Entity create_villager(int x, int y, const std::string& name = "Villager") {
        return entity_loader.create_entity_from_template(manager, "villager", x, y, 0, name);
    }
    
    // Create a villager NPC with a home they return to
    Entity create_villager_with_home(int x, int y, int home_x, int home_y, 
                                     int bed_x = 0, int bed_y = 0,
                                     const std::string& structure_id = "house_small",
                                     const std::string& name = "Villager") {
        // Create base villager
        Entity entity = create_villager(x, y, name);
        
        // Add home component
        HomeComponent home(home_x, home_y, bed_x, bed_y, structure_id);
        entity.add_component(home);
        
        // Update AI to use villager_with_home preset
        UtilityAIComponent* utility_ai = entity.get_component<UtilityAIComponent>();
        if (utility_ai) {
            UtilityPresets::setup_villager_with_home(*utility_ai);
        }
        
        return entity;
    }
    
    // Create a merchant NPC
    Entity create_merchant(int x, int y, const std::string& name = "Merchant") {
        return entity_loader.create_entity_from_template(manager, "merchant", x, y, 0, name);
    }
    
    // Create a guard NPC
    Entity create_guard(int x, int y, const std::string& name = "Guard") {
        return entity_loader.create_entity_from_template(manager, "guard", x, y, 0, name);
    }
    
    // Create an innkeeper NPC
    Entity create_innkeeper(int x, int y, const std::string& name = "Innkeeper") {
        return entity_loader.create_entity_from_template(manager, "innkeeper", x, y, 0, name);
    }
    
    // Create a mysterious stranger NPC
    Entity create_mysterious_stranger(int x, int y) {
        return entity_loader.create_entity_from_template(manager, "mysterious_stranger", x, y, 0);
    }
    
    // Create a wandering trader (spawns randomly in the world)
    Entity create_wandering_trader(int x, int y) {
        return entity_loader.create_entity_from_template(manager, "wandering_trader", x, y, 0);
    }
};
