// Template implementation file for InteractionSystem
// Included by interaction_system.hpp

template<typename WorldType>
bool InteractionSystem::interact_at_position(int x, int y, WorldType* world) {
    auto entities = manager->get_entities_with_component<InteractableComponent>();
    
    for (EntityId id : entities) {
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        if (pos && pos->x == x && pos->y == y) {
            // Check if it's a door
            DoorComponent* door = manager->get_component<DoorComponent>(id);
            if (door) {
                toggle_door(id, door, world);
                return true;
            }
            
            // Could add other interactable types here (levers, chests, etc.)
        }
    }
    
    if (message_callback) {
        message_callback("Nothing to interact with.");
    }
    return false;
}

template<typename WorldType>
void InteractionSystem::toggle_door(EntityId door_id, DoorComponent* door, WorldType* world) {
    door->toggle();
    
    // Update render component
    RenderComponent* render = manager->get_component<RenderComponent>(door_id);
    if (render) {
        render->ch = door->is_open ? door->open_char : door->closed_char;
        render->color = door->is_open ? door->open_color : door->closed_color;
    }
    
    // Update collision
    CollisionComponent* collision = manager->get_component<CollisionComponent>(door_id);
    if (collision) {
        collision->blocks_movement = !door->is_open;
    }
    
    // Update world tile walkability
    PositionComponent* pos = manager->get_component<PositionComponent>(door_id);
    if (pos && world) {
        auto tile = world->get_tile(pos->x, pos->y);
        tile.walkable = door->is_open;
        world->set_tile(pos->x, pos->y, tile);
    }
    
    // Update name
    NameComponent* name = manager->get_component<NameComponent>(door_id);
    if (name) {
        name->name = door->is_open ? "Open Door" : "Closed Door";
    }
    
    if (message_callback) {
        message_callback(door->is_open ? "You opened the door." : "You closed the door.");
    }
}
