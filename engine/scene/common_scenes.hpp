#pragma once
#include "scene.hpp"
#include "scene_ids.hpp"
#include "menu_window.hpp"
#include "settings_menu.hpp"
#include "save_load_window.hpp"
#include "new_game_window.hpp"
#include "../input/input.hpp"
#include "../render/console.hpp"
#include "../render/renderer.hpp"
#include "../render/ui.hpp"
#include "../ecs/component_manager.hpp"
#include "../util/save_system.hpp"
#include "../world/dungeon_state.hpp"
#include <vector>
#include <string>
#include <fstream>

// Common scene implementations for the game
// Main Menu Scene - standalone scene with window-based sub-menus
class MainMenuScene : public Scene {
private:
    std::vector<std::string> title_art;
    std::vector<std::string> menu_items;
    int selected_index;
    
public:
    MainMenuScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys)
        : Scene(con, mgr, rend, ui_sys), selected_index(0) {
        menu_items = {"New Game", "Load Game", "Settings", "Quit"};
    }
    
    void on_enter() override {
        selected_index = 0;
        load_title_art();
        // Clear entire console and present immediately to remove any remnants from previous scene
        console->clear();
        console->present();
    }
    
    void load_title_art() {
        title_art.clear();
        std::ifstream file("assets/title.txt");
        
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                title_art.push_back(line);
            }
            file.close();
        } else {
            // Fallback if file not found
            title_art.push_back("DUNGEON CRAWLER");
        }
    }
    
    void update() override {
        // Main menu doesn't need update logic
    }
    
    void handle_input(Key key) override {
        auto& bindings = KeyBindings::instance();
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            navigate_up();
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            navigate_down();
        } else if (bindings.is_action(key, GameAction::CONFIRM)) {
            handle_selection(selected_index);
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            exit_application();
        }
    }
    
    void navigate_up() {
        if (!menu_items.empty()) {
            selected_index = (selected_index - 1 + static_cast<int>(menu_items.size())) % static_cast<int>(menu_items.size());
        }
    }
    
    void navigate_down() {
        if (!menu_items.empty()) {
            selected_index = (selected_index + 1) % static_cast<int>(menu_items.size());
        }
    }
    
    void handle_selection(int index) {
        switch (index) {
            case 0: // New Game
                show_new_game_window();
                break;
            case 1: // Load Game
                show_load_game_window();
                break;
            case 2: // Settings
                show_settings_window();
                break;
            case 3: // Quit
                exit_application();
                break;
        }
    }
    
    // New game window
    void show_new_game_window() {
        auto result = NewGameWindow::show(console, [this]() { render(); });
        
        switch (result.mode) {
            case NewGameWindow::NewGameResult::Mode::EXPLORATION:
                change_to_fresh_scene("exploration");
                break;
            case NewGameWindow::NewGameResult::Mode::STRUCTURE_DEBUG:
                change_to_fresh_scene("structure_debug");
                break;
            default:
                // Cancelled or none - stay on main menu
                break;
        }
    }
    
    // Load game window
    void show_load_game_window() {
        auto result = SaveLoadWindow::show_load_window(console, game_state, [this]() { render(); }, transfer_manager);
        
        if (result.action == SaveLoadWindow::SaveLoadResult::Action::LOADED) {
            if (result.fresh_scene) {
                change_to_fresh_scene(result.next_scene);
            } else {
                change_scene(result.next_scene);
            }
        }
    }
    
    // Settings window - uses shared settings menu
    void show_settings_window() {
        SettingsMenu::show(console, ui, [this]() { render(); });
    }

    void render() override {
        console->clear();
        
        int width = console->get_width();
        int height = console->get_height();
        
        // Draw decorative border
        draw_decorative_border(width, height);
        
        // Draw ASCII art title with smooth red to orange RGB gradient
        if (!title_art.empty()) {
            int start_y = 3;
            
            for (size_t i = 0; i < title_art.size(); ++i) {
                const std::string& line = title_art[i];
                int title_x = (width - static_cast<int>(line.length())) / 2;
                if (title_x < 0) title_x = 0;
                
                // Calculate RGB gradient from bright red to orange
                float progress = static_cast<float>(i) / static_cast<float>(title_art.size());
                unsigned char red = 255;
                unsigned char green = static_cast<unsigned char>(100 * progress);
                unsigned char blue = 0;
                
                RGBColor gradient_color(red, green, blue);
                console->draw_string_rgb(title_x, start_y + static_cast<int>(i), line, gradient_color, RGBColor(0, 0, 0));
            }
            
            // Draw an invisible character with regular colors to reset terminal state
            console->draw_string(0, start_y + static_cast<int>(title_art.size()), " ", Color::WHITE, Color::BLACK);
        }
        
        // Draw subtitle
        int subtitle_y = 3 + static_cast<int>(title_art.size()) + 1;
        std::string subtitle = "~ The Forgotten World ~";
        int subtitle_x = (width - static_cast<int>(subtitle.length())) / 2;
        console->draw_string_rgb(subtitle_x, subtitle_y, subtitle, RGBColor(200, 80, 40), RGBColor(0, 0, 0));
        
        // Draw fancy menu box
        int menu_y = subtitle_y + 3;
        int menu_width = 30;
        int menu_height = static_cast<int>(menu_items.size()) + 4;
        int menu_x = (width - menu_width) / 2;
        
        // Draw menu frame with themed box
        RGBColor border_color(220, 80, 50);
        RGBColor separator_color(150, 60, 40);
        console->draw_box_rgb(menu_x, menu_y, menu_width, menu_height, border_color, RGBColor(0, 0, 0),
                              '=', '!', '@', '@', '@', '@');
        
        // Separator line under title
        console->set_cell_rgb(menu_x, menu_y + 2, '@', border_color, RGBColor(0, 0, 0));
        console->set_cell_rgb(menu_x + menu_width - 1, menu_y + 2, '@', border_color, RGBColor(0, 0, 0));
        for (int i = 1; i < menu_width - 1; ++i) {
            console->set_cell_rgb(menu_x + i, menu_y + 2, ':', separator_color, RGBColor(0, 0, 0));
        }
        
        // Menu title
        std::string menu_title = "MAIN MENU";
        int title_x = menu_x + (menu_width - static_cast<int>(menu_title.length())) / 2;
        console->draw_string_rgb(title_x, menu_y + 1, menu_title, RGBColor(255, 100, 50), RGBColor(0, 0, 0));
        
        // Draw menu items
        for (size_t i = 0; i < menu_items.size(); ++i) {
            RGBColor color = (static_cast<int>(i) == selected_index) ? RGBColor(255, 200, 100) : RGBColor(200, 150, 150);
            RGBColor bg = (static_cast<int>(i) == selected_index) ? RGBColor(60, 20, 20) : RGBColor(0, 0, 0);
            std::string prefix = (static_cast<int>(i) == selected_index) ? ">> " : "   ";
            std::string suffix = (static_cast<int>(i) == selected_index) ? " <<" : "   ";
            
            std::string display = prefix + menu_items[i] + suffix;
            int item_x = menu_x + (menu_width - static_cast<int>(display.length())) / 2;
            
            // Add extra spacing for selected item
            if (static_cast<int>(i) == selected_index) {
                // Draw selection highlight bar
                for (int dx = 2; dx < menu_width - 2; ++dx) {
                    console->set_cell_rgb(menu_x + dx, menu_y + 3 + static_cast<int>(i), ' ', color, bg);
                }
            }
            
            console->draw_string_rgb(item_x, menu_y + 3 + static_cast<int>(i), display, color, bg);
        }
        
        // Version info
        console->draw_string_rgb(2, height - 1, "v0.1.0 Alpha", RGBColor(80, 60, 60), RGBColor(0, 0, 0));
        
        // Controls hint with dynamic keybindings
        auto& bindings = KeyBindings::instance();
        std::string nav = bindings.get_nav_keys();
        std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
        std::string cancel = bindings.get_key_hint(GameAction::CANCEL);
        console->draw_string_rgb(width / 2 - 22, height - 3, "[ " + nav + " ] Navigate", RGBColor(150, 100, 100), RGBColor(0, 0, 0));
        console->draw_string_rgb(width / 2 - 18, height - 2, "[ " + confirm + " ] Select", RGBColor(150, 100, 100), RGBColor(0, 0, 0));
        console->draw_string_rgb(width / 2 + 5, height - 2, "[ " + cancel + " ] Quit", RGBColor(150, 100, 100), RGBColor(0, 0, 0));
        
        console->present();
    }

