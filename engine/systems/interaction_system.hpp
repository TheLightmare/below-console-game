#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include <functional>
#include <vector>
#include <string>

// Interaction system - handles entity interactions, doors, examination, etc.
class InteractionSystem {
private:
    ComponentManager* manager;
    std::function<void(const std::string&)> message_callback;
    EntityId player_id = 0;

public:
    InteractionSystem(ComponentManager* mgr);
    
    void set_message_callback(std::function<void(const std::string&)> callback);
    void set_player_id(EntityId id) { player_id = id; }
    
    // Get all interactable entities near the player (within range tiles)
    std::vector<EntityId> get_nearby_entities(int range = 1);
    
    // Examine a specific entity - returns detailed info strings
    std::vector<std::string> get_entity_info(EntityId target_id);
    
    // Check if an entity can be interacted with
    bool is_interactable(EntityId entity_id);
    
    // Interact with entity at position (e.g., doors)
    template<typename WorldType>
    bool interact_at_position(int x, int y, WorldType* world);
    
    // Toggle door state
    template<typename WorldType>
    void toggle_door(EntityId door_id, DoorComponent* door, WorldType* world);
    
    // Get faction name as string
    static std::string get_faction_name(FactionId faction);
    
    // Get item type as string
    static std::string get_item_type_name(ItemComponent::Type type);
};

// Template implementations must stay in header
#include "interaction_system.inl"
