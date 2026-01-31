#pragma once
// Entity Transfer System
// Provides a generalized mechanism for transferring entities between scenes
// with full component serialization/deserialization support.

#include "component_manager.hpp"
#include "component_serialization.hpp"
#include "../util/json.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

// Represents a serialized entity ready for transfer between scenes
struct TransferredEntity {
    std::string components_json;  // JSON string containing all component data
    EntityId original_id = 0;     // Original entity ID (for reference tracking)
    std::string tag;              // Optional tag for identification (e.g., "player", "companion")
    
    // Quick access fields (extracted from components for convenience)
    std::string name;             // From NameComponent
    int x = 0, y = 0, z = 0;      // From PositionComponent (if any)
    
    TransferredEntity() = default;
    TransferredEntity(const std::string& json, EntityId id, const std::string& t = "")
        : components_json(json), original_id(id), tag(t) {}
};

// Manages entity transfers between scenes
// Integrated into SceneManager for cross-scene entity migration
class EntityTransferManager {
private:
    // Pending entities to be transferred to the next scene
    std::unordered_map<std::string, TransferredEntity> pending_transfers;
    
    // ID remapping table for resolving entity references after deserialization
    std::unordered_map<int, EntityId> id_remap;
    
public:
    EntityTransferManager() = default;
    
    // ==================== Serialization (Source Scene) ====================
    
    // Serialize an entity with all components for transfer
    // tag: unique identifier for this entity (e.g., "player", "companion_1")
    // Returns true if successful
    bool queue_entity_for_transfer(EntityId entity_id, ComponentManager* manager, 
                                   const std::string& tag) {
        if (!manager || entity_id == 0) return false;
        
        // Serialize all components
        json::Object comp_obj = ComponentSerializer::serialize_entity_components(entity_id, manager);
        std::string json_str = json::stringify(json::Value(std::move(comp_obj)));
        
        // Extract convenience fields
        TransferredEntity te(json_str, entity_id, tag);
        
        // Get name for quick identification
        NameComponent* name = manager->get_component<NameComponent>(entity_id);
        if (name) te.name = name->name;
        
        // Get position if present
        PositionComponent* pos = manager->get_component<PositionComponent>(entity_id);
        if (pos) {
            te.x = pos->x;
            te.y = pos->y;
            te.z = pos->z;
        }
        
        pending_transfers[tag] = std::move(te);
        return true;
    }
    
    // Serialize multiple entities with a common tag prefix
    // e.g., "party_" produces "party_0", "party_1", etc.
    int queue_entities_for_transfer(const std::vector<EntityId>& entities, 
                                    ComponentManager* manager,
                                    const std::string& tag_prefix) {
        int count = 0;
        for (size_t i = 0; i < entities.size(); ++i) {
            std::string tag = tag_prefix + std::to_string(i);
            if (queue_entity_for_transfer(entities[i], manager, tag)) {
                count++;
            }
        }
        return count;
    }
    
    // ==================== Deserialization (Target Scene) ====================
    
    // Restore a transferred entity in the new scene
    // Returns the new EntityId, or 0 if the tag doesn't exist
    // Optional new_x, new_y, new_z can override the position
    EntityId restore_transferred_entity(const std::string& tag, ComponentManager* manager,
                                        std::optional<int> new_x = std::nullopt,
                                        std::optional<int> new_y = std::nullopt,
                                        std::optional<int> new_z = std::nullopt) {
        auto it = pending_transfers.find(tag);
        if (it == pending_transfers.end()) return 0;
        
        const TransferredEntity& te = it->second;
        
        // Parse and deserialize
        json::Value comp_val;
        if (!json::try_parse(te.components_json, comp_val)) {
            return 0;
        }
        
        EntityId new_id = ComponentSerializer::deserialize_entity_components(comp_val, manager, &id_remap);
        
        // Override position if specified
        if (new_x.has_value() || new_y.has_value() || new_z.has_value()) {
            PositionComponent* pos = manager->get_component<PositionComponent>(new_id);
            if (pos) {
                if (new_x.has_value()) pos->x = *new_x;
                if (new_y.has_value()) pos->y = *new_y;
                if (new_z.has_value()) pos->z = *new_z;
            } else if (new_x.has_value() && new_y.has_value()) {
                // Add position component if it doesn't exist but coords were specified
                manager->add_component<PositionComponent>(new_id, *new_x, *new_y, new_z.value_or(0));
            }
        }
        
        return new_id;
    }
    