private:
    void draw_decorative_border(int width, int height) {
        // Draw corner decorations with red theme
        console->draw_string_rgb(0, 0, "@", RGBColor(200, 50, 30), RGBColor(0, 0, 0));
        console->draw_string_rgb(width - 1, 0, "@", RGBColor(200, 50, 30), RGBColor(0, 0, 0));
        console->draw_string_rgb(0, height - 1, "@", RGBColor(200, 50, 30), RGBColor(0, 0, 0));
        console->draw_string_rgb(width - 1, height - 1, "@", RGBColor(200, 50, 30), RGBColor(0, 0, 0));
        
        // Top and bottom decorative lines with alternating characters
        for (int x = 1; x < width - 1; ++x) {
            if (x % 3 == 0) {
                console->draw_string_rgb(x, 0, ":", RGBColor(180, 60, 40), RGBColor(0, 0, 0));
                console->draw_string_rgb(x, height - 1, ":", RGBColor(180, 60, 40), RGBColor(0, 0, 0));
            } else {
                console->draw_string_rgb(x, 0, "!", RGBColor(120, 50, 40), RGBColor(0, 0, 0));
                console->draw_string_rgb(x, height - 1, "!", RGBColor(120, 50, 40), RGBColor(0, 0, 0));
            }
        }
    }
};

// Game Over Scene
class GameOverScene : public Scene {
private:
    bool victory;
    std::string death_reason;
    
public:
    GameOverScene(Console* con, ComponentManager* mgr, Renderer* rend, UI* ui_sys, bool won = false)
        : Scene(con, mgr, rend, ui_sys), victory(won) {}
    
    void set_victory(bool won) { victory = won; }
    
    void on_enter() override {
        // Read death reason from game state if available
        if (game_state && !victory) {
            death_reason = game_state->dungeon.death_reason;
        } else {
            death_reason.clear();
        }
    }
    
    void update() override {}
    
    void handle_input(Key key) override {
        if (key != Key::NONE) {
            change_scene(SceneId::MENU);
        }
    }
    
    void render() override {
        console->clear();
        ui->draw_game_over(victory, death_reason);
        console->present();
    }
};
