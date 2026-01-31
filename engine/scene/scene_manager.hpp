#pragma once
#include "scene.hpp"
#include "../ecs/entity_transfer.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

// Forward declaration
struct GameState;

class SceneManager {
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>()>> scene_factories;
    std::unordered_map<std::string, std::unique_ptr<Scene>> persistent_scenes;  // Cached scenes
    std::unordered_set<std::string> persistent_scene_names;  // Which scenes to keep alive
    Scene* current_scene = nullptr;
    std::string current_scene_name;
    GameState* game_state = nullptr;  // Shared game state (owned externally, typically by Game)
    EntityTransferManager transfer_manager;  // Manages entity transfers between scenes
    
public:
    SceneManager() : current_scene(nullptr), current_scene_name("") {}
    
    // Set the shared game state - must be called before loading scenes
    void set_game_state(GameState* state) { game_state = state; }
    
    // Register a scene factory
    template<typename T>
    void register_scene(const std::string& name, std::function<std::unique_ptr<Scene>()> factory) {
        scene_factories[name] = factory;
    }
    
    // Mark a scene as persistent (keeps state between visits)
    void set_persistent(const std::string& name) {
        persistent_scene_names.insert(name);
    }
    
    // Load and start a scene
    bool load_scene(const std::string& name) {
        auto it = scene_factories.find(name);
        if (it == scene_factories.end()) {
            return false;
        }
        
        // Exit current scene
        if (current_scene) {
            current_scene->on_exit();
        }
        
        // Check if this scene should be persistent
        bool is_persistent = persistent_scene_names.count(name) > 0;
        
        if (is_persistent) {
            // Check if we already have this scene cached
            auto cached_it = persistent_scenes.find(name);
            if (cached_it != persistent_scenes.end()) {
                // Reuse existing scene
                current_scene = cached_it->second.get();
            } else {
                // Create and cache new scene
                persistent_scenes[name] = it->second();
                current_scene = persistent_scenes[name].get();
                // Inject game state and transfer manager into newly created scene
                if (game_state) {
                    current_scene->set_game_state(game_state);
                }
                current_scene->set_transfer_manager(&transfer_manager);
            }
        } else {
            // Non-persistent scene - clear old cache if any and create fresh
            persistent_scenes.erase(name);
            persistent_scenes[name] = it->second();
            current_scene = persistent_scenes[name].get();
            // Inject game state and transfer manager into newly created scene
            if (game_state) {
                current_scene->set_game_state(game_state);
            }
            current_scene->set_transfer_manager(&transfer_manager);
        }
        
        current_scene_name = name;
        current_scene->enter();  // Use enter() to reset flags before on_enter()
        
        return true;
    }
    
    // Clear a persistent scene's cache (forces recreation next time)
    void clear_scene_cache(const std::string& name) {
        persistent_scenes.erase(name);
    }
    
    // Clear all scene caches
    void clear_all_caches() {
        persistent_scenes.clear();
        current_scene = nullptr;
    }
    
    // Update current scene
    void update() {
        if (current_scene) {
            current_scene->update();
            
            // Check if scene wants to change
            if (current_scene->is_finished()) {
                std::string next = current_scene->get_next_scene();
                if (!next.empty()) {
                    // Check if fresh scene was requested
                    if (current_scene->wants_fresh_scene()) {
                        clear_scene_cache(next);
                    }
                    load_scene(next);
                } else {
                    current_scene = nullptr;
                }
            }
        }
    }
    
    // Handle input for current scene
    void handle_input(Key key) {
        if (current_scene) {
            current_scene->handle_input(key);
        }
    }
    
    // Render current scene
    void render() {
        if (current_scene) {
            current_scene->render();
        }
    }
    
    // Check if there's an active scene
    bool has_active_scene() const {
        return current_scene != nullptr;
    }
    
    // Get current scene name
    std::string get_current_scene_name() const {
        return current_scene_name;
    }
    
    // ==================== Entity Transfer System ====================
    
    // Get the entity transfer manager for cross-scene entity migration
    EntityTransferManager& get_transfer_manager() {
        return transfer_manager;
    }
    
    const EntityTransferManager& get_transfer_manager() const {
        return transfer_manager;
    }
    
    // Convenience: Queue player entity for transfer to next scene
    bool transfer_player(EntityId player_id, ComponentManager* manager) {
        return transfer_manager.queue_player_for_transfer(player_id, manager);
    }
    
    // Convenience: Check if there's a player waiting to be restored
    bool has_player_to_restore() const {
        return transfer_manager.has_player_transfer();
    }
    
    // Convenience: Restore player entity from transfer
    EntityId restore_player(ComponentManager* manager, int spawn_x, int spawn_y) {
        return transfer_manager.restore_player(manager, spawn_x, spawn_y);
    }
    
    // Convenience: Clear player transfer after restoration
    void clear_player_transfer() {
        transfer_manager.clear_player_transfer();
    }
};
