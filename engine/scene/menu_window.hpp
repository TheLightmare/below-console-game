#pragma once
#include "../render/console.hpp"
#include "../render/ui.hpp"
#include "../input/input.hpp"
#include <functional>
#include <string>
#include <vector>

// Abstract base class for menu windows (overlays on top of scenes)
// Similar pattern to SettingsMenu but reusable for any menu

namespace MenuWindow {

// Theme colors matching main menu style
namespace Theme {
    inline RGBColor border()       { return RGBColor(220, 80, 50); }
    inline RGBColor separator()    { return RGBColor(150, 60, 40); }
    inline RGBColor title()        { return RGBColor(255, 100, 50); }
    inline RGBColor selected()     { return RGBColor(255, 200, 100); }
    inline RGBColor normal()       { return RGBColor(200, 150, 150); }
    inline RGBColor hint()         { return RGBColor(150, 100, 100); }
    inline RGBColor highlight_bg() { return RGBColor(60, 20, 20); }
    inline RGBColor category()     { return RGBColor(180, 100, 80); }
    inline RGBColor bg()           { return RGBColor(0, 0, 0); }
}

// Draw themed box matching main menu style
inline void draw_themed_box(Console* console, int x, int y, int width, int height, const std::string& title) {
    // Fill background first to make it opaque
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            console->set_cell_rgb(x + dx, y + dy, ' ', Theme::normal(), Theme::bg());
        }
    }
    
    // Draw box frame
    console->draw_box_rgb(x, y, width, height, Theme::border(), Theme::bg(), '=', '!', '@', '@', '@', '@');
    
    // Separator line under title
    console->set_cell_rgb(x, y + 2, '@', Theme::border(), Theme::bg());
    console->set_cell_rgb(x + width - 1, y + 2, '@', Theme::border(), Theme::bg());
    for (int i = 1; i < width - 1; ++i) {
        console->set_cell_rgb(x + i, y + 2, ':', Theme::separator(), Theme::bg());
    }
    
    // Title
    int title_x = x + (width - static_cast<int>(title.length())) / 2;
    console->draw_string_rgb(title_x, y + 1, title, Theme::title(), Theme::bg());
}

// Draw themed decorative border on full screen
inline void draw_themed_border(Console* console, int width, int height) {
    // Draw corner decorations with red theme
    console->draw_string_rgb(0, 0, "@", RGBColor(200, 50, 30), Theme::bg());
    console->draw_string_rgb(width - 1, 0, "@", RGBColor(200, 50, 30), Theme::bg());
    console->draw_string_rgb(0, height - 1, "@", RGBColor(200, 50, 30), Theme::bg());
    console->draw_string_rgb(width - 1, height - 1, "@", RGBColor(200, 50, 30), Theme::bg());
    
    // Top and bottom decorative lines with alternating characters
    for (int x = 1; x < width - 1; ++x) {
        if (x % 3 == 0) {
            console->draw_string_rgb(x, 0, ":", RGBColor(180, 60, 40), Theme::bg());
            console->draw_string_rgb(x, height - 1, ":", RGBColor(180, 60, 40), Theme::bg());
        } else {
            console->draw_string_rgb(x, 0, "!", RGBColor(120, 50, 40), Theme::bg());
            console->draw_string_rgb(x, height - 1, "!", RGBColor(120, 50, 40), Theme::bg());
        }
    }
}

// Result from showing a menu
struct MenuResult {
    int selected_index = -1;  // -1 means cancelled/back
    bool cancelled = false;
};

// Show a simple selection menu as an overlay window
// Returns the selected index, or -1 if cancelled
inline MenuResult show_selection_menu(
    Console* console,
    const std::string& title,
    const std::vector<std::string>& options,
    std::function<void()> render_background = nullptr,
    int initial_selection = 0,
    int panel_width = 30
) {
    int selected = initial_selection;
    auto& bindings = KeyBindings::instance();
    
    while (true) {
        // Render background if provided
        if (render_background) {
            render_background();
        } else {
            console->clear();
        }
        
        // Calculate panel dimensions
        int panel_height = static_cast<int>(options.size()) + 6;
        int panel_x = (console->get_width() - panel_width) / 2;
        int panel_y = (console->get_height() - panel_height) / 2;
        
        // Draw themed box
        draw_themed_box(console, panel_x, panel_y, panel_width, panel_height, title);
        
        // Draw menu items
        for (size_t i = 0; i < options.size(); ++i) {
            bool is_selected = (static_cast<int>(i) == selected);
            RGBColor color = is_selected ? Theme::selected() : Theme::normal();
            RGBColor item_bg = is_selected ? Theme::highlight_bg() : Theme::bg();
            std::string prefix = is_selected ? ">> " : "   ";
            std::string suffix = is_selected ? " <<" : "   ";
            
            std::string display = prefix + options[i] + suffix;
            int item_x = panel_x + (panel_width - static_cast<int>(display.length())) / 2;
            
            // Draw selection highlight bar
            if (is_selected) {
                for (int dx = 2; dx < panel_width - 2; ++dx) {
                    console->set_cell_rgb(panel_x + dx, panel_y + 3 + static_cast<int>(i), ' ', color, item_bg);
                }
            }
            
            console->draw_string_rgb(item_x, panel_y + 3 + static_cast<int>(i), display, color, item_bg);
        }
        
        // Footer hint
        std::string nav = bindings.get_nav_keys();
        std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
        std::string cancel = bindings.get_key_hint(GameAction::CANCEL);
        std::string footer = "[ " + nav + " ]  [ " + confirm + " ]  [ " + cancel + " ]";
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
            return MenuResult{selected, false};
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            return MenuResult{-1, true};
        }
    }
}

} // namespace MenuWindow
