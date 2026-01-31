// gameplay_menu.inl - Pause menu and message log for BaseGameplayScene
// This file is included by base_gameplay_scene.hpp - do not include directly

// ==================== Pause Menu Window ====================

enum class PauseMenuResult {
    RESUME,
    SAVE_GAME,
    SETTINGS,
    MAIN_MENU,
    QUIT
};

// Show pause menu as overlay window
PauseMenuResult show_pause_menu() {
    // Queue player state for potential scene transition
    if (transfer_manager && !transfer_manager->has_player_transfer()) {
        queue_player_for_transfer();
    }
    
    std::vector<std::string> options = {"Resume", "Save Game", "Settings", "Main Menu", "Quit"};
    int selected = 0;
    
    while (true) {
        // Render game underneath
        render();
        
        // Draw pause overlay with themed style matching main menu
        int panel_width = 30;
        int panel_height = static_cast<int>(options.size()) + 6;
        int panel_x = (console->get_width() - panel_width) / 2;
        int panel_y = (console->get_height() - panel_height) / 2;
        
        // Theme colors matching main menu style
        RGBColor border_color(220, 80, 50);
        RGBColor separator_color(150, 60, 40);
        RGBColor title_color(255, 100, 50);
        RGBColor selected_color(255, 200, 100);
        RGBColor normal_color(200, 150, 150);
        RGBColor hint_color(150, 100, 100);
        RGBColor highlight_bg(60, 20, 20);
        RGBColor bg(0, 0, 0);
        
        // Fill background first to make it opaque
        for (int dy = 0; dy < panel_height; ++dy) {
            for (int dx = 0; dx < panel_width; ++dx) {
                console->set_cell_rgb(panel_x + dx, panel_y + dy, ' ', normal_color, bg);
            }
        }
        
        // Draw box frame
        console->draw_box_rgb(panel_x, panel_y, panel_width, panel_height, border_color, bg, '=', '!', '@', '@', '@', '@');
        
        // Separator line under title
        console->set_cell_rgb(panel_x, panel_y + 2, '@', border_color, bg);
        console->set_cell_rgb(panel_x + panel_width - 1, panel_y + 2, '@', border_color, bg);
        for (int i = 1; i < panel_width - 1; ++i) {
            console->set_cell_rgb(panel_x + i, panel_y + 2, ':', separator_color, bg);
        }
        
        // Title
        std::string title = "PAUSED";
        int title_x = panel_x + (panel_width - static_cast<int>(title.length())) / 2;
        console->draw_string_rgb(title_x, panel_y + 1, title, title_color, bg);
        
        // Draw menu items
        for (size_t i = 0; i < options.size(); ++i) {
            bool is_selected = (static_cast<int>(i) == selected);
            RGBColor color = is_selected ? selected_color : normal_color;
            RGBColor item_bg = is_selected ? highlight_bg : bg;
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
        auto& bindings = KeyBindings::instance();
        std::string nav = bindings.get_nav_keys();
        std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
        std::string footer = "[ " + nav + " ] Navigate  [ " + confirm + " ] Select";
        int footer_x = panel_x + (panel_width - static_cast<int>(footer.length())) / 2;
        if (footer_x < panel_x + 1) footer_x = panel_x + 1;
        console->draw_string_rgb(footer_x, panel_y + panel_height - 1, footer, hint_color, bg);
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            if (selected > 0) selected--;
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            if (selected < static_cast<int>(options.size()) - 1) selected++;
        } else if (bindings.is_action(key, GameAction::CONFIRM)) {
            switch (selected) {
                case 0: return PauseMenuResult::RESUME;
                case 1: return PauseMenuResult::SAVE_GAME;
                case 2: return PauseMenuResult::SETTINGS;
                case 3: return PauseMenuResult::MAIN_MENU;
                case 4: return PauseMenuResult::QUIT;
            }
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            return PauseMenuResult::RESUME;
        }
    }
}

// ==================== Settings Menu Window ====================

// Show settings menu as overlay window - uses shared settings menu
void show_settings_menu() {
    SettingsMenu::show(console, ui, [this]() { render(); });
}

// ==================== Save Game Window ====================

// Show save game window as overlay
void show_save_game_window() {
    // Queue player for transfer - this serializes all components
    if (transfer_manager && !transfer_manager->has_player_transfer()) {
        queue_player_for_transfer();
    }
    
    // Populate dungeon state with player data for the save system
    // The save system reads from game_state->dungeon
    if (transfer_manager) {
        std::string player_json = transfer_manager->get_player_transfer_json();
        if (!player_json.empty()) {
            game_state->dungeon.player_components_json = player_json;
            game_state->dungeon.has_player_components = true;
        }
    }
    
    // Also populate basic player data for save metadata
    PositionComponent* pos = manager->get_component<PositionComponent>(player_id);
    if (pos) {
        game_state->dungeon.player_x = pos->x;
        game_state->dungeon.player_y = pos->y;
    }
    StatsComponent* stats = manager->get_component<StatsComponent>(player_id);
    if (stats) {
        game_state->dungeon.player_hp = stats->current_hp;
        game_state->dungeon.player_max_hp = stats->max_hp;
        game_state->dungeon.player_level = stats->level;
        game_state->dungeon.player_xp = stats->experience;
        game_state->dungeon.player_attack = stats->attack;
        game_state->dungeon.player_defense = stats->defense;
    }
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    if (inv) {
        game_state->dungeon.player_gold = inv->gold;
    }
    
    save_world_entities_to_dungeon_state();
    
    SaveLoadWindow::show_save_window(console, game_state, [this]() { render(); });
}

// ==================== Message Log ====================

// Common message log functionality
void add_message(const std::string& msg) {
    message_log.push_back(msg);
    if (message_log.size() > 100) {
        message_log.erase(message_log.begin());
    }
    // Reset scroll when new message arrives
    message_scroll = 0;
}

// Initialize message log with multiple messages (clears existing messages)
void init_message_log(std::initializer_list<std::string> messages) {
    message_log.clear();
    message_scroll = 0;
    for (const auto& msg : messages) {
        add_message(msg);
    }
}

// Message log scrolling
void scroll_messages_up() {
    int max_scroll = static_cast<int>(message_log.size()) - 4;  // Keep at least 4 lines visible
    if (max_scroll < 0) max_scroll = 0;
    if (message_scroll < max_scroll) {
        message_scroll++;
    }
}

void scroll_messages_down() {
    if (message_scroll > 0) {
        message_scroll--;
    }
}
