#include "engine/render/console.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/ui.hpp"
#include "engine/input/input.hpp"
#include "engine/ecs/component_manager.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/scene/scene_manager.hpp"
#include "engine/scene/scene_ids.hpp"
#include "engine/scene/common_scenes.hpp"
#include "engine/world/game_state.hpp"
#include "game/scenes/dungeon_scene.hpp"
#include "game/scenes/exploration_scene.hpp"
#include "game/scenes/structure_debug_scene.hpp"
#include "game/entities/effect_registry_init.hpp"
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

class Game {
private:
    Console console;
    ComponentManager manager;
    Renderer renderer;
    UI ui;
    GameState game_state;  // Centralized game state (replaces DungeonState singleton)
    SceneManager scene_manager;
    bool running;
    
public:
    Game() : console(120, 35), renderer(&console, &manager), ui(&console, &manager), running(true) {
        // Inject game state into scene manager before registering scenes
        scene_manager.set_game_state(&game_state);
        register_scenes();
        scene_manager.load_scene(SceneId::MENU);
    }
    
    void register_scenes() {
        // Register all game scenes
        scene_manager.register_scene<MainMenuScene>(SceneId::MENU, 
            [this]() -> std::unique_ptr<Scene> {
                return std::make_unique<MainMenuScene>(&console, &manager, &renderer, &ui);
            });
        
        scene_manager.register_scene<DungeonScene>(SceneId::DUNGEON, 
            [this]() -> std::unique_ptr<Scene> {
                return std::make_unique<DungeonScene>(&console, &manager, &renderer, &ui);
            });
        
        scene_manager.register_scene<ExplorationScene>(SceneId::EXPLORATION, 
            [this]() -> std::unique_ptr<Scene> {
                return std::make_unique<ExplorationScene>(&console, &manager, &renderer, &ui);
            });
        
        // Mark gameplay scenes as persistent so they survive pause/resume
        scene_manager.set_persistent(SceneId::EXPLORATION);
        scene_manager.set_persistent(SceneId::DUNGEON);
        
        scene_manager.register_scene<GameOverScene>(SceneId::GAME_OVER, 
            [this]() -> std::unique_ptr<Scene> {
                return std::make_unique<GameOverScene>(&console, &manager, &renderer, &ui, false);
            });
        
        scene_manager.register_scene<StructureDebugScene>(SceneId::STRUCTURE_DEBUG, 
            [this]() -> std::unique_ptr<Scene> {
                return std::make_unique<StructureDebugScene>(&console, &renderer, &ui);
            });
    }
    
    void run() {
        while (running && scene_manager.has_active_scene()) {
            // Get input
            Key key = Input::get_key();
            
            // Handle input in current scene
            scene_manager.handle_input(key);
            
            // Update current scene
            scene_manager.update();
            
            // Render current scene
            scene_manager.render();
            
            SLEEP_MS(16); // ~60 FPS
        }
    }
};

int main(int argc, char** argv) {
    (void)argc;  // Unused
    (void)argv;  // Unused
    
    // Initialize the effect registry before any game systems
    init_effect_registry();
    
    Game game;
    game.run();
    return 0;
}