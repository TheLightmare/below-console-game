#pragma once
#include "component_manager.hpp"
#include <optional>
#include <utility>
#include "component.hpp"
using namespace std;

// Entity class - lightweight wrapper around an entity ID and its component manager.
// Provides convenient access to components via templates.
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