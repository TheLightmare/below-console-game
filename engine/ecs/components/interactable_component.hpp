#pragma once
#include "component_base.hpp"
#include "../types.hpp"
#include <vector>
#include <algorithm>
#include <string>

// Door component
struct DoorComponent : Component {
    bool is_open = false;
    char closed_char = '+';
    char open_char = '-';
    std::string closed_color = "brown";
    std::string open_color = "gray";
    
    DoorComponent() = default;
    DoorComponent(bool is_open, char closed_char = '+', char open_char = '-')
        : is_open(is_open), closed_char(closed_char), open_char(open_char) {}
    
    void toggle() {
        is_open = !is_open;
    }
    IMPLEMENT_CLONE(DoorComponent)
};

// Chest component
struct ChestComponent : Component {
    bool is_open = false;
    std::vector<EntityId> items;
    char closed_char = '=';
    char open_char = 'u';
    std::string closed_color = "yellow";
    std::string open_color = "dark_yellow";
    
    ChestComponent() = default;
    explicit ChestComponent(bool is_open)
        : is_open(is_open) {}
    
    void add_item(EntityId item_id) {
        items.push_back(item_id);
    }
    
    void remove_item(EntityId item_id) {
        items.erase(std::remove(items.begin(), items.end(), item_id), items.end());
    }
    
    void toggle() {
        is_open = !is_open;
    }
    IMPLEMENT_CLONE(ChestComponent)
};

// Interactable component
struct InteractableComponent : Component {
    std::string interaction_message;
    
    InteractableComponent() = default;
    InteractableComponent(const std::string& message)
        : interaction_message(message) {}
    IMPLEMENT_CLONE(InteractableComponent)
};
