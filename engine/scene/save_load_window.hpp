#pragma once
#include "menu_window.hpp"
#include "../util/save_system.hpp"
#include "../world/game_state.hpp"
#include "../ecs/entity_transfer.hpp"
#include <functional>

// Save/Load game windows that can be shown as overlays from any scene

namespace SaveLoadWindow {

using namespace MenuWindow;

// Result from save/load window
struct SaveLoadResult {
    enum class Action {
        NONE,           // Window closed without action
        LOADED,         // Game loaded successfully
        SAVED,          // Game saved successfully
        CANCELLED       // User cancelled
    };
    
    Action action = Action::NONE;
    std::string next_scene;  // For load: which scene to switch to
    bool fresh_scene = false;  // Whether to request a fresh scene
};

// Draw save slot list panel
inline void draw_slot_list(Console* console, const std::string& title, 
    const std::vector<SaveSlotInfo>& slots, int selected_slot, int panel_width = 60) {
    
    int width = console->get_width();
    int height = console->get_height();
    
    // Draw themed border on full screen
    draw_themed_border(console, width, height);
    
    int panel_height = MAX_SAVE_SLOTS + 8;
    int panel_x = (width - panel_width) / 2;
    int panel_y = (height - panel_height) / 2;
    
    // Draw menu frame
    draw_themed_box(console, panel_x, panel_y, panel_width, panel_height, title);
    
    // Draw slots
    for (int i = 0; i < MAX_SAVE_SLOTS; ++i) {
        bool is_selected = (i == selected_slot);
        RGBColor color = is_selected ? Theme::selected() : Theme::normal();
        RGBColor bg = is_selected ? Theme::highlight_bg() : Theme::bg();
        
        std::string prefix = is_selected ? ">> " : "   ";
        std::string suffix = is_selected ? " <<" : "   ";
        std::string display = prefix + slots[i].get_display_string();
        
        // Truncate if too long
        if (display.length() > static_cast<size_t>(panel_width - 8)) {
            display = display.substr(0, panel_width - 11) + "...";
        }
        display += suffix;
        
        // Clear line with background
        if (is_selected) {
            for (int dx = 2; dx < panel_width - 2; ++dx) {
                console->set_cell_rgb(panel_x + dx, panel_y + 3 + i, ' ', color, bg);
            }
        }
        
        console->draw_string_rgb(panel_x + 2, panel_y + 3 + i, display, color, bg);
    }
}

// Draw confirmation dialog
inline void draw_confirm_dialog(Console* console, const std::string& message) {
    int width = console->get_width();
    int height = console->get_height();
    
    int dialog_width = 40;
    int dialog_height = 7;
    int dialog_x = (width - dialog_width) / 2;
    int dialog_y = (height - dialog_height) / 2;
    
    draw_themed_box(console, dialog_x, dialog_y, dialog_width, dialog_height, "CONFIRM");
    
    // Center the message
    int msg_x = dialog_x + (dialog_width - static_cast<int>(message.length())) / 2;
    console->draw_string_rgb(msg_x, dialog_y + 3, message, Theme::selected(), Theme::bg());
    
    console->draw_string_rgb(dialog_x + 3, dialog_y + 5, "[Y] Yes    [N] No    [ESC] Cancel", 
        Theme::hint(), Theme::bg());
}

// Show load game window
inline SaveLoadResult show_load_window(
    Console* console, 
    GameState* game_state,
    std::function<void()> render_background = nullptr,
    EntityTransferManager* transfer_manager = nullptr
) {
    auto& bindings = KeyBindings::instance();
    std::vector<SaveSlotInfo> slots = SaveSystem::instance().get_all_slot_info();
    int selected_slot = 0;
    bool confirm_dialog = false;
    int slot_to_delete = -1;
    
    while (true) {
        // Render background
        if (render_background) {
            render_background();
        } else {
            console->clear();
        }
        
        draw_slot_list(console, "LOAD GAME", slots, selected_slot);
        
        // Additional hint for empty slots
        if (slots[selected_slot].is_empty) {
            int width = console->get_width();
            console->draw_string(width / 2 - 10, console->get_height() - 4, 
                "(No save in this slot)", Color::DARK_GRAY);
        }
        
        // Help text
        int panel_x = (console->get_width() - 60) / 2;
        int panel_y = (console->get_height() - (MAX_SAVE_SLOTS + 8)) / 2;
        std::string help = bindings.get_key_hint(GameAction::CONFIRM) + ": Load  X: Delete  " + 
            bindings.get_key_hint(GameAction::CANCEL) + ": Back";
        console->draw_string_rgb(panel_x + 2, panel_y + MAX_SAVE_SLOTS + 6, help, Theme::hint(), Theme::bg());
        
        if (confirm_dialog) {
            draw_confirm_dialog(console, "Delete this save?");
        }
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (confirm_dialog) {
            if (key == Key::Y) {
                SaveSystem::instance().delete_save(slot_to_delete + 1);
                slots = SaveSystem::instance().get_all_slot_info();
                confirm_dialog = false;
                slot_to_delete = -1;
            } else if (key == Key::N || bindings.is_action(key, GameAction::CANCEL)) {
                confirm_dialog = false;
                slot_to_delete = -1;
            }
            continue;
        }
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            selected_slot = (selected_slot - 1 + MAX_SAVE_SLOTS) % MAX_SAVE_SLOTS;
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            selected_slot = (selected_slot + 1) % MAX_SAVE_SLOTS;
        } else if (key == Key::X && !slots[selected_slot].is_empty) {
            slot_to_delete = selected_slot;
            confirm_dialog = true;
        } else if (bindings.is_action(key, GameAction::CONFIRM)) {
            if (!slots[selected_slot].is_empty) {
                // Load the game
                GameSaveData data;
                if (SaveSystem::instance().load_game(selected_slot + 1, data)) {
                    // Store loaded data in game state
                    auto& state = game_state->dungeon;
                    state.reset();
                    
                    state.player_hp = data.player_hp;
                    state.player_max_hp = data.player_max_hp;
                    state.player_level = data.player_level;
                    state.player_xp = data.player_xp;
                    state.player_attack = data.player_attack;
                    state.player_defense = data.player_defense;
                    state.player_gold = data.player_gold;
                    state.player_components_json = data.player_components_json;
                    state.has_player_components = data.has_player_components;
                    state.saved_entities = data.world_entities;
                    state.loading_from_save = true;
                    state.player_x = data.player_x;
                    state.player_y = data.player_y;
                    state.world_seed = data.world_seed;
                    
                    // Queue player transfer from loaded data (preferred method)
                    if (transfer_manager && data.has_player_components && !data.player_components_json.empty()) {
                        transfer_manager->set_player_transfer_from_json(data.player_components_json);
                    }
                    
                    // Restore visited chunks (prevents enemy respawning)
                    for (const auto& coord : data.visited_chunks) {
                        state.visited_chunks.insert(ChunkCoord(coord.first, coord.second));
                    }
                    
                    // Restore per-chunk entity persistence
                    state.chunk_entities = data.chunk_entities;
                    
                    SaveLoadResult result;
                    result.action = SaveLoadResult::Action::LOADED;
                    result.fresh_scene = true;
                    
                    if (data.in_dungeon) {
                        state.active = true;
                        state.type = data.dungeon_type;
                        state.name = data.dungeon_name;
                        state.seed = data.dungeon_seed;
                        state.depth = data.dungeon_depth;
                        state.max_depth = data.dungeon_max_depth;
                        state.difficulty = data.dungeon_difficulty;
                        state.return_x = data.dungeon_return_x;
                        state.return_y = data.dungeon_return_y;
                        result.next_scene = "game";
                    } else {
                        result.next_scene = "exploration";
                    }
                    
                    return result;
                }
            }
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            return SaveLoadResult{SaveLoadResult::Action::CANCELLED, "", false};
        }
    }
}

