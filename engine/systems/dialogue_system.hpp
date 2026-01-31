#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../input/input.hpp"
#include "../render/console.hpp"
#include <functional>
#include <string>
#include <vector>

// Result of a dialogue interaction
struct DialogueResult {
    bool ended = false;
    std::string action_triggered;
    EntityId npc_id = 0;
    
    DialogueResult() = default;
    DialogueResult(bool ended, const std::string& action = "", EntityId npc = 0)
        : ended(ended), action_triggered(action), npc_id(npc) {}
};

// Dialogue system - handles NPC conversations
class DialogueSystem {
private:
    ComponentManager* manager;
    Console* console;
    std::function<void(const std::string&)> message_callback;
    std::function<void(const std::string&, EntityId)> action_callback;
    
    // Dialogue UI colors
    const Color PANEL_BORDER_COLOR = Color::CYAN;
    const Color SPEAKER_COLOR = Color::YELLOW;
    const Color TEXT_COLOR = Color::WHITE;
    const Color OPTION_COLOR = Color::WHITE;
    const Color SELECTED_OPTION_COLOR = Color::YELLOW;
    const Color DISABLED_OPTION_COLOR = Color::DARK_GRAY;
    
public:
    DialogueSystem(ComponentManager* mgr, Console* con)
        : manager(mgr), console(con) {}
    
    void set_message_callback(std::function<void(const std::string&)> callback) {
        message_callback = callback;
    }
    
    void set_action_callback(std::function<void(const std::string&, EntityId)> callback) {
        action_callback = callback;
    }
    
    // Start a dialogue with an NPC
    DialogueResult start_dialogue(EntityId npc_id) {
        DialogueComponent* dialogue = manager->get_component<DialogueComponent>(npc_id);
        NPCComponent* npc = manager->get_component<NPCComponent>(npc_id);
        NameComponent* name = manager->get_component<NameComponent>(npc_id);
        
        if (!dialogue || dialogue->nodes.empty()) {
            if (message_callback) {
                std::string npc_name = name ? name->name : "They";
                message_callback(npc_name + " has nothing to say.");
            }
            return DialogueResult(true);
        }
        
        // Reset dialogue to start
        dialogue->reset();
        
        // Show greeting if first time
        if (npc && !dialogue->has_talked && !npc->greeting.empty()) {
            if (message_callback) {
                message_callback(npc->greeting);
            }
        }
        dialogue->has_talked = true;
        
        // Run the dialogue loop
        return run_dialogue_loop(npc_id);
    }
    
    // Continue an existing dialogue
    DialogueResult continue_dialogue(EntityId npc_id, int option_index) {
        DialogueComponent* dialogue = manager->get_component<DialogueComponent>(npc_id);
        if (!dialogue) return DialogueResult(true);
        
        DialogueNode* current = dialogue->get_current_node();
        if (!current) return DialogueResult(true);
        
        if (option_index < 0 || option_index >= static_cast<int>(current->options.size())) {
            return DialogueResult(true);
        }
        
        const DialogueOption& selected = current->options[option_index];
        
        // Trigger action if any
        if (!selected.action.empty() && action_callback) {
            action_callback(selected.action, npc_id);
        }
        
        // Check if dialogue ends
        if (selected.ends_dialogue || selected.next_node_id < 0) {
            NPCComponent* npc = manager->get_component<NPCComponent>(npc_id);
            if (npc && !npc->farewell.empty() && message_callback) {
                message_callback(npc->farewell);
            }
            return DialogueResult(true, selected.action, npc_id);
        }
        
        // Move to next node
        dialogue->current_node_id = selected.next_node_id;
        return DialogueResult(false, selected.action, npc_id);
    }
    
private:
    DialogueResult run_dialogue_loop(EntityId npc_id) {
        DialogueComponent* dialogue = manager->get_component<DialogueComponent>(npc_id);
        if (!dialogue) return DialogueResult(true);
        
        int selected_option = 0;
        bool running = true;
        DialogueResult result;
        
        while (running) {
            DialogueNode* current = dialogue->get_current_node();
            if (!current) {
                running = false;
                result.ended = true;
                break;
            }
            
            // Render dialogue UI
            render_dialogue_ui(current, selected_option);
            
            // Handle input
            Key key = Input::wait_for_key();
            auto& bindings = KeyBindings::instance();
            
            if (bindings.is_action(key, GameAction::MENU_UP)) {
                if (selected_option > 0) {
                    selected_option--;
                }
            } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
                if (selected_option < static_cast<int>(current->options.size()) - 1) {
                    selected_option++;
                }
            } else if (bindings.is_action(key, GameAction::CONFIRM) || bindings.is_action(key, GameAction::INTERACT)) {
                if (!current->options.empty()) {
                    result = continue_dialogue(npc_id, selected_option);
                    if (result.ended) {
                        running = false;
                    } else {
                        selected_option = 0;  // Reset selection for new node
                    }
                } else {
                    // No options = end of dialogue
                    running = false;
                    result.ended = true;
                }
            } else if (bindings.is_action(key, GameAction::CANCEL)) {
                running = false;
                result.ended = true;
            } else if (key >= Key::NUM_1 && key <= Key::NUM_9) {
                // Number key shortcuts
                int num = static_cast<int>(key) - static_cast<int>(Key::NUM_1);
                if (num < static_cast<int>(current->options.size())) {
                    result = continue_dialogue(npc_id, num);
                    if (result.ended) {
                        running = false;
                    } else {
                        selected_option = 0;
                    }
                }
            }
        }
        
