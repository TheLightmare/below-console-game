#pragma once

// Prevent Windows min/max macros from conflicting with std::min/max
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "scene.hpp"
#include "scene_ids.hpp"
#include "settings_menu.hpp"
#include "save_load_window.hpp"
#include "menu_window.hpp"
#include "../input/input.hpp"
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/entity_transfer.hpp"
#include "../loaders/entity_loader.hpp"
#include "../world/game_state.hpp"
#include "../systems/combat_system.hpp"
#include "../systems/inventory_system.hpp"
#include "../systems/interaction_system.hpp"
#include "../systems/dialogue_system.hpp"
#include "../systems/ai_system.hpp"
#include "../systems/status_effect_system.hpp"
#include "../ecs/component_serialization.hpp"
#include "../loaders/item_effect_type_registry.hpp"
#include "../util/rng.hpp"
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <functional>
#include <map>

// Base template for gameplay scenes - provides common UI and coordination
// Game logic is handled by systems (combat, inventory, interaction)
template<typename WorldType, typename FactoryType>
class BaseGameplayScene : public Scene {
protected:
    EntityId player_id;
    std::unique_ptr<WorldType> world;
    std::unique_ptr<FactoryType> factory;
    std::vector<std::string> message_log;
    int message_scroll = 0;  // Scroll offset for message log (0 = latest)
    
    // Game systems
    std::unique_ptr<CombatSystem> combat_system;
    std::unique_ptr<InventorySystem> inventory_system;
    std::unique_ptr<InteractionSystem> interaction_system;
    std::unique_ptr<DialogueSystem> dialogue_system;
    std::unique_ptr<AISystem> ai_system;
    std::unique_ptr<StatusEffectSystem> status_effect_system;
    
    // Walkable checker callback type for AI system
    using WalkableChecker = std::function<bool(int, int)>;
    
public:
    BaseGameplayScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : Scene(con, mgr, rend, ui_sys), player_id(0) {
        factory = std::make_unique<FactoryType>(mgr);
        
        // Load entity templates from JSON files
        factory->load_entity_templates("assets/entities");
        
        // Initialize systems
        combat_system = std::make_unique<CombatSystem>(mgr);
        combat_system->set_entity_loader(&factory->get_entity_loader());
        inventory_system = std::make_unique<InventorySystem>(mgr, combat_system.get());
        interaction_system = std::make_unique<InteractionSystem>(mgr);
        dialogue_system = std::make_unique<DialogueSystem>(mgr, con);
        status_effect_system = std::make_unique<StatusEffectSystem>(mgr);
        
        // Wire up status effect system to combat system
        combat_system->set_status_effect_system(status_effect_system.get());
        
        // Set message callbacks
        auto msg_callback = [this](const std::string& msg) { add_message(msg); };
        combat_system->set_message_callback(msg_callback);
        inventory_system->set_message_callback(msg_callback);
        interaction_system->set_message_callback(msg_callback);
        dialogue_system->set_message_callback(msg_callback);
        status_effect_system->set_message_callback(msg_callback);
        
        // Set player death callbacks to record death reason in game state
        auto death_callback = [this](const std::string& reason) {
            if (game_state) {
                game_state->dungeon.death_reason = reason;
            }
        };
        combat_system->set_player_death_callback(death_callback);
        status_effect_system->set_player_death_callback(death_callback);
        
        // Set dialogue action callback
        dialogue_system->set_action_callback([this](const std::string& action, EntityId npc_id) {
            handle_dialogue_action(action, npc_id);
        });
    }
    
    virtual ~BaseGameplayScene() = default;
    
    virtual void process_turn() {}
    
    virtual void handle_dialogue_action(const std::string& action, EntityId /*npc_id*/) {
        if (action == "trade") {
            add_message("Trading not yet implemented.");
        } else if (action == "rest") {
            // Restore HP
            StatsComponent* stats = manager->get_component<StatsComponent>(player_id);
            if (stats) {
                stats->current_hp = stats->max_hp;
                add_message("You feel well rested. HP fully restored!");
            }
        } else if (action == "accept_favor") {
            add_message("You've made a mysterious pact...");
        }
    }
    
