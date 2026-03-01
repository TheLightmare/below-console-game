#include "../render/ui.hpp"

// Draw a panel/window
void UI::draw_panel(int x, int y, int width, int height, const std::string& title, 
               Color fg, Color bg) {
    // Fill panel background
    console->fill_rect(x, y, width, height, ' ', fg, bg);
    
    // Draw border
    console->draw_box(x, y, width, height, fg, bg);
    
    if (!title.empty()) {
        int title_x = x + (width - static_cast<int>(title.length())) / 2;
        console->draw_string(title_x, y, title, fg, bg);
    }
}

// Draw text with word wrap
void UI::draw_text(int x, int y, int max_width, const std::string& text, 
              Color fg, Color bg) {
    int current_x = x;
    int current_y = y;
    
    std::string word;
    for (char ch : text) {
        if (ch == ' ' || ch == '\n') {
            if (current_x + static_cast<int>(word.length()) > x + max_width) {
                current_y++;
                current_x = x;
            }
            console->draw_string(current_x, current_y, word, fg, bg);
            current_x += static_cast<int>(word.length());
            word.clear();
            
            if (ch == '\n') {
                current_y++;
                current_x = x;
            } else {
                current_x++;
            }
        } else {
            word += ch;
        }
    }
    
    // Draw last word
    if (!word.empty()) {
        if (current_x + static_cast<int>(word.length()) > x + max_width) {
            current_y++;
            current_x = x;
        }
        console->draw_string(current_x, current_y, word, fg, bg);
    }
}

// Draw a health/progress bar
void UI::draw_health_bar(int x, int y, int width, int current, int max, 
                    Color full_color, Color empty_color) {
    if (max <= 0) return;
    
    int filled = (current * width) / max;
    filled = (filled < 0) ? 0 : ((filled > width) ? width : filled);
    
    // Draw filled portion
    for (int i = 0; i < filled; ++i) {
        console->set_cell(x + i, y, '=', full_color, Color::BLACK);
    }
    
    // Draw empty portion
    for (int i = filled; i < width; ++i) {
        console->set_cell(x + i, y, '-', empty_color, Color::BLACK);
    }
    
    // Draw values
    std::string health_text = std::to_string(current) + "/" + std::to_string(max);
    int text_x = x + (width - static_cast<int>(health_text.length())) / 2;
    console->draw_string(text_x, y, health_text, Color::WHITE, Color::BLACK);
}

// Draw message log
void UI::draw_message_log(int x, int y, int width, int height, const std::vector<std::string>& messages, int scroll_offset) {
    // Show scroll indicator in title if scrolled up
    std::string title = scroll_offset > 0 ? "Messages [SCROLLED]" : "Messages";
    draw_panel(x, y, width, height, title);
    
    int visible_lines = height - 2;
    int total_messages = static_cast<int>(messages.size());
    
    // Calculate start index: normally show latest messages, scroll_offset moves us up in history
    int base_start = total_messages - visible_lines;
    if (base_start < 0) base_start = 0;
    
    int start_index = base_start - scroll_offset;
    if (start_index < 0) start_index = 0;
    
    int current_y = y + 1;
    
    for (int i = start_index; i < total_messages && current_y < y + height - 1; ++i) {
        console->draw_string(x + 2, current_y++, messages[i], Color::WHITE);
    }
    
    // Draw scroll indicators
    if (scroll_offset > 0) {
        console->draw_string(x + width - 2, y + height - 1, "v", Color::CYAN);
    }
    if (start_index > 0) {
        console->draw_string(x + width - 2, y, "^", Color::CYAN);
    }
}

// Draw a menu
int UI::draw_menu(int x, int y, const std::string& title, const std::vector<std::string>& options, 
              int selected_index) {
    int width = static_cast<int>(title.length()) + 4;
    for (const auto& opt : options) {
        int opt_width = static_cast<int>(opt.length()) + 6;
        width = (opt_width > width) ? opt_width : width;
    }
    
    int height = static_cast<int>(options.size()) + 3;
    
    draw_panel(x, y, width, height, title, Color::YELLOW);
    
    for (size_t i = 0; i < options.size(); ++i) {
        Color color = Color::WHITE;
        Color bg = Color::BLACK;
        std::string prefix = (static_cast<int>(i) == selected_index) ? "> " : "  ";
        
        console->fill_rect(x + 1, y + 1 + static_cast<int>(i), width - 2, 1, ' ', color, bg);
        console->draw_string(x + 2, y + 1 + static_cast<int>(i), prefix + options[i], color, bg);
    }
    
    return height;
}

// Show entity selection when multiple entities are nearby
EntityId UI::show_entity_selection(const std::vector<EntityId>& nearby_entities, std::function<void()> render_background) {
    if (nearby_entities.empty()) {
        return 0;
    }
    
    if (nearby_entities.size() == 1) {
        return nearby_entities[0];
    }
    
    int selected = 0;
    auto& bindings = KeyBindings::instance();
    
    while (true) {
        if (render_background) {
            render_background();
        } else {
            console->clear();
        }
        
        int panel_width = 50;
        int panel_height = 4 + static_cast<int>(nearby_entities.size()) * 2;
        int panel_x = (console->get_width() - panel_width) / 2;
        int panel_y = (console->get_height() - panel_height) / 2;
        
        draw_panel(panel_x, panel_y, panel_width, panel_height, "Select Target", Color::CYAN);
        
        int y = panel_y + 2;
        
        for (size_t i = 0; i < nearby_entities.size(); ++i) {
            EntityId id = nearby_entities[i];
            RenderComponent* render = manager->get_component<RenderComponent>(id);
            
            bool is_selected = (static_cast<int>(i) == selected);
            Color color = is_selected ? Color::YELLOW : Color::WHITE;
            
            std::string prefix = is_selected ? "> " : "  ";
            std::string symbol_str = render ? std::string(1, render->ch) : "?";
            
            std::string line = prefix + "[" + symbol_str + "] Entity #" + std::to_string(id);
            console->draw_string(panel_x + 2, y++, line, color);
        }
        
        std::string nav = bindings.get_nav_keys();
        std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
        std::string cancel = bindings.get_key_hint(GameAction::CANCEL);
        console->draw_string(panel_x + 2, panel_y + panel_height - 2, nav + ": Select | " + confirm + ": Confirm | " + cancel + ": Cancel", Color::DARK_GRAY);
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            if (selected > 0) selected--;
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            if (selected < static_cast<int>(nearby_entities.size()) - 1) selected++;
        } else if (bindings.is_action(key, GameAction::CONFIRM)) {
            return nearby_entities[selected];
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            return 0;
        } else if (key >= Key::NUM_1 && key <= Key::NUM_9) {
            int num = static_cast<int>(key) - static_cast<int>(Key::NUM_1);
            if (num < static_cast<int>(nearby_entities.size())) {
                return nearby_entities[num];
            }
        }
    }
    
    return 0;
}

// Simple confirmation dialog
bool UI::confirm_action(const std::string& message) {
    int panel_width = static_cast<int>(message.length()) + 10;
    int panel_height = 5;
    int panel_x = (console->get_width() - panel_width) / 2;
    int panel_y = (console->get_height() - panel_height) / 2;
    
    draw_panel(panel_x, panel_y, panel_width, panel_height, "Confirm", Color::YELLOW);
    console->draw_string(panel_x + 2, panel_y + 1, message, Color::WHITE);
    console->draw_string(panel_x + 2, panel_y + 3, "[Y] Yes    [N] No", Color::DARK_GRAY);
    console->present();
    
    while (true) {
        Key key = Input::wait_for_key();
        if (key == Key::Y) return true;
        if (key == Key::N || key == Key::ESCAPE) return false;
    }
}
