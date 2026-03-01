// Console Engine - Minimal Demo
// This demonstrates the core engine systems: Console, ECS, Renderer, Scenes, Input.

#include "engine/render/console.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/ui.hpp"
#include "engine/input/input.hpp"
#include "engine/ecs/component_manager.hpp"
#include "engine/ecs/entity.hpp"
#include "engine/scene/scene_manager.hpp"
#include "engine/world/world.hpp"
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

// A simple demo scene that creates a small multi-level world with a movable entity
class DemoScene : public Scene {
private:
    // 3-level world: underground (z=0), ground (z=1), sky/upper floor (z=2)
    World world;
    EntityId player_id = 0;
    std::vector<std::string> messages;

public:
    DemoScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : Scene(con, mgr, rend, ui_sys), world(60, 25, 3) {}
    
    void on_enter() override {
        // Clear previous state
        manager->clear_all_entities();
        messages.clear();
        
        // === Z-Level 0: Underground cave ===
        for (int y = 0; y < world.get_height(); ++y) {
            for (int x = 0; x < world.get_width(); ++x) {
                Tile& tile = world.get_tile(x, y, 0);
                tile.symbol = '#';
                tile.color = "dark_gray";
                tile.walkable = false;
                tile.has_floor = true;
                tile.transparent = false;
                tile.below_symbol = '#';
                tile.below_color = "dark_gray";
            }
        }
        // Carve out a cave room
        for (int y = 8; y < 20; ++y) {
            for (int x = 5; x < 35; ++x) {
                Tile& tile = world.get_tile(x, y, 0);
                tile.symbol = '.';
                tile.color = "dark_cyan";
                tile.walkable = true;
                tile.below_symbol = '.';
                tile.below_color = "dark_cyan";
            }
        }
        // Cave water pool
        for (int y = 12; y < 16; ++y) {
            for (int x = 20; x < 28; ++x) {
                Tile& tile = world.get_tile(x, y, 0);
                tile.symbol = '~';
                tile.color = "blue";
                tile.walkable = false;
                tile.below_symbol = '~';
                tile.below_color = "dark_blue";
            }
        }
        
        // === Z-Level 1: Surface / ground ===
        for (int y = 0; y < world.get_height(); ++y) {
            for (int x = 0; x < world.get_width(); ++x) {
                Tile& tile = world.get_tile(x, y, 1);
                tile.symbol = '.';
                tile.color = "green";
                tile.walkable = true;
                tile.has_floor = true;
                tile.transparent = false;
                tile.below_symbol = '.';
                tile.below_color = "dark_green";
            }
        }
        // Some walls / buildings
        for (int x = 10; x < 20; ++x) {
            Tile& tile = world.get_tile(x, 10, 1);
            tile.symbol = '#';
            tile.color = "gray";
            tile.walkable = false;
        }
        for (int x = 10; x < 20; ++x) {
            Tile& tile = world.get_tile(x, 14, 1);
            tile.symbol = '#';
            tile.color = "gray";
            tile.walkable = false;
        }
        for (int y = 10; y <= 14; ++y) {
            world.get_tile(10, y, 1) = Tile(false, '#', "gray");
            world.get_tile(19, y, 1) = Tile(false, '#', "gray");
        }
        // Door
        world.get_tile(14, 14, 1) = Tile(true, '+', "yellow");
        
        // Trees
        for (int tx : {3, 7, 25, 30, 40, 45, 50}) {
            for (int ty : {3, 6, 18, 22}) {
                if (tx < world.get_width() && ty < world.get_height()) {
                    Tile& tile = world.get_tile(tx, ty, 1);
                    tile.symbol = 'T';
                    tile.color = "green";
                    tile.walkable = false;
                }
            }
        }
        
        // === Z-Level 2: Upper floor / treetops ===
        for (int y = 0; y < world.get_height(); ++y) {
            for (int x = 0; x < world.get_width(); ++x) {
                Tile& tile = world.get_tile(x, y, 2);
                tile.symbol = ' ';
                tile.color = "white";
                tile.walkable = false;
                tile.has_floor = false;   // Open air - see through to ground below
                tile.transparent = true;
            }
        }
        // Building upper floor (above the walls on z=1)
        for (int y = 10; y <= 14; ++y) {
            for (int x = 10; x < 20; ++x) {
                Tile& tile = world.get_tile(x, y, 2);
                tile.symbol = '.';
                tile.color = "dark_yellow";
                tile.walkable = true;
                tile.has_floor = true;
                tile.transparent = false;
                tile.below_symbol = '.';
                tile.below_color = "dark_yellow";
            }
        }
        
        // === Stairs connecting levels ===
        // Stairs from underground (z=0) up to ground (z=1) at (10, 14)
        {
            Tile& s0 = world.get_tile(10, 15, 0);
            s0.symbol = '<';
            s0.color = "white";
            s0.walkable = true;
            s0.is_stairs_up = true;
            
            Tile& s1 = world.get_tile(10, 15, 1);
            s1.symbol = '>';
            s1.color = "white";
            s1.walkable = true;
            s1.is_stairs_down = true;
        }
        
        // Stairs from ground (z=1) up to building roof (z=2) at (18, 12)
        {
            Tile& s1 = world.get_tile(18, 12, 1);
            s1.symbol = '<';
            s1.color = "white";
            s1.walkable = true;
            s1.is_stairs_up = true;
            
            Tile& s2 = world.get_tile(18, 12, 2);
            s2.symbol = '>';
            s2.color = "white";
            s2.walkable = true;
            s2.is_stairs_down = true;
            s2.has_floor = true;
        }
        
        // Create player entity on the ground floor (z=1)
        player_id = manager->create_entity();
        manager->add_component<PositionComponent>(player_id, 5, 5, 1);
        manager->add_component<RenderComponent>(player_id, '@', "yellow", "", 100);
        manager->add_component<VelocityComponent>(player_id, 0, 0, 1.0f);
        
        // Underground NPC (z=0)
        EntityId miner = manager->create_entity();
        manager->add_component<PositionComponent>(miner, 15, 12, 0);
        manager->add_component<RenderComponent>(miner, 'M', "cyan", "", 50);
        
        // Surface NPC (z=1)
        EntityId guard = manager->create_entity();
        manager->add_component<PositionComponent>(guard, 25, 15, 1);
        manager->add_component<RenderComponent>(guard, 'G', "white", "", 50);
        
        // Rooftop item (z=2)
        EntityId flag = manager->create_entity();
        manager->add_component<PositionComponent>(flag, 14, 12, 2);
        manager->add_component<RenderComponent>(flag, 'P', "red", "", 50);
        
        messages.push_back("Welcome to the 3D Console Engine demo!");
        messages.push_back("WASD/arrows: move | </>: change z-level");
        messages.push_back("Z=0 underground, Z=1 ground, Z=2 upper");
        messages.push_back("Press ESC to quit.");
    }
    
    void handle_input(Key key) override {
        auto& bindings = KeyBindings::instance();
        
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        if (!pos) return;
        
        int dx = 0, dy = 0;
        if (bindings.is_action(key, GameAction::MOVE_UP)) dy = -1;
        else if (bindings.is_action(key, GameAction::MOVE_DOWN)) dy = 1;
        else if (bindings.is_action(key, GameAction::MOVE_LEFT)) dx = -1;
        else if (bindings.is_action(key, GameAction::MOVE_RIGHT)) dx = 1;
        else if (bindings.is_action(key, GameAction::MOVE_UP_LAYER)) {
            // Go up a z-level (need stairs-up or ramp at current position)
            if (world.can_go_up(pos->x, pos->y, pos->z)) {
                pos->z += 1;
                messages.push_back("You ascend to z-level " + std::to_string(pos->z) + ".");
            } else {
                messages.push_back("You can't go up here.");
            }
            return;
        }
        else if (bindings.is_action(key, GameAction::MOVE_DOWN_LAYER)) {
            // Go down a z-level (need stairs-down at current position, or stairs-up below)
            if (world.can_go_down(pos->x, pos->y, pos->z)) {
                pos->z -= 1;
                messages.push_back("You descend to z-level " + std::to_string(pos->z) + ".");
            } else {
                messages.push_back("You can't go down here.");
            }
            return;
        }
        else if (bindings.is_action(key, GameAction::CANCEL)) {
            exit_application();
            return;
        }
        
        if (dx != 0 || dy != 0) {
            int nx = pos->x + dx;
            int ny = pos->y + dy;
            if (nx >= 0 && nx < world.get_width() && ny >= 0 && ny < world.get_height()) {
                if (world.get_tile(nx, ny, pos->z).walkable) {
                    pos->x = nx;
                    pos->y = ny;
                }
            }
        }
    }
    
    void update() override {}
    
    void render() override {
        PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
        int view_z = pos ? pos->z : 1;
        
        renderer->set_camera(0, 0, view_z);
        renderer->render_all(&world);
        
        // Show z-level indicator
        std::string z_label = " Z:" + std::to_string(view_z) + " ";
        console->draw_string(console->get_width() - (int)z_label.size() - 1, 0, z_label, Color::YELLOW, Color::DARK_BLUE);
        
        // Draw a small message log at the bottom
        ui->draw_message_log(0, world.get_height(), console->get_width(), 5, messages);
        
        renderer->present();
    }
};

class EngineDemo {
private:
    Console console;
    ComponentManager manager;
    Renderer renderer;
    UI ui;
    SceneManager scene_manager;
    bool running;
    
public:
    EngineDemo() : console(80, 35), renderer(&console, &manager), ui(&console, &manager), running(true) {
        scene_manager.register_scene<DemoScene>("demo",
            [this]() -> std::unique_ptr<Scene> {
                return std::make_unique<DemoScene>(&console, &manager, &renderer, &ui);
            });
        scene_manager.load_scene("demo");
    }
    
    void run() {
        while (running && scene_manager.has_active_scene()) {
            Key key = Input::get_key();
            scene_manager.handle_input(key);
            scene_manager.update();
            scene_manager.render();
            SLEEP_MS(16); // ~60 FPS
        }
    }
};

int main() {
    EngineDemo demo;
    demo.run();
    return 0;
}