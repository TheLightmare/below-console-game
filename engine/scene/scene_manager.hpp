#pragma once
#include "scene.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

class SceneManager {
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>()>> scene_factories;
    std::unordered_map<std::string, std::unique_ptr<Scene>> persistent_scenes;  // Cached scenes
    std::unordered_set<std::string> persistent_scene_names;  // Which scenes to keep alive
    Scene* current_scene = nullptr;
    std::string current_scene_name;
    void* user_data = nullptr;  // Optional user data propagated to scenes
    
public:
    SceneManager() : current_scene(nullptr), current_scene_name("") {}
    
    // Set optional user data - propagated to scenes on load
    void set_user_data(void* data) { user_data = data; }
    
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
                // Inject user data into newly created scene
                if (user_data) {
                    current_scene->set_user_data(user_data);
                }
            }
        } else {
            // Non-persistent scene - clear old cache if any and create fresh
            persistent_scenes.erase(name);
            persistent_scenes[name] = it->second();
            current_scene = persistent_scenes[name].get();
            // Inject user data into newly created scene
            if (user_data) {
                current_scene->set_user_data(user_data);
            }
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
    
};