    CombatSystem* get_combat_system() { return combat_system.get(); }
    DialogueSystem* get_dialogue_system() { return dialogue_system.get(); }
    InventorySystem* get_inventory_system() { return inventory_system.get(); }
    InteractionSystem* get_interaction_system() { return interaction_system.get(); }
    AISystem* get_ai_system() { return ai_system.get(); }
    
    // Find and update player_id based on which entity has PlayerControllerComponent
    // This allows effects like soul swap to change which entity the player controls
    void sync_player_id() {
        auto player_entities = manager->get_entities_with_component<PlayerControllerComponent>();
        if (!player_entities.empty()) {
            EntityId new_player_id = player_entities[0];
            if (new_player_id != player_id) {
                player_id = new_player_id;
                // Update systems that track player_id
                if (combat_system) combat_system->set_player_id(player_id);
                if (ai_system) ai_system->set_player_id(player_id);
                if (status_effect_system) status_effect_system->set_player_id(player_id);
            }
        }
    }
    
    void initialize_ai_system(WalkableChecker walkable_checker) {
        ai_system = std::make_unique<AISystem>(manager, player_id);
        
        // Set player ID for systems that need to properly filter messages
        combat_system->set_player_id(player_id);
        status_effect_system->set_player_id(player_id);
        
        ai_system->set_attack_callback([this](EntityId attacker, EntityId target) {
            this->perform_attack(attacker, target);
        });
        
        ai_system->set_walkable_checker(walkable_checker);
    }
    
    virtual bool is_tile_walkable(int x, int y) = 0;
    
    bool can_move_to(int x, int y) {
        // Check if tile is walkable (delegated to derived class)
        if (!is_tile_walkable(x, y)) {
            return false;
        }
        
        // Check for entities blocking movement using centralized utility
        return !manager->is_position_blocked(x, y);
    }
    
    // Common move player logic - handles combat and movement
    void move_player(int dx, int dy) {
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;
        
        int new_x = pos->x + dx;
        int new_y = pos->y + dy;
        
        // Check if there's an enemy at the target position
        EntityId enemy_id = get_enemy_at(new_x, new_y);
        if (enemy_id != 0) {
            // Attack the enemy
            attack_enemy(enemy_id);
            process_turn();  // Enemy gets one turn to respond
            return;
        }
        
        if (can_move_to(new_x, new_y)) {
            pos->x = new_x;
            pos->y = new_y;
            on_player_moved(new_x, new_y);  // Hook for derived classes
            
            // Apply movement cost - slow terrain uses extra turns
            int turns_to_process = get_movement_turns(new_x, new_y);
            for (int i = 0; i < turns_to_process; ++i) {
                process_turn();
            }
        } else {
            on_movement_blocked(new_x, new_y);  // Hook for derived classes
        }
    }
    
    // Get number of turns a move takes based on tile movement cost
    virtual int get_movement_turns(int /*x*/, int /*y*/) {
        return 1;  // Default: 1 turn per move
    }
    
    // Hooks for derived classes to customize behavior
    virtual void on_player_moved(int /*new_x*/, int /*new_y*/) {}
    virtual void on_movement_blocked(int /*x*/, int /*y*/) {
        add_message("You can't move there!");
    }
    
    // ==================== Common Input Handling ====================
    
    // Scene-specific action hooks - override in derived classes
    virtual void on_interact_action() { pickup_items(); process_turn(); }
    virtual void on_confirm_action() {}      // Enter/> key - enter dungeon, go down stairs, etc.
    virtual void on_secondary_action() {}    // < key - go up stairs, etc.
    virtual void on_map_action() {}          // M key - toggle map
    virtual void on_quit_action() { change_scene(SceneId::MENU); }  // Q key
    
    // Default input handler - just calls handle_common_input
    // Override in derived classes for scene-specific input handling
    void handle_input(Key key) override {
        handle_common_input(key);
    }
    