    // Restore a transferred entity and remove position component
    // Useful for inventory items that shouldn't exist in world space
    EntityId restore_transferred_entity_no_position(const std::string& tag, ComponentManager* manager) {
        EntityId id = restore_transferred_entity(tag, manager);
        if (id != 0) {
            manager->remove_component<PositionComponent>(id);
        }
        return id;
    }
    
    // Check if a transfer with given tag is pending
    bool has_pending_transfer(const std::string& tag) const {
        return pending_transfers.count(tag) > 0;
    }
    
    // Get info about a pending transfer without restoring it
    const TransferredEntity* get_pending_transfer_info(const std::string& tag) const {
        auto it = pending_transfers.find(tag);
        return (it != pending_transfers.end()) ? &it->second : nullptr;
    }
    
    // Get all pending transfer tags matching a prefix
    std::vector<std::string> get_transfer_tags_with_prefix(const std::string& prefix) const {
        std::vector<std::string> result;
        for (const auto& [tag, te] : pending_transfers) {
            if (tag.compare(0, prefix.length(), prefix) == 0) {
                result.push_back(tag);
            }
        }
        return result;
    }
    
    // ==================== Management ====================
    
    // Remove a pending transfer (either after restoring or to cancel)
    void clear_transfer(const std::string& tag) {
        pending_transfers.erase(tag);
    }
    
    // Clear all pending transfers
    void clear_all_transfers() {
        pending_transfers.clear();
        id_remap.clear();
    }
    
    // Get ID remapping table (useful for fixing entity references after restoration)
    const std::unordered_map<int, EntityId>& get_id_remap() const {
        return id_remap;
    }
    
    // Clear ID remapping table
    void clear_id_remap() {
        id_remap.clear();
    }
    
    // Get number of pending transfers
    size_t pending_count() const {
        return pending_transfers.size();
    }
    
    // ==================== Convenience Methods ====================
    
    // Transfer player entity between scenes (common use case)
    // Source scene calls queue_player_for_transfer before changing scenes
    // Target scene calls restore_player in init_game/on_enter
    bool queue_player_for_transfer(EntityId player_id, ComponentManager* manager) {
        return queue_entity_for_transfer(player_id, manager, "player");
    }
    
    EntityId restore_player(ComponentManager* manager, int spawn_x, int spawn_y) {
        return restore_transferred_entity("player", manager, spawn_x, spawn_y, 0);
    }
    
    bool has_player_transfer() const {
        return has_pending_transfer("player");
    }
    
    void clear_player_transfer() {
        clear_transfer("player");
    }
    
    // Get the serialized JSON for the player transfer (for save system integration)
    // Returns empty string if no player transfer is pending
    std::string get_player_transfer_json() const {
        auto it = pending_transfers.find("player");
        if (it != pending_transfers.end()) {
            return it->second.components_json;
        }
        return "";
    }
    
    // Create a player transfer from serialized JSON (for load system integration)
    // This allows loading saved player data through the transfer system
    void set_player_transfer_from_json(const std::string& components_json, EntityId original_id = 0) {
        TransferredEntity te(components_json, original_id, "player");
        pending_transfers["player"] = std::move(te);
    }
};

// ==================== Static Instance for Cross-Scene Access ====================
// The EntityTransferManager needs to persist across scene transitions
// It's owned by SceneManager and accessible via get_transfer_manager()

