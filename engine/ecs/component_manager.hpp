#pragma once
#include "types.hpp"
#include "component.hpp"
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <typeinfo>

class ComponentManager {
private:
    std::unordered_map<EntityId, 
        std::unordered_map<std::type_index, std::unique_ptr<Component>>> components;
    EntityId next_entity_id = 1;

public:
    EntityId create_entity() {
        EntityId id = next_entity_id++;
        components[id]; // Just create the entry, default constructs empty map
        return id;
    }

    void destroy_entity(EntityId id) {
        components.erase(id);
    }
    
    void clear_all_entities() {
        components.clear();
    }

    template<typename T>
    void add_component(EntityId id, T component) {
        components[id][std::type_index(typeid(T))] = 
            std::make_unique<T>(std::move(component));
    }
    
    // Variadic template version that constructs component in-place
    template<typename T, typename... Args>
    T& add_component(EntityId id, Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = comp.get();
        components[id][std::type_index(typeid(T))] = std::move(comp);
        return *ptr;
    }

    template<typename T>
    T* get_component(EntityId id) {
        auto entity_it = components.find(id);
        if (entity_it == components.end()) return nullptr;
        
        auto comp_it = entity_it->second.find(std::type_index(typeid(T)));
        if (comp_it == entity_it->second.end()) return nullptr;
        
        return static_cast<T*>(comp_it->second.get());
    }

    template<typename T>
    bool has_component(EntityId id) {
        return get_component<T>(id) != nullptr;
    }
    
    template<typename T>
    void remove_component(EntityId id) {
        auto entity_it = components.find(id);
        if (entity_it != components.end()) {
            entity_it->second.erase(std::type_index(typeid(T)));
        }
    }

    template<typename T>
    std::vector<EntityId> get_entities_with_component() {
        std::vector<EntityId> result;
        for (const auto& [id, comps] : components) {
            if (comps.count(std::type_index(typeid(T)))) {
                result.push_back(id);
            }
        }
        return result;
    }
    
    bool is_position_blocked(int x, int y, EntityId exclude_id = 0) {
        auto entities = get_entities_with_component<PositionComponent>();
        for (EntityId id : entities) {
            if (id == exclude_id) continue;
            
            PositionComponent* pos = get_component<PositionComponent>(id);
            CollisionComponent* col = get_component<CollisionComponent>(id);
            
            if (pos && col && pos->x == x && pos->y == y && col->blocks_movement) {
                return true;
            }
        }
        return false;
    }
    
    template<typename T>
    EntityId get_entity_at(int x, int y, EntityId exclude_id = 0) {
        auto entities = get_entities_with_component<PositionComponent>();
        for (EntityId id : entities) {
            if (id == exclude_id) continue;
            
            PositionComponent* pos = get_component<PositionComponent>(id);
            if (pos && pos->x == x && pos->y == y && has_component<T>(id)) {
                return id;
            }
        }
        return 0;
    }
    
    static bool is_at_position(PositionComponent* pos, int x, int y) {
        return pos && pos->x == x && pos->y == y;
    }
    
    // Copy all components from source entity to target entity
    void copy_all_components(EntityId source_id, EntityId target_id) {
        auto source_it = components.find(source_id);
        if (source_it == components.end()) return;
        
        for (const auto& [type_idx, component_ptr] : source_it->second) {
            if (component_ptr) {
                // Clone the component and add it to the target entity
                components[target_id][type_idx] = component_ptr->clone();
            }
        }
    }
    
    // Check if an entity exists
    bool entity_exists(EntityId id) const {
        return components.find(id) != components.end();
    }
};