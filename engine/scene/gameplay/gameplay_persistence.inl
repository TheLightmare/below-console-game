// gameplay_persistence.inl - Player/World persistence methods for BaseGameplayScene
// This file is included by base_gameplay_scene.hpp - do not include directly
// NOTE: component_serialization.hpp is included by base_gameplay_scene.hpp before this file

// Debug flag for save/load logging - set to true to enable file logging
#ifndef PERSISTENCE_DEBUG_LOGGING
#define PERSISTENCE_DEBUG_LOGGING false
#endif

// ==================== World Entity Persistence ====================
// Player persistence is now handled by the EntityTransferManager
// These methods handle world entities (NPCs, items on ground, enemies)

// Serialize a single entity to SerializedEntity format
// Now uses FULL component serialization for complete state preservation
SerializedEntity serialize_entity(EntityId entity_id) {
    SerializedEntity se;
    
    // Basic info for quick access/display
    PositionComponent* pos = manager->get_component<PositionComponent>(entity_id);
    if (pos) {
        se.x = pos->x;
        se.y = pos->y;
        se.z = pos->z;
    }
    
    RenderComponent* render = manager->get_component<RenderComponent>(entity_id);
    if (render) {
        se.symbol = render->ch;
        se.color = render->color;
        se.bg_color = render->bg_color;
        se.render_priority = render->priority;
    }
    
    NameComponent* name = manager->get_component<NameComponent>(entity_id);
    if (name) {
        se.name = name->name;
        se.description = name->description;
    }
    
    CollisionComponent* collision = manager->get_component<CollisionComponent>(entity_id);
    if (collision) {
        se.blocks_movement = collision->blocks_movement;
        se.blocks_sight = collision->blocks_sight;
    }
    
    StatsComponent* stats = manager->get_component<StatsComponent>(entity_id);
    if (stats) {
        se.has_stats = true;
        se.level = stats->level;
        se.current_hp = stats->current_hp;
        se.max_hp = stats->max_hp;
        se.attack = stats->attack;
        se.defense = stats->defense;
        se.experience = stats->experience;
    }
    
    UtilityAIComponent* utility_ai = manager->get_component<UtilityAIComponent>(entity_id);
    if (utility_ai) {
        se.has_utility_ai = true;
        se.aggro_range = utility_ai->aggro_range;
        // Store preset name for recreation (simplified - could be enhanced)
        se.utility_ai_preset = "aggressive_melee";  // Default preset
    }
    
    NPCComponent* npc = manager->get_component<NPCComponent>(entity_id);
    if (npc) {
        se.has_npc = true;
        se.npc_disposition = static_cast<int>(npc->disposition);
        se.npc_role = static_cast<int>(npc->role);
        se.greeting = npc->greeting;
        se.farewell = npc->farewell;
        se.relationship = npc->relationship;
    }
    
    ItemComponent* item = manager->get_component<ItemComponent>(entity_id);
    if (item) {
        se.has_item = true;
        se.item_data.name = se.name;
        se.item_data.description = se.description;
        se.item_data.symbol = se.symbol;
        se.item_data.color = se.color;
        se.item_data.type = item->type;
        se.item_data.equip_slot = item->equip_slot;
        se.item_data.value = item->value;
        se.item_data.stackable = item->stackable;
        se.item_data.stack_count = item->stack_count;
        se.item_data.armor_bonus = item->armor_bonus;
        se.item_data.attack_bonus = item->attack_bonus;
        se.item_data.defense_bonus = item->defense_bonus;
        se.item_data.cutting_chance = item->cutting_chance;
    }
    
    FactionComponent* faction = manager->get_component<FactionComponent>(entity_id);
    if (faction) {
        se.faction_id = static_cast<int>(faction->faction);
    }
    
    // Determine entity type for backward compatibility
    if (se.has_item && !se.has_npc && !se.has_stats) {
        se.entity_type = SerializedEntity::EntityType::ITEM;
    } else if (se.has_npc) {
        if (npc && npc->disposition == NPCComponent::Disposition::HOSTILE) {
            se.entity_type = SerializedEntity::EntityType::ENEMY;
        } else {
            se.entity_type = SerializedEntity::EntityType::NPC;
        }
    } else if (se.has_stats && se.has_utility_ai) {
        se.entity_type = SerializedEntity::EntityType::ENEMY;
    } else if (se.blocks_movement || se.name.find("door") != std::string::npos || 
               se.name.find("chest") != std::string::npos) {
        se.entity_type = SerializedEntity::EntityType::INTERACTABLE;
    } else {
        se.entity_type = SerializedEntity::EntityType::OTHER;
    }
    
    // NEW: Full component serialization for complete state preservation
    json::Object comp_obj = ComponentSerializer::serialize_entity_components(entity_id, manager);
    se.components_json = json::stringify(json::Value(std::move(comp_obj)));
    se.has_full_components = true;
    se.original_entity_id = static_cast<int>(entity_id);
    
    return se;
}

// Deserialize a SerializedEntity and create the entity
// Now uses FULL component deserialization when available
EntityId deserialize_entity(const SerializedEntity& se) {
    // NEW: If full component data is available, use it for complete restoration
    if (se.has_full_components && !se.components_json.empty()) {
        json::Value comp_val;
        if (json::try_parse(se.components_json, comp_val)) {
            return ComponentSerializer::deserialize_entity_components(comp_val, manager);
        }
    }
    
    // Fallback: Legacy deserialization for backward compatibility
    EntityId entity_id = manager->create_entity();
    
    // Position
    manager->add_component<PositionComponent>(entity_id, se.x, se.y, se.z);
    
    // Render
    manager->add_component<RenderComponent>(entity_id, se.symbol, se.color, se.bg_color, se.render_priority);
    
    // Name
    if (!se.name.empty()) {
        manager->add_component<NameComponent>(entity_id, se.name, se.description);
    }
    
    // Collision
    if (se.blocks_movement || se.blocks_sight) {
        manager->add_component<CollisionComponent>(entity_id, se.blocks_movement, se.blocks_sight);
    }
    
    // Stats
    if (se.has_stats) {
        manager->add_component<StatsComponent>(entity_id, se.level, se.experience, se.max_hp, se.current_hp, se.attack, se.defense);
    }
    
    // Utility AI - restored via preset name or default aggressive behavior
    if (se.has_utility_ai) {
        UtilityAIComponent utility_ai;
        utility_ai.aggro_range = se.aggro_range;
        // Apply preset based on stored name, default to aggressive melee
        if (se.utility_ai_preset == "defensive_guard") {
            UtilityPresets::setup_defensive_guard(utility_ai);
        } else if (se.utility_ai_preset == "cowardly") {
            UtilityPresets::setup_cowardly(utility_ai);
        } else if (se.utility_ai_preset == "pack_hunter") {
            UtilityPresets::setup_pack_hunter(utility_ai);
        } else if (se.utility_ai_preset == "berserker") {
            UtilityPresets::setup_berserker(utility_ai);
        } else if (se.utility_ai_preset == "undead") {
            UtilityPresets::setup_undead(utility_ai);
        } else {
            UtilityPresets::setup_aggressive_melee(utility_ai);
        }
        manager->add_component<UtilityAIComponent>(entity_id, utility_ai);
    }
    
    // NPC
    if (se.has_npc) {
        auto& npc = manager->add_component<NPCComponent>(entity_id, 
            static_cast<NPCComponent::Disposition>(se.npc_disposition),
            static_cast<NPCComponent::Role>(se.npc_role));
        npc.greeting = se.greeting;
        npc.farewell = se.farewell;
        npc.relationship = se.relationship;
    }
    
    // Item
    if (se.has_item) {
        auto& item = manager->add_component<ItemComponent>(entity_id, se.item_data.type, se.item_data.value, se.item_data.stackable);
        item.equip_slot = se.item_data.equip_slot;
        item.stack_count = se.item_data.stack_count;
        item.armor_bonus = se.item_data.armor_bonus;
        item.attack_bonus = se.item_data.attack_bonus;
        item.defense_bonus = se.item_data.defense_bonus;
        item.cutting_chance = se.item_data.cutting_chance;
        
        // Restore item effects
        if (!se.item_data.effects.empty()) {
            auto& effect_comp = manager->add_component<ItemEffectComponent>(entity_id);
            for (const auto& eff : se.item_data.effects) {
                ItemEffectComponent::Effect effect;
                effect.type = static_cast<ItemEffectComponent::EffectType>(eff.effect_type);
                effect.trigger = static_cast<ItemEffectComponent::Trigger>(eff.trigger);
                effect.magnitude = eff.magnitude;
                effect.duration = eff.duration;
                effect.chance = eff.chance;
                effect.message = eff.message;
                effect.effect_id = eff.effect_id;
                effect_comp.effects.push_back(effect);
            }
        }
    }
    
    // Faction
    if (se.faction_id != 0) {
        manager->add_component<FactionComponent>(entity_id, static_cast<FactionId>(se.faction_id));
    }
    
    return entity_id;
}

// Save world entities (NPCs, items on ground, enemies) to game state
void save_world_entities_to_dungeon_state() {
    auto& state = game_state->dungeon;
    state.saved_entities.clear();
    
    // DEBUG: Log to file (optional)
    std::ofstream debug_log;
    if (PERSISTENCE_DEBUG_LOGGING) {
        debug_log.open("debug_world_save.txt");
        debug_log << "save_world_entities_to_dungeon_state called\n";
        debug_log << "player_id = " << player_id << "\n";
    }
    
    // Get all entities with position (except player)
    auto entities_with_pos = manager->get_entities_with_component<PositionComponent>();
    if (PERSISTENCE_DEBUG_LOGGING) debug_log << "entities_with_pos.size() = " << entities_with_pos.size() << "\n";
    
    for (EntityId entity_id : entities_with_pos) {
        if (entity_id == player_id) {
            if (PERSISTENCE_DEBUG_LOGGING) debug_log << "Skipping player entity " << entity_id << "\n";
            continue;  // Skip player
        }
        
        PositionComponent* pos = manager->get_component<PositionComponent>(entity_id);
        if (!pos) continue;
        
        if (PERSISTENCE_DEBUG_LOGGING) {
            NameComponent* name = manager->get_component<NameComponent>(entity_id);
            debug_log << "Saving entity " << entity_id << ": " << (name ? name->name : "unnamed") << " at (" << pos->x << "," << pos->y << ")\n";
        }
        
        state.saved_entities.push_back(serialize_entity(entity_id));
    }
    
    if (PERSISTENCE_DEBUG_LOGGING) {
        debug_log << "Total saved_entities: " << state.saved_entities.size() << "\n";
        debug_log.close();
    }
}

// Restore world entities from game state after loading
void restore_world_entities_from_dungeon_state() {
    auto& state = game_state->dungeon;
    
    // DEBUG: Log to file (optional)
    std::ofstream debug_log;
    if (PERSISTENCE_DEBUG_LOGGING) {
        debug_log.open("debug_world_restore.txt");
        debug_log << "restore_world_entities_from_dungeon_state called\n";
        debug_log << "state.saved_entities.size() = " << state.saved_entities.size() << "\n";
    }
    
    int restored_count = 0;
    for (const auto& se : state.saved_entities) {
        if (PERSISTENCE_DEBUG_LOGGING) {
            debug_log << "Entity: " << se.name << " at (" << se.x << "," << se.y << ")\n";
            debug_log << "  has_full_components = " << se.has_full_components << "\n";
            debug_log << "  components_json.length() = " << se.components_json.length() << "\n";
        }
        
        EntityId eid = deserialize_entity(se);
        if (PERSISTENCE_DEBUG_LOGGING) debug_log << "  created entity_id = " << eid << "\n";
        restored_count++;
    }
    
    if (PERSISTENCE_DEBUG_LOGGING) {
        debug_log << "Restored " << restored_count << " entities\n";
        debug_log.close();
    }
    
    state.saved_entities.clear();  // Clear after restoring
}

// ==================== Utility Functions ====================