    // Common input handler - returns true if input was handled
    // Derived classes should call this first, then handle scene-specific input
    bool handle_common_input(Key key) {
        auto& bindings = KeyBindings::instance();
        
        // Movement
        if (bindings.is_action(key, GameAction::MOVE_UP)) {
            move_player(0, -1);
            return true;
        }
        if (bindings.is_action(key, GameAction::MOVE_DOWN)) {
            move_player(0, 1);
            return true;
        }
        if (bindings.is_action(key, GameAction::MOVE_LEFT)) {
            move_player(-1, 0);
            return true;
        }
        if (bindings.is_action(key, GameAction::MOVE_RIGHT)) {
            move_player(1, 0);
            return true;
        }
        
        // Common actions
        if (bindings.is_action(key, GameAction::INVENTORY)) {
            show_inventory();
            return true;
        }
        if (bindings.is_action(key, GameAction::CHARACTER)) {
            show_stats_screen();
            return true;
        }
        if (bindings.is_action(key, GameAction::PAUSE)) {
            // Show pause menu as overlay window
            PauseMenuResult result = show_pause_menu();
            switch (result) {
                case PauseMenuResult::RESUME:
                    // Just return to game
                    break;
                case PauseMenuResult::SAVE_GAME:
                    show_save_game_window();
                    break;
                case PauseMenuResult::SETTINGS:
                    show_settings_menu();
                    break;
                case PauseMenuResult::MAIN_MENU:
                    change_scene(SceneId::MENU);
                    break;
                case PauseMenuResult::QUIT:
                    exit_application();
                    break;
            }
            return true;
        }
        if (bindings.is_action(key, GameAction::HELP)) {
            show_help();
            return true;
        }
        if (bindings.is_action(key, GameAction::INTERACT)) {
            on_interact_action();
            return true;
        }
        if (bindings.is_action(key, GameAction::TALK)) {
            open_interaction_menu();
            return true;
        }
        if (bindings.is_action(key, GameAction::EXAMINE)) {
            examine_nearby();
            return true;
        }
        if (bindings.is_action(key, GameAction::MAP)) {
            on_map_action();
            return true;
        }
        if (key == Key::TAB) {
            show_faction_relations();
            return true;
        }
        if (key == Key::GREATER || bindings.is_action(key, GameAction::CONFIRM)) {
            on_confirm_action();
            return true;
        }
        if (key == Key::LESS) {
            on_secondary_action();
            return true;
        }
        if (key == Key::Q) {
            on_quit_action();
            return true;
        }
        // Ranged attack
        if (bindings.is_action(key, GameAction::RANGED_ATTACK)) {
            if (show_ranged_targeting()) {
                process_turn();
            }
            return true;
        }
        // Throw item
        if (bindings.is_action(key, GameAction::THROW_ITEM)) {
            if (show_throw_item()) {
                process_turn();
            }
            return true;
        }
        // Message log scrolling
        if (bindings.is_action(key, GameAction::SCROLL_MESSAGES_UP)) {
            scroll_messages_up();
            return true;
        }
        if (bindings.is_action(key, GameAction::SCROLL_MESSAGES_DOWN)) {
            scroll_messages_down();
            return true;
        }
        
        return false;  // Input not handled
    }
    
    // ==================== Help System ====================
    // Implemented in gameplay/gameplay_help.inl
#include "gameplay/gameplay_help.inl"

    // ==================== Pause Menu & Message Log ====================
    // Implemented in gameplay/gameplay_menu.inl
#include "gameplay/gameplay_menu.inl"

    // Helper: Get all equipped weapons (delegates to combat system)
    std::vector<EntityId> get_equipped_weapons(EntityId entity_id) {
        return combat_system->get_equipped_weapons(entity_id);
    }
    
    // Generic attack function - attacker attacks target with all weapons (delegates to combat system)
    void perform_attack(EntityId attacker_id, EntityId target_id) {
        bool is_player_attacker = (attacker_id == player_id);
        combat_system->perform_attack(attacker_id, target_id, is_player_attacker);
    }
    
