#pragma once
#include "menu_window.hpp"
#include <functional>
#include <string>
#include <vector>

// Shared settings menu window that can be used by any scene
// Takes a render callback to render the background scene
namespace SettingsMenu {

using namespace MenuWindow;

enum class Mode {
    MAIN_MENU,
    KEY_BINDINGS,
    REBINDING
};

// Additional theme colors specific to key bindings
namespace KeyTheme {
    inline RGBColor key_normal()   { return RGBColor(180, 180, 180); }
    inline RGBColor key_editing()  { return RGBColor(100, 255, 100); }
}

// Render key bindings overlay
inline void render_key_bindings_overlay(Console* console, UI* ui, 
    std::function<void()> render_background,
    int selected_binding, int binding_scroll, bool editing_primary, bool rebinding) {
    (void)ui; // Reserved for future use
    
    if (render_background) render_background();
    
    auto& bindings = KeyBindings::instance();
    auto categories = bindings.get_actions_by_category();
    
    int panel_width = 60;
    int panel_height = 24;
    int panel_x = (console->get_width() - panel_width) / 2;
    int panel_y = (console->get_height() - panel_height) / 2;
    
    std::string title = rebinding ? "Press a key..." : "KEY BINDINGS";
    draw_themed_box(console, panel_x, panel_y, panel_width, panel_height, title);
    
    // Header
    console->draw_string_rgb(panel_x + 2, panel_y + 3, "Action", Theme::category(), Theme::bg());
    std::string pri_header = editing_primary ? "[PRIMARY]" : "Primary";
    std::string sec_header = !editing_primary ? "[SECONDARY]" : "Secondary";
    console->draw_string_rgb(panel_x + 28, panel_y + 3, pri_header, 
        editing_primary ? Theme::selected() : Theme::normal(), Theme::bg());
    console->draw_string_rgb(panel_x + 43, panel_y + 3, sec_header, 
        !editing_primary ? Theme::selected() : Theme::normal(), Theme::bg());
    
    // Build flat list of actions with their display order
    int y = panel_y + 5;
    int display_idx = 0;
    int action_idx = 0;
    
    for (const auto& cat : categories) {
        // Show category header if visible
        if (display_idx >= binding_scroll && y < panel_y + panel_height - 2) {
            console->draw_string_rgb(panel_x + 2, y++, "-- " + category_to_string(cat.category) + " --", 
                Theme::category(), Theme::bg());
        }
        display_idx++;
        
        for (const auto& action : cat.actions) {
            if (action_idx >= binding_scroll && y < panel_y + panel_height - 2) {
                bool is_selected = (action_idx == selected_binding);
                RGBColor color = is_selected ? Theme::selected() : Theme::normal();
                RGBColor bg = is_selected ? Theme::highlight_bg() : Theme::bg();
                std::string prefix = is_selected ? ">> " : "   ";
                
                // Draw selection highlight bar
                if (is_selected) {
                    for (int dx = 2; dx < panel_width - 2; ++dx) {
                        console->set_cell_rgb(panel_x + dx, y, ' ', color, bg);
                    }
                }
                
                console->draw_string_rgb(panel_x + 2, y, prefix + action_to_string(action), color, bg);
                
                Key pri_key = bindings.get_primary(action);
                Key sec_key = bindings.get_secondary(action);
                
                RGBColor pri_color = (is_selected && editing_primary) ? KeyTheme::key_editing() : KeyTheme::key_normal();
                RGBColor sec_color = (is_selected && !editing_primary) ? KeyTheme::key_editing() : KeyTheme::key_normal();
                
                console->draw_string_rgb(panel_x + 28, y, key_to_string(pri_key), pri_color, bg);
                console->draw_string_rgb(panel_x + 43, y, key_to_string(sec_key), sec_color, bg);
                y++;
            }
            action_idx++;
        }
    }
    
    // Footer - use dynamic keybindings
    std::string confirm_key = key_to_short_string(bindings.get_primary(GameAction::CONFIRM));
    std::string cancel_key = key_to_short_string(bindings.get_primary(GameAction::CANCEL));
    std::string left_key = key_to_short_string(bindings.get_primary(GameAction::MENU_LEFT));
    std::string right_key = key_to_short_string(bindings.get_primary(GameAction::MENU_RIGHT));
    std::string footer = rebinding ? ("[ " + cancel_key + " ] Cancel") : 
        ("[ " + confirm_key + " ] Rebind  [ " + left_key + "/" + right_key + " ] Column  [ R ] Clear  [ " + cancel_key + " ] Back");
    console->draw_string_rgb(panel_x + 2, panel_y + panel_height - 1, footer, Theme::hint(), Theme::bg());
    console->present();
}

// Show the settings window - takes a render callback for the background
inline void show(Console* console, UI* ui, std::function<void()> render_background) {
    std::vector<std::string> options = {"Key Bindings", "Reset to Defaults", "Back"};
    int selected = 0;
    Mode mode = Mode::MAIN_MENU;
    int binding_scroll = 0;
    int selected_binding = 0;
    bool editing_primary = true;
    GameAction rebinding_action = GameAction::ACTION_COUNT;
    
    while (true) {
        auto& bindings = KeyBindings::instance();
        
        if (mode == Mode::MAIN_MENU) {
            // Render background
            if (render_background) render_background();
            
            // Draw settings menu overlay
            int panel_width = 30;
            int panel_height = static_cast<int>(options.size()) + 6;
            int panel_x = (console->get_width() - panel_width) / 2;
            int panel_y = (console->get_height() - panel_height) / 2;
            
            draw_themed_box(console, panel_x, panel_y, panel_width, panel_height, "SETTINGS");
            
            // Draw menu items
            for (size_t i = 0; i < options.size(); ++i) {
                bool is_selected = (static_cast<int>(i) == selected);
                RGBColor color = is_selected ? Theme::selected() : Theme::normal();
                RGBColor bg = is_selected ? Theme::highlight_bg() : Theme::bg();
                std::string prefix = is_selected ? ">> " : "   ";
                std::string suffix = is_selected ? " <<" : "   ";
                
                std::string display = prefix + options[i] + suffix;
                int item_x = panel_x + (panel_width - static_cast<int>(display.length())) / 2;
                
                // Draw selection highlight bar
                if (is_selected) {
                    for (int dx = 2; dx < panel_width - 2; ++dx) {
                        console->set_cell_rgb(panel_x + dx, panel_y + 3 + static_cast<int>(i), ' ', color, bg);
                    }
                }
                
                console->draw_string_rgb(item_x, panel_y + 3 + static_cast<int>(i), display, color, bg);
            }
            
            // Footer hint
            std::string nav = bindings.get_nav_keys();
            std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
            std::string footer = "[ " + nav + " ] Navigate  [ " + confirm + " ] Select";
            int footer_x = panel_x + (panel_width - static_cast<int>(footer.length())) / 2;
            if (footer_x < panel_x + 1) footer_x = panel_x + 1;
            console->draw_string_rgb(footer_x, panel_y + panel_height - 1, footer, Theme::hint(), Theme::bg());
            console->present();
            
            Key key = Input::wait_for_key();
            
            if (bindings.is_action(key, GameAction::MENU_UP)) {
                if (selected > 0) selected--;
            } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
                if (selected < static_cast<int>(options.size()) - 1) selected++;
            } else if (bindings.is_action(key, GameAction::CONFIRM)) {
                switch (selected) {
                    case 0:
                        mode = Mode::KEY_BINDINGS;
                        selected_binding = 0;
                        binding_scroll = 0;
                        break;
                    case 1:
                        KeyBindings::instance().reset_to_defaults();
                        KeyBindings::instance().save();
                        break;
                    case 2:
                        return; // Back
                }
            } else if (bindings.is_action(key, GameAction::CANCEL)) {
                return;
            }
            
        } else if (mode == Mode::KEY_BINDINGS) {
            render_key_bindings_overlay(console, ui, render_background, 
                selected_binding, binding_scroll, editing_primary, false);
            
            Key key = Input::wait_for_key();
            auto actions = bindings.get_all_actions();
            
            if (bindings.is_action(key, GameAction::MENU_UP)) {
                if (selected_binding > 0) {
                    selected_binding--;
                    if (selected_binding < binding_scroll) binding_scroll = selected_binding;
                }
            } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
                if (selected_binding < static_cast<int>(actions.size()) - 1) {
                    selected_binding++;
                    int max_visible = 18;
                    if (selected_binding >= binding_scroll + max_visible) {
                        binding_scroll = selected_binding - max_visible + 1;
                    }
                }
            } else if (bindings.is_action(key, GameAction::MENU_LEFT)) {
                editing_primary = true;
            } else if (bindings.is_action(key, GameAction::MENU_RIGHT)) {
                editing_primary = false;
            } else if (bindings.is_action(key, GameAction::CONFIRM)) {
                rebinding_action = actions[selected_binding];
                mode = Mode::REBINDING;
            } else if (key == Key::R) {
                if (editing_primary) {
                    bindings.set_primary(actions[selected_binding], Key::NONE);
                } else {
                    bindings.set_secondary(actions[selected_binding], Key::NONE);
                }
                bindings.save();
            } else if (bindings.is_action(key, GameAction::CANCEL)) {
                mode = Mode::MAIN_MENU;
            }
            
        } else if (mode == Mode::REBINDING) {
            render_key_bindings_overlay(console, ui, render_background,
                selected_binding, binding_scroll, editing_primary, true);
            
            Key key = Input::wait_for_key();
            
            if (key != Key::NONE && key != Key::UNKNOWN) {
                if (key == Key::ESCAPE) {
                    mode = Mode::KEY_BINDINGS;
                } else {
                    if (editing_primary) {
                        bindings.set_primary(rebinding_action, key);
                    } else {
                        bindings.set_secondary(rebinding_action, key);
                    }
                    bindings.save();
                    mode = Mode::KEY_BINDINGS;
                }
            }
        }
    }
}

} // namespace SettingsMenu
