#pragma once
#include "component_manager.hpp"
#include <optional>
#include <utility>
#include "component.hpp"
using namespace std;

// Base Entity class - part of engine
class Entity {
protected:
    EntityId id;
    ComponentManager* manager;

public:
    Entity(EntityId id, ComponentManager* mgr) : id(id), manager(mgr) {}

    EntityId get_id() const { return id; }
    ComponentManager* get_manager() const { return manager; }

    template<typename T>
    void add_component(T component) {
        manager->add_component<T>(id, std::move(component));
    }

    template<typename T>
    T* get_component() {
        return manager->get_component<T>(id);
    }

    std::optional<std::pair<int, int>> get_position() {
        if (PositionComponent* pos = get_component<PositionComponent>()) {
            return std::make_pair(pos->x, pos->y);
        }
        return std::nullopt;
    }
};

// Abstract base entity factory - part of engine
class BaseEntityFactory {
protected:
    ComponentManager* manager;

public:
    BaseEntityFactory(ComponentManager* mgr) : manager(mgr) {}
    virtual ~BaseEntityFactory() = default;
    
    // Pure virtual methods that game must implement
    virtual Entity create_player(int x, int y, const std::string& name = "Player") = 0;
    virtual Entity create_enemy(int x, int y, const std::string& name = "Enemy", int level = 1) = 0;
    
    Entity clone_entity(Entity source) {
        EntityId new_id = manager->create_entity();
        Entity new_entity(new_id, manager);
        
        // Copy all components from source to new entity
        manager->copy_all_components(source.get_id(), new_id);
        
        return new_entity;
    }

    // Common utility - add standard humanoid anatomy
    void add_humanoid_anatomy(Entity& entity) {
        AnatomyComponent anatomy;
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
        entity.add_component(anatomy);
    }
    
    // Common utility - add quadruped anatomy (4 legs)
    void add_quadruped_anatomy(Entity& entity) {
        AnatomyComponent anatomy;
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
        entity.add_component(anatomy);
    }
    
    // Common utility - add spider anatomy (8 legs)
    void add_spider_anatomy(Entity& entity) {
        AnatomyComponent anatomy;
        anatomy.add_limb(LimbType::HEAD, "head");
        anatomy.add_limb(LimbType::TORSO, "abdomen");
        for (int i = 1; i <= 8; ++i) {
            anatomy.add_limb(LimbType::LEG, "leg_" + std::to_string(i));
        }
        entity.add_component(anatomy);
    }
};