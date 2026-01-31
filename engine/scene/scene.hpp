#pragma once
#include <memory>
#include <string>

// Forward declarations
class Console;
class ComponentManager;
class Renderer;
class UI;
class EntityTransferManager;
struct GameState;
enum class Key;

// Base scene class
class Scene {
protected:
    Console* console;
    ComponentManager* manager;
    Renderer* renderer;
    UI* ui;
    GameState* game_state;  // Shared game state (owned by Game, managed by SceneManager)
    EntityTransferManager* transfer_manager = nullptr;  // For entity transfers between scenes
    bool finished;
    std::string next_scene;
    bool request_fresh_scene = false;  // Request that the next scene be freshly created
    
public:
    Scene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : console(con), manager(mgr), renderer(rend), ui(ui_sys), game_state(nullptr)
        , finished(false), next_scene("") {}
    
    virtual ~Scene() = default;
    
    // Set the shared game state (called by SceneManager)
    void set_game_state(GameState* state) { game_state = state; }
    
    // Set the entity transfer manager (called by SceneManager)
    void set_transfer_manager(EntityTransferManager* tm) { transfer_manager = tm; }
    
    // Get the entity transfer manager for cross-scene entity transfers
    EntityTransferManager* get_transfer_manager() { return transfer_manager; }
    
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
