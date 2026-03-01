#pragma once
#include "console.hpp"
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../input/input.hpp"
#include <string>
#include <vector>
#include <functional>

// General-purpose UI class for console-based games.
// Provides drawing primitives: panels, text, health bars, menus, message logs,
// confirmation dialogs, and entity selection windows.
class UI {
private:
    Console* console;
    ComponentManager* manager;
    
public:
    UI(Console* con, ComponentManager* mgr) : console(con), manager(mgr) {}
    
    // Draw a panel/window with optional title
    void draw_panel(int x, int y, int width, int height, const std::string& title = "", 
                   Color fg = Color::WHITE, Color bg = Color::BLACK);
    
    // Draw text with word wrap
    void draw_text(int x, int y, int max_width, const std::string& text, 
                  Color fg = Color::WHITE, Color bg = Color::BLACK);
    
    // Draw a health/progress bar
    void draw_health_bar(int x, int y, int width, int current, int max, 
                        Color full_color = Color::GREEN, Color empty_color = Color::RED);
    
    // Draw message log (scroll_offset = 0 shows latest messages, positive values scroll up)
    void draw_message_log(int x, int y, int width, int height, const std::vector<std::string>& messages, int scroll_offset = 0);
    
    // Draw a menu with selectable options
    int draw_menu(int x, int y, const std::string& title, const std::vector<std::string>& options, 
                  int selected_index);
    
    // Show entity selection when multiple entities are nearby
    EntityId show_entity_selection(const std::vector<EntityId>& entities, 
                                   std::function<void()> render_background = nullptr);
    
    // Simple confirmation dialog (Y/N)
    bool confirm_action(const std::string& message);
};
