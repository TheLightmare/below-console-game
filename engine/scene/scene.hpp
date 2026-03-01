#pragma once
#include <memory>
#include <string>

// Forward declarations
class Console;
class ComponentManager;
class Renderer;
class UI;
enum class Key;

// Base scene class
class Scene {
protected:
    Console* console;
    ComponentManager* manager;
    Renderer* renderer;
    UI* ui;
    void* user_data = nullptr;  // Optional user data pointer (game-specific state)
    bool finished;
    std::string next_scene;
    bool request_fresh_scene = false;  // Request that the next scene be freshly created
    
public:
    Scene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : console(con), manager(mgr), renderer(rend), ui(ui_sys)
        , finished(false), next_scene("") {}
    
    virtual ~Scene() = default;
    
    // Set optional user data (game-specific state, owned externally)
    void set_user_data(void* data) { user_data = data; }
    void* get_user_data() const { return user_data; }
    
    void enter() {
        finished = false;
        request_fresh_scene = false;
        next_scene = "";
        on_enter();
    }
    
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void update() = 0;
    virtual void handle_input(Key key) = 0;
    virtual void render() = 0;
    
    // Check if scene is finished
    bool is_finished() const { return finished; }
    
    // Get next scene name
    std::string get_next_scene() const { return next_scene; }
    
    // Change to another scene
    void change_scene(const std::string& scene_name) {
        next_scene = scene_name;
        finished = true;
    }
    
    // Change to a fresh (newly created) scene, clearing any cached state
    void change_to_fresh_scene(const std::string& scene_name) {
        next_scene = scene_name;
        request_fresh_scene = true;
        finished = true;
    }
    
    // Check if fresh scene was requested
    bool wants_fresh_scene() const { return request_fresh_scene; }
    
    // Exit application
    void exit_application() {
        next_scene = "";
        finished = true;
    }
};