// Show save game window
inline SaveLoadResult show_save_window(
    Console* console,
    GameState* game_state,
    std::function<void()> render_background = nullptr
) {
    auto& bindings = KeyBindings::instance();
    std::vector<SaveSlotInfo> slots = SaveSystem::instance().get_all_slot_info();
    int selected_slot = 0;
    bool confirm_dialog = false;
    
    // Populate save data from game state
    auto& state = game_state->dungeon;
    GameSaveData pending_save;
    
    pending_save.player_name = "Explorer";
    pending_save.current_scene = state.in_dungeon ? "dungeon" : "exploration";
    pending_save.player_x = state.player_x;
    pending_save.player_y = state.player_y;
    pending_save.player_hp = state.player_hp;
    pending_save.player_max_hp = state.player_max_hp;
    pending_save.player_level = state.player_level;
    pending_save.player_xp = state.player_xp;
    pending_save.player_attack = state.player_attack;
    pending_save.player_defense = state.player_defense;
    pending_save.player_gold = state.player_gold;
    pending_save.player_components_json = state.player_components_json;
    pending_save.has_player_components = state.has_player_components;
    pending_save.inventory = state.saved_inventory;
    pending_save.world_seed = state.world_seed;
    pending_save.world_entities = state.saved_entities;
    pending_save.in_dungeon = state.in_dungeon;
    
    // Save visited chunks (prevents enemy respawning)
    for (const auto& coord : state.visited_chunks) {
        pending_save.visited_chunks.push_back({coord.x, coord.y});
    }
    
    // Save per-chunk entity persistence
    pending_save.chunk_entities = state.chunk_entities;
    
    if (state.in_dungeon) {
        pending_save.dungeon_type = state.type;
        pending_save.dungeon_name = state.name;
        pending_save.dungeon_seed = state.seed;
        pending_save.dungeon_depth = state.depth;
        pending_save.dungeon_max_depth = state.max_depth;
        pending_save.dungeon_difficulty = state.difficulty;
        pending_save.dungeon_return_x = state.return_x;
        pending_save.dungeon_return_y = state.return_y;
    }
    
    while (true) {
        // Render background
        if (render_background) {
            render_background();
        } else {
            console->clear();
        }
        
        draw_slot_list(console, "SAVE GAME", slots, selected_slot);
        
        // Help text
        int panel_x = (console->get_width() - 60) / 2;
        int panel_y = (console->get_height() - (MAX_SAVE_SLOTS + 8)) / 2;
        std::string help = bindings.get_key_hint(GameAction::CONFIRM) + ": Save  " + 
            bindings.get_key_hint(GameAction::CANCEL) + ": Back";
        console->draw_string_rgb(panel_x + 2, panel_y + MAX_SAVE_SLOTS + 6, help, Theme::hint(), Theme::bg());
        
        if (confirm_dialog) {
            draw_confirm_dialog(console, "Overwrite existing save?");
        }
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (confirm_dialog) {
            if (key == Key::Y) {
                SaveSystem::instance().save_game(selected_slot + 1, pending_save);
                return SaveLoadResult{SaveLoadResult::Action::SAVED, "", false};
            } else if (key == Key::N || bindings.is_action(key, GameAction::CANCEL)) {
                confirm_dialog = false;
            }
            continue;
        }
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            selected_slot = (selected_slot - 1 + MAX_SAVE_SLOTS) % MAX_SAVE_SLOTS;
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            selected_slot = (selected_slot + 1) % MAX_SAVE_SLOTS;
        } else if (bindings.is_action(key, GameAction::CONFIRM)) {
            if (!slots[selected_slot].is_empty) {
                confirm_dialog = true;
            } else {
                // Save directly to empty slot
                SaveSystem::instance().save_game(selected_slot + 1, pending_save);
                return SaveLoadResult{SaveLoadResult::Action::SAVED, "", false};
            }
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            return SaveLoadResult{SaveLoadResult::Action::CANCELLED, "", false};
        }
    }
}

} // namespace SaveLoadWindow