    // Common helper: Find enemy at position (hostile to the player)
    EntityId get_enemy_at(int x, int y) {
        return combat_system->get_enemy_at(x, y, player_id);
    }
    
    // Common combat logic
    void attack_enemy(EntityId enemy_id) {
        combat_system->player_attack_enemy(player_id, enemy_id);
    }
    
    // ==================== Ranged Combat ====================
    // Implemented in gameplay/gameplay_combat.inl
#include "gameplay/gameplay_combat.inl"
    
    // ==================== Entity Transfer System ====================
    // Generalized entity transfer between scenes using the SceneManager's EntityTransferManager
    
    // Queue the player entity for transfer to another scene
    // Call this before changing scenes (e.g., entering/exiting dungeon)
    bool queue_player_for_transfer() {
        if (!transfer_manager || player_id == 0) return false;
        return transfer_manager->queue_player_for_transfer(player_id, manager);
    }
    
    // Queue any entity for transfer with a custom tag
    bool queue_entity_for_transfer(EntityId entity_id, const std::string& tag) {
        if (!transfer_manager || entity_id == 0) return false;
        return transfer_manager->queue_entity_for_transfer(entity_id, manager, tag);
    }
    
    // Queue multiple entities for transfer with a tag prefix
    int queue_entities_for_transfer(const std::vector<EntityId>& entities, const std::string& tag_prefix) {
        if (!transfer_manager) return 0;
        return transfer_manager->queue_entities_for_transfer(entities, manager, tag_prefix);
    }
    
    // Check if a player transfer is pending (to be restored in this scene)
    bool has_player_transfer_pending() const {
        return transfer_manager && transfer_manager->has_player_transfer();
    }
    
    // Restore the player from a pending transfer at the given spawn position
    // Returns the new player EntityId, or 0 if no transfer was pending
    EntityId restore_player_from_transfer(int spawn_x, int spawn_y) {
        if (!transfer_manager || !transfer_manager->has_player_transfer()) return 0;
        
        EntityId new_player_id = transfer_manager->restore_player(manager, spawn_x, spawn_y);
        if (new_player_id != 0) {
            player_id = new_player_id;
            transfer_manager->clear_player_transfer();
        }
        return new_player_id;
    }
    
    // Check if an entity transfer with the given tag is pending
    bool has_entity_transfer_pending(const std::string& tag) const {
        return transfer_manager && transfer_manager->has_pending_transfer(tag);
    }
    
    // Restore an entity from a pending transfer
    EntityId restore_entity_from_transfer(const std::string& tag, 
                                          std::optional<int> x = std::nullopt,
                                          std::optional<int> y = std::nullopt,
                                          std::optional<int> z = std::nullopt) {
        if (!transfer_manager) return 0;
        EntityId id = transfer_manager->restore_transferred_entity(tag, manager, x, y, z);
        if (id != 0) {
            transfer_manager->clear_transfer(tag);
        }
        return id;
    }
    
    // Get all transfer tags matching a prefix
    std::vector<std::string> get_transfer_tags_with_prefix(const std::string& prefix) const {
        if (!transfer_manager) return {};
        return transfer_manager->get_transfer_tags_with_prefix(prefix);
    }
    
    // Clear all pending entity transfers
    void clear_all_entity_transfers() {
        if (transfer_manager) {
            transfer_manager->clear_all_transfers();
        }
    }
    
    // ==================== Player State Persistence ====================
    // Implemented in gameplay/gameplay_persistence.inl
#include "gameplay/gameplay_persistence.inl"

    // ==================== Interaction System Methods ====================
    // Implemented in gameplay/gameplay_interaction.inl
#include "gameplay/gameplay_interaction.inl"

    // ==================== Stats & Inventory Screens ====================
    // Implemented in gameplay/gameplay_ui.inl
#include "gameplay/gameplay_ui.inl"

public:
    
    // Check if player is still alive
    void check_player_death() {
        StatsComponent* player_stats = manager->get_component<StatsComponent>(player_id);
        if (player_stats && !player_stats->is_alive()) {
            change_scene(SceneId::GAME_OVER);
        }
    }
};