        return result;
    }
    
    void render_dialogue_ui(DialogueNode* node, int selected_option) {
        console->clear();
        
        int screen_width = console->get_width();
        int screen_height = console->get_height();
        
        // Calculate panel dimensions
        int panel_width = screen_width - 20;
        int panel_height = 15 + static_cast<int>(node->options.size()) * 2;
        int panel_x = (screen_width - panel_width) / 2;
        int panel_y = (screen_height - panel_height) / 2;
        
        // Ensure minimum size
        if (panel_height > screen_height - 4) {
            panel_height = screen_height - 4;
            panel_y = 2;
        }
        
        // Draw panel background
        console->fill_rect(panel_x, panel_y, panel_width, panel_height, ' ', TEXT_COLOR, Color::BLACK);
        
        // Draw border
        draw_dialogue_box(panel_x, panel_y, panel_width, panel_height);
        
        int y = panel_y + 2;
        
        // Draw speaker name
        console->draw_string(panel_x + 3, y, node->speaker_name, SPEAKER_COLOR);
        y += 2;
        
        // Draw separator line
        for (int i = panel_x + 2; i < panel_x + panel_width - 2; ++i) {
            console->set_cell(i, y, '-', Color::DARK_GRAY, Color::BLACK);
        }
        y += 2;
        
        // Draw dialogue text with word wrap
        draw_wrapped_text(panel_x + 3, y, panel_width - 6, node->text);
        y += count_wrapped_lines(node->text, panel_width - 6) + 2;
        
        // Draw separator before options
        for (int i = panel_x + 2; i < panel_x + panel_width - 2; ++i) {
            console->set_cell(i, y, '-', Color::DARK_GRAY, Color::BLACK);
        }
        y += 2;
        
        // Draw options
        if (node->options.empty()) {
            console->draw_string(panel_x + 3, y, "[Press ENTER to continue]", Color::DARK_GRAY);
        } else {
            for (size_t i = 0; i < node->options.size() && y < panel_y + panel_height - 2; ++i) {
                const DialogueOption& opt = node->options[i];
                
                bool is_selected = (static_cast<int>(i) == selected_option);
                Color color = is_selected ? SELECTED_OPTION_COLOR : OPTION_COLOR;
                
                std::string prefix = is_selected ? "> " : "  ";
                std::string num = "[" + std::to_string(i + 1) + "] ";
                std::string option_text = prefix + num + opt.text;
                
                console->draw_string(panel_x + 3, y, option_text, color);
                y += 2;
            }
        }
        
        // Draw controls hint
        console->draw_string(panel_x + 3, panel_y + panel_height - 2, 
                            "UP/DOWN: Select | ENTER: Choose | ESC: End", Color::DARK_GRAY);
        
        console->present();
    }
    
    void draw_dialogue_box(int x, int y, int width, int height) {
        // Corners
        console->set_cell(x, y, '+', PANEL_BORDER_COLOR, Color::BLACK);
        console->set_cell(x + width - 1, y, '+', PANEL_BORDER_COLOR, Color::BLACK);
        console->set_cell(x, y + height - 1, '+', PANEL_BORDER_COLOR, Color::BLACK);
        console->set_cell(x + width - 1, y + height - 1, '+', PANEL_BORDER_COLOR, Color::BLACK);
        
        // Top and bottom edges
        for (int i = x + 1; i < x + width - 1; ++i) {
            console->set_cell(i, y, '-', PANEL_BORDER_COLOR, Color::BLACK);
            console->set_cell(i, y + height - 1, '-', PANEL_BORDER_COLOR, Color::BLACK);
        }
        
        // Left and right edges
        for (int i = y + 1; i < y + height - 1; ++i) {
            console->set_cell(x, i, '|', PANEL_BORDER_COLOR, Color::BLACK);
            console->set_cell(x + width - 1, i, '|', PANEL_BORDER_COLOR, Color::BLACK);
        }
    }
    
    void draw_wrapped_text(int x, int y, int max_width, const std::string& text) {
        int current_x = x;
        int current_y = y;
        std::string word;
        
        for (size_t i = 0; i <= text.length(); ++i) {
            char ch = (i < text.length()) ? text[i] : ' ';
            
            if (ch == ' ' || ch == '\n' || i == text.length()) {
                if (!word.empty()) {
                    if (current_x - x + static_cast<int>(word.length()) > max_width) {
                        current_y++;
                        current_x = x;
                    }
                    console->draw_string(current_x, current_y, word, TEXT_COLOR);
                    current_x += word.length() + 1;
                    word.clear();
                }
                
                if (ch == '\n') {
                    current_y++;
                    current_x = x;
                }
            } else {
                word += ch;
            }
        }
    }
    
    int count_wrapped_lines(const std::string& text, int max_width) {
        int lines = 1;
        int current_width = 0;
        std::string word;
        
        for (size_t i = 0; i <= text.length(); ++i) {
            char ch = (i < text.length()) ? text[i] : ' ';
            
            if (ch == ' ' || ch == '\n' || i == text.length()) {
                if (!word.empty()) {
                    if (current_width + static_cast<int>(word.length()) > max_width) {
                        lines++;
                        current_width = word.length() + 1;
                    } else {
                        current_width += word.length() + 1;
                    }
                    word.clear();
                }
                
                if (ch == '\n') {
                    lines++;
                    current_width = 0;
                }
            } else {
                word += ch;
            }
        }
        
        return lines;
    }
};
