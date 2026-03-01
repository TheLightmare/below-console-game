#pragma once
#include "types.hpp"
#include "component.hpp"
#include "spatial_hash.hpp"
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

    // Spatial hash index — kept in sync with PositionComponent automatically.
    SpatialHash spatial_hash_;

    // Internal: register an entity in the spatial hash if it has a position.
    void spatial_insert(EntityId id) {
        auto* pos = get_component<PositionComponent>(id);
        if (pos) spatial_hash_.insert(id, pos->x, pos->y, pos->z);
    }

    // Internal: remove an entity from the spatial hash.
    void spatial_remove(EntityId id) {
        spatial_hash_.remove(id);
    }

public:
    ComponentManager() : spatial_hash_(8) {}

    EntityId create_entity() {
        EntityId id = next_entity_id++;
        components[id]; // Just create the entry, default constructs empty map
        return id;
    }

    void destroy_entity(EntityId id) {
        spatial_remove(id);
        components.erase(id);
    }
    
    void clear_all_entities() {
        spatial_hash_.clear();
        components.clear();
    }

    template<typename T>
    void add_component(EntityId id, T component) {
        components[id][std::type_index(typeid(T))] = 
            std::make_unique<T>(std::move(component));
        // If a PositionComponent was just added, register in spatial hash
        if constexpr (std::is_same_v<T, PositionComponent>) {
            spatial_insert(id);
        }
    }
    
    // Variadic template version that constructs component in-place
    template<typename T, typename... Args>
    T& add_component(EntityId id, Args&&... args) {
        auto comp = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = comp.get();
        components[id][std::type_index(typeid(T))] = std::move(comp);
        // If a PositionComponent was just added, register in spatial hash
        if constexpr (std::is_same_v<T, PositionComponent>) {
            spatial_insert(id);
        }
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
        // If removing PositionComponent, unregister from spatial hash
        if constexpr (std::is_same_v<T, PositionComponent>) {
            spatial_remove(id);
        }
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

    // ---- Spatial-hash-accelerated position update ---------------------------
    
    // Move an entity's PositionComponent and update the spatial hash in one call.
    // Returns true if the entity has a PositionComponent and was moved.
    bool move_entity(EntityId id, int new_x, int new_y, int new_z = 0) {
        auto* pos = get_component<PositionComponent>(id);
        if (!pos) return false;
        int old_x = pos->x, old_y = pos->y, old_z = pos->z;
        pos->x = new_x;
        pos->y = new_y;
        pos->z = new_z;
        spatial_hash_.move(id, old_x, old_y, old_z, new_x, new_y, new_z);
        return true;
    }

    // Notify the spatial hash after directly mutating a PositionComponent.
    // Call this if you changed pos->x/y/z manually instead of using move_entity().
    void update_spatial(EntityId id, int old_x, int old_y, int old_z) {
        auto* pos = get_component<PositionComponent>(id);
        if (pos) spatial_hash_.move(id, old_x, old_y, old_z, pos->x, pos->y, pos->z);
    }

    // ---- Spatial-hash-accelerated queries -----------------------------------
    
    // z-aware position blocking check — O(cell) instead of O(N)
    bool is_position_blocked(int x, int y, EntityId exclude_id = 0, int z = 0) {
        const auto& candidates = spatial_hash_.at_cell(x, y, z);
        for (EntityId id : candidates) {
            if (id == exclude_id) continue;
            
            PositionComponent* pos = get_component<PositionComponent>(id);
            CollisionComponent* col = get_component<CollisionComponent>(id);
            
            if (pos && col && pos->x == x && pos->y == y && pos->z == z && col->blocks_movement) {
                return true;
            }
        }
        return false;
    }
    
    // z-aware entity lookup — O(cell) instead of O(N)
    template<typename T>
    EntityId get_entity_at(int x, int y, EntityId exclude_id = 0, int z = 0) {
        const auto& candidates = spatial_hash_.at_cell(x, y, z);
        for (EntityId id : candidates) {
            if (id == exclude_id) continue;
            
            PositionComponent* pos = get_component<PositionComponent>(id);
            if (pos && pos->x == x && pos->y == y && pos->z == z && has_component<T>(id)) {
                return id;
            }
        }
        return 0;
    }

    // Get all entities within `radius` tiles of (cx, cy, cz) — spatial hash accelerated
    std::vector<EntityId> get_entities_in_radius(int cx, int cy, int cz, int radius) {
        return spatial_hash_.in_radius(cx, cy, cz, radius);
    }
    std::vector<EntityId> get_entities_in_radius(int cx, int cy, int radius) {
        return spatial_hash_.in_radius(cx, cy, radius);
    }
    
    static bool is_at_position(PositionComponent* pos, int x, int y, int z = 0) {
        return pos && pos->x == x && pos->y == y && pos->z == z;
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
        // If the source had a position, the clone now does too — register it
        spatial_insert(target_id);
    }
    
    // Check if an entity exists
    bool entity_exists(EntityId id) const {
        return components.find(id) != components.end();
    }

    // Direct access to the spatial hash (for advanced queries)
    const SpatialHash& spatial_hash() const { return spatial_hash_; }
    SpatialHash& spatial_hash() { return spatial_hash_; }
};