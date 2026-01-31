#include "../render/ui.hpp"
#include "../ecs/components/status_effects_component.hpp"

// Helper to map StatusEffectType to Color enum for UI display
static Color get_effect_display_color(StatusEffectType type) {
    switch (type) {
        // Instant effects (not typically displayed, but included for completeness)
        case StatusEffectType::HEAL: return Color::CYAN;
        case StatusEffectType::HEAL_PERCENT: return Color::CYAN;
        case StatusEffectType::DAMAGE: return Color::RED;
        case StatusEffectType::GAIN_XP: return Color::YELLOW;
        // Duration effects
        case StatusEffectType::POISON: return Color::GREEN;
        case StatusEffectType::BURN: return Color::RED;
        case StatusEffectType::BLEED: return Color::DARK_RED;
        case StatusEffectType::REGENERATION: return Color::CYAN;
        case StatusEffectType::BUFF_ATTACK: return Color::YELLOW;
        case StatusEffectType::BUFF_DEFENSE: return Color::BLUE;
        case StatusEffectType::BUFF_SPEED: return Color::CYAN;
        case StatusEffectType::DEBUFF_ATTACK: return Color::DARK_YELLOW;
        case StatusEffectType::DEBUFF_DEFENSE: return Color::DARK_BLUE;
        case StatusEffectType::DEBUFF_SPEED: return Color::DARK_CYAN;
        case StatusEffectType::STUN: return Color::YELLOW;
        case StatusEffectType::PARALYSIS: return Color::MAGENTA;
        case StatusEffectType::BLIND: return Color::DARK_GRAY;
        case StatusEffectType::CONFUSION: return Color::MAGENTA;
        case StatusEffectType::FEAR: return Color::DARK_MAGENTA;
        case StatusEffectType::CHARM: return Color::MAGENTA;
        case StatusEffectType::INVISIBILITY: return Color::WHITE;
        case StatusEffectType::INVULNERABLE: return Color::WHITE;
        case StatusEffectType::BERSERK: return Color::RED;
        case StatusEffectType::CUSTOM: return Color::GRAY;
        default: return Color::WHITE;
    }
}

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

// Draw a health bar
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

// Draw stats for an entity
void UI::draw_stats(int x, int y, EntityId entity_id) {
    NameComponent* name = manager->get_component<NameComponent>(entity_id);
    StatsComponent* stats = manager->get_component<StatsComponent>(entity_id);
    
    int current_y = y;
    
    if (name) {
        console->draw_string(x, current_y++, name->name, Color::YELLOW);
    }
    
    if (stats) {
        console->draw_string(x, current_y++, "Level: " + std::to_string(stats->level), Color::WHITE);
        console->draw_string(x, current_y++, "HP: ", Color::WHITE);
        draw_health_bar(x + 4, current_y - 1, 15, stats->current_hp, stats->max_hp);
        current_y++;
        
        console->draw_string(x, current_y++, "ATK: " + std::to_string(stats->attack), Color::RED);
        console->draw_string(x, current_y++, "DEF: " + std::to_string(stats->defense), Color::BLUE);
        console->draw_string(x, current_y++, "EXP: " + std::to_string(stats->experience) + 
                           "/" + std::to_string(stats->get_exp_to_level()), Color::CYAN);
    }
}

// Draw inventory
void UI::draw_inventory(int x, int y, int width, int height, EntityId entity_id) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    if (!inv) return;
    
    draw_panel(x, y, width, height, "Inventory");
    
    int current_y = y + 1;
    console->draw_string(x + 2, current_y++, "Gold: " + std::to_string(inv->gold), Color::YELLOW);
    console->draw_string(x + 2, current_y++, "Items:", Color::WHITE);
    
    for (size_t i = 0; i < inv->items.size() && current_y < y + height - 1; ++i) {
        EntityId item_id = inv->items[i];
        NameComponent* item_name = manager->get_component<NameComponent>(item_id);
        
        if (item_name) {
            std::string item_text = "  " + std::to_string(i + 1) + ". " + item_name->name;
            console->draw_string(x + 2, current_y++, item_text, Color::CYAN);
        }
    }
    
    if (inv->items.empty()) {
        console->draw_string(x + 4, current_y, "(empty)", Color::DARK_GRAY);
    }
}

// Draw equipment panel showing dynamic limb-based slots
void UI::draw_equipment(int x, int y, int width, int height, EntityId entity_id) {
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(entity_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    
    if (!anatomy || !equip) {
        draw_panel(x, y, width, height, "Equipment");
        console->draw_string(x + 2, y + 2, "No equipment data", Color::DARK_GRAY);
        return;
    }
    
    draw_panel(x, y, width, height, "Equipment");
    
    int current_y = y + 1;
    
    // Group limbs by type and display equipment slots
    std::unordered_map<LimbType, std::vector<Limb*>> limb_groups;
    for (auto& limb : anatomy->limbs) {
        if (limb.functional) {
            limb_groups[limb.type].push_back(&limb);
        }
    }
    
    // Display equipment by limb type
    auto display_limb_group = [&](LimbType type, const std::string& group_name) {
        if (limb_groups.find(type) != limb_groups.end() && current_y < y + height - 1) {
            console->draw_string(x + 2, current_y++, group_name + ":", Color::YELLOW);
            
            for (Limb* limb : limb_groups[type]) {
                if (current_y >= y + height - 1) break;
                
                EntityId equipped = equip->get_equipped(limb->name);
                std::string slot_text = "  " + limb->name + ": ";
                
                if (equipped != 0) {
                    NameComponent* item_name = manager->get_component<NameComponent>(equipped);
                    slot_text += item_name ? item_name->name : "???";
                    console->draw_string(x + 2, current_y++, slot_text, Color::GREEN);
                } else {
                    slot_text += "(empty)";
                    console->draw_string(x + 2, current_y++, slot_text, Color::DARK_GRAY);
                }
            }
        }
    };
    
    display_limb_group(LimbType::HEAD, "Head");
    display_limb_group(LimbType::TORSO, "Torso");
    display_limb_group(LimbType::HAND, "Hands");
    display_limb_group(LimbType::ARM, "Arms");
    display_limb_group(LimbType::FOOT, "Feet");
    display_limb_group(LimbType::LEG, "Legs");
    display_limb_group(LimbType::OTHER, "Other");
    
    // Show special slots
    if (current_y < y + height - 1) {
        console->draw_string(x + 2, current_y++, "Other:", Color::YELLOW);
        if (current_y < y + height - 1) {
            std::string back_text = "  back: ";
            if (equip->back != 0) {
                NameComponent* item_name = manager->get_component<NameComponent>(equip->back);
                back_text += item_name ? item_name->name : "???";
                console->draw_string(x + 2, current_y++, back_text, Color::GREEN);
            } else {
                back_text += "(empty)";
                console->draw_string(x + 2, current_y++, back_text, Color::DARK_GRAY);
            }
        }
    }
}

// Draw message log
void UI::draw_message_log(int x, int y, int width, int height, const std::vector<std::string>& messages, int scroll_offset, const KeyBindings* bindings) {
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
    
    // Draw scroll indicators with actual keybinding names
    std::string scroll_down_key = "PgDn";
    std::string scroll_up_key = "PgUp";
    if (bindings) {
        Key down_key = bindings->get_primary(GameAction::SCROLL_MESSAGES_DOWN);
        Key up_key = bindings->get_primary(GameAction::SCROLL_MESSAGES_UP);
        if (down_key != Key::NONE) scroll_down_key = key_to_short_string(down_key);
        if (up_key != Key::NONE) scroll_up_key = key_to_short_string(up_key);
    }
    
    if (scroll_offset > 0) {
        console->draw_string(x + width - static_cast<int>(scroll_down_key.length()), y + height - 1, scroll_down_key, Color::CYAN);
    }
    if (start_index > 0) {
        console->draw_string(x + width - static_cast<int>(scroll_up_key.length()), y, scroll_up_key, Color::CYAN);
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
        Color color = (static_cast<int>(i) == selected_index) ? Color::WHITE : Color::WHITE;
        Color bg = (static_cast<int>(i) == selected_index) ? Color::BLACK : Color::BLACK;
        std::string prefix = (static_cast<int>(i) == selected_index) ? "> " : "  ";
        
        console->fill_rect(x + 1, y + 1 + static_cast<int>(i), width - 2, 1, ' ', color, bg);
        console->draw_string(x + 2, y + 1 + static_cast<int>(i), prefix + options[i], color, bg);
    }
    
    return height;
}

// Draw HUD (Heads-Up Display) - Side panel on the right
void UI::draw_hud(EntityId player_id, int screen_width, int screen_height) {
    StatsComponent* stats = manager->get_component<StatsComponent>(player_id);
    NameComponent* name = manager->get_component<NameComponent>(player_id);
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);

    // Responsive panel width (clamped)
    int panel_width = screen_width / 4;
    if (panel_width < 24) panel_width = 24;
    if (panel_width > 36) panel_width = 36;
    const int panel_x = screen_width - panel_width;

    // Draw the base panel
    draw_panel(panel_x, 0, panel_width, screen_height, "Status", Color::CYAN, Color::BLACK);

    int y = 1;

    // Name/header
    if (name) {
        console->draw_string(panel_x + 2, y++, name->name, Color::YELLOW, Color::BLACK);
        y++;
    }

    // Stats block using helper functions where possible
    if (stats) {
        console->draw_string(panel_x + 2, y++, "Level: " + std::to_string(stats->level), Color::WHITE, Color::BLACK);

        // Health and bar
        console->draw_string(panel_x + 2, y, "HP:", Color::WHITE, Color::BLACK);
        draw_health_bar(panel_x + 7, y, panel_width - 10, stats->current_hp, stats->max_hp,
                        (stats->current_hp < stats->max_hp / 3) ? Color::RED :
                        (stats->current_hp < stats->max_hp * 2 / 3) ? Color::YELLOW : Color::GREEN,
                        Color::DARK_GRAY);
        y += 2;

        console->draw_string(panel_x + 2, y++, "ATK: " + std::to_string(stats->attack), Color::RED, Color::BLACK);
        console->draw_string(panel_x + 2, y++, "DEF: " + std::to_string(stats->defense), Color::BLUE, Color::BLACK);
        console->draw_string(panel_x + 2, y++, "EXP: " + std::to_string(stats->experience) + "/" + std::to_string(stats->get_exp_to_level()), Color::CYAN, Color::BLACK);
        y++;
    }

    // Inventory summary
    if (inv) {
        console->draw_string(panel_x + 2, y++, "Gold: " + std::to_string(inv->gold), Color::YELLOW, Color::BLACK);
        console->draw_string(panel_x + 2, y++, "Inventory:", Color::WHITE, Color::BLACK);

        const int max_items_shown = 6;
        int shown = 0;
        for (size_t i = 0; i < inv->items.size() && shown < max_items_shown && y < screen_height - 1; ++i) {
            EntityId item_id = inv->items[i];
            NameComponent* item_name = manager->get_component<NameComponent>(item_id);
            std::string text = "- ";
            text += (item_name ? item_name->name : "(item)");
            console->draw_string(panel_x + 4, y++, text, Color::CYAN, Color::BLACK);
            ++shown;
        }

        if (inv->items.empty()) {
            console->draw_string(panel_x + 4, y++, "(empty)", Color::DARK_GRAY, Color::BLACK);
        } else if (inv->items.size() > static_cast<size_t>(max_items_shown)) {
            console->draw_string(panel_x + 4, y++, "...more", Color::DARK_GRAY, Color::BLACK);
        }
    }
    
    // Status effects section
    StatusEffectsComponent* effects = manager->get_component<StatusEffectsComponent>(player_id);
    if (effects && !effects->effects.empty()) {
        y++;
        console->draw_string(panel_x + 2, y++, "Effects:", Color::WHITE, Color::BLACK);
        
        const int max_effects_shown = 5;
        int shown = 0;
        for (const auto& effect : effects->effects) {
            if (shown >= max_effects_shown || y >= screen_height - 1) break;
            
            std::string effect_name = get_status_effect_name(effect.type);
            std::string duration_str;
            if (effect.turns_remaining == -1) {
                duration_str = " (perm)";
            } else {
                duration_str = " (" + std::to_string(effect.turns_remaining) + "t)";
            }
            
            // Truncate if needed
            int max_text_len = panel_width - 6;
            std::string display_text = effect_name + duration_str;
            if (static_cast<int>(display_text.length()) > max_text_len) {
                display_text = display_text.substr(0, max_text_len - 2) + "..";
            }
            
            Color effect_color = get_effect_display_color(effect.type);
            console->draw_string(panel_x + 4, y++, display_text, effect_color, Color::BLACK);
            ++shown;
        }
        
        if (effects->effects.size() > static_cast<size_t>(max_effects_shown)) {
            console->draw_string(panel_x + 4, y++, "...more", Color::DARK_GRAY, Color::BLACK);
        }
    }
}

// Draw game over screen
void UI::draw_game_over(bool victory, const std::string& death_reason) {
    int width = console->get_width();
    int height = console->get_height();
    
    // Semi-transparent overlay
    console->fill_rect(0, 0, width, height, ' ', Color::WHITE, Color::BLACK);
    
    std::string title = victory ? "VICTORY!" : "GAME OVER";
    Color title_color = victory ? Color::GREEN : Color::RED;
    
    int title_x = (width - static_cast<int>(title.length())) / 2;
    int title_y = height / 2 - 3;
    
    console->draw_string(title_x, title_y, title, title_color);
    
    // Display death reason if available and this is not a victory
    if (!victory && !death_reason.empty()) {
        int reason_x = (width - static_cast<int>(death_reason.length())) / 2;
        console->draw_string(reason_x, title_y + 2, death_reason, Color::YELLOW);
    }
    
    std::string prompt = "Press any key to continue...";
    int prompt_x = (width - static_cast<int>(prompt.length())) / 2;
    int prompt_y = (!victory && !death_reason.empty()) ? title_y + 4 : title_y + 2;
    console->draw_string(prompt_x, prompt_y, prompt, Color::WHITE);
}

// Show interaction window for an entity
InteractionResult UI::show_interaction_window(EntityId target_id, std::function<void()> render_background) {
    NPCComponent* npc = manager->get_component<NPCComponent>(target_id);
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    
    // Build list of available interactions
    std::vector<InteractionOption> options;
    
    if (npc) {
        options = npc->available_interactions;
    } else {
        // Non-NPC entities have simpler interactions
        if (manager->get_component<ChestComponent>(target_id)) {
            options.emplace_back(InteractionType::OPEN, "Open", "Open the container");
        } else if (manager->get_component<DoorComponent>(target_id)) {
            options.emplace_back(InteractionType::USE, "Open/Close", "Toggle the door");
        } else if (manager->get_component<ItemComponent>(target_id)) {
            options.emplace_back(InteractionType::PICKUP, "Pick up", "Add to inventory");
        }
    }
    
    if (options.empty()) {
        return InteractionResult(InteractionType::NONE, target_id, false);
    }
    
    int selected = 0;
    bool running = true;
    
    while (running) {
        if (render_background) {
            render_background();
        } else {
            console->clear();
        }
        
        std::string target_name = name ? name->name : "Object";
        int panel_width = 40;
        int panel_height = 6 + static_cast<int>(options.size()) * 2;
        int panel_x = (console->get_width() - panel_width) / 2;
        int panel_y = (console->get_height() - panel_height) / 2;
        
        draw_panel(panel_x, panel_y, panel_width, panel_height, "Interact: " + target_name, Color::CYAN);
        
        int y = panel_y + 2;
        
        if (npc) {
            std::string disposition_str;
            switch (npc->disposition) {
                case NPCComponent::Disposition::FRIENDLY: disposition_str = "(Friendly)"; break;
                case NPCComponent::Disposition::NEUTRAL: disposition_str = "(Neutral)"; break;
                case NPCComponent::Disposition::HOSTILE: disposition_str = "(Hostile)"; break;
                case NPCComponent::Disposition::FEARFUL: disposition_str = "(Fearful)"; break;
            }
            console->draw_string(panel_x + 2, y++, disposition_str, Color::DARK_GRAY);
            y++;
        }
        
        for (size_t i = 0; i < options.size(); ++i) {
            const InteractionOption& opt = options[i];
            
            bool is_selected = (static_cast<int>(i) == selected);
            Color color;
            
            if (!opt.enabled) {
                color = Color::DARK_GRAY;
            } else if (is_selected) {
                color = Color::YELLOW;
            } else {
                color = Color::WHITE;
            }
            
            std::string prefix = is_selected ? "> " : "  ";
            std::string num = "[" + std::to_string(i + 1) + "] ";
            std::string option_text = prefix + num + opt.label;
            
            console->draw_string(panel_x + 2, y, option_text, color);
            
            if (is_selected && !opt.description.empty()) {
                console->draw_string(panel_x + 20, y, opt.description, Color::DARK_GRAY);
            }
            
            if (!opt.enabled && is_selected && !opt.disabled_reason.empty()) {
                console->draw_string(panel_x + 20, y, opt.disabled_reason, Color::RED);
            }
            
            y++;
        }
        
        y++;
        auto& bindings = KeyBindings::instance();
        std::string nav = bindings.get_nav_keys();
        std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
        std::string cancel = bindings.get_key_hint(GameAction::CANCEL);
        console->draw_string(panel_x + 2, y, nav + ": Select | " + confirm + ": Confirm | " + cancel + ": Cancel", Color::DARK_GRAY);
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            if (selected > 0) selected--;
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            if (selected < static_cast<int>(options.size()) - 1) selected++;
        } else if (bindings.is_action(key, GameAction::CONFIRM) || bindings.is_action(key, GameAction::INTERACT)) {
            if (options[selected].enabled) {
                return InteractionResult(options[selected].type, target_id, false);
            }
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            return InteractionResult(InteractionType::EXAMINE, target_id, true);
        } else if (key >= Key::NUM_1 && key <= Key::NUM_9) {
            int num = static_cast<int>(key) - static_cast<int>(Key::NUM_1);
            if (num < static_cast<int>(options.size()) && options[num].enabled) {
                return InteractionResult(options[num].type, target_id, false);
            }
        }
    }
    
    return InteractionResult(InteractionType::EXAMINE, target_id, true);
}

// Show target selection when multiple entities are nearby
EntityId UI::show_target_selection(const std::vector<EntityId>& nearby_entities, std::function<void()> render_background) {
    if (nearby_entities.empty()) {
        return 0;
    }
    
    if (nearby_entities.size() == 1) {
        return nearby_entities[0];
    }
    
    int selected = 0;
    bool running = true;
    
    while (running) {
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
            NameComponent* name = manager->get_component<NameComponent>(id);
            RenderComponent* render = manager->get_component<RenderComponent>(id);
            
            bool is_selected = (static_cast<int>(i) == selected);
            Color color = is_selected ? Color::YELLOW : Color::WHITE;
            
            std::string prefix = is_selected ? "> " : "  ";
            std::string entity_name = name ? name->name : "Unknown";
            std::string symbol_str = render ? std::string(1, render->ch) : "?";
            
            std::string line = prefix + "[" + symbol_str + "] " + entity_name;
            console->draw_string(panel_x + 2, y++, line, color);
        }
        
        auto& bindings = KeyBindings::instance();
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
        } else if (bindings.is_action(key, GameAction::CONFIRM) || bindings.is_action(key, GameAction::INTERACT)) {
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

// Show trade interface
UI::TradeResult UI::show_trade_interface(EntityId player_id, EntityId merchant_id, 
                                 std::function<void()> render_background,
                                 std::function<EntityId(const ShopItem&, int, int)> create_item_callback) {
    ShopInventoryComponent* shop = manager->get_component<ShopInventoryComponent>(merchant_id);
    InventoryComponent* player_inv = manager->get_component<InventoryComponent>(player_id);
    NameComponent* merchant_name = manager->get_component<NameComponent>(merchant_id);
    
    if (!shop || !player_inv) {
        return TradeResult{TradeResult::Action::CANCELLED, 0, 0};
    }
    
    std::string shop_title = merchant_name ? merchant_name->name + "'s Shop" : "Shop";
    
    TradeMode mode = TradeMode::BUY;
    int selected = 0;
    int scroll_offset = 0;
    
    while (true) {
        if (render_background) {
            render_background();
        } else {
            console->clear();
        }
        
        int panel_width = 70;
        int panel_height = 25;
        int panel_x = (console->get_width() - panel_width) / 2;
        int panel_y = (console->get_height() - panel_height) / 2;
        
        draw_panel(panel_x, panel_y, panel_width, panel_height, shop_title, Color::YELLOW);
        
        std::string gold_str = "Your Gold: " + std::to_string(player_inv->gold);
        console->draw_string(panel_x + panel_width - static_cast<int>(gold_str.length()) - 3, panel_y, 
                           gold_str, Color::YELLOW);
        
        int tab_y = panel_y + 2;
        std::string buy_tab = mode == TradeMode::BUY ? "[ BUY ]" : "  BUY  ";
        std::string sell_tab = mode == TradeMode::SELL ? "[ SELL ]" : "  SELL  ";
        console->draw_string(panel_x + 3, tab_y, buy_tab, 
                           mode == TradeMode::BUY ? Color::GREEN : Color::DARK_GRAY);
        console->draw_string(panel_x + 12, tab_y, sell_tab, 
                           mode == TradeMode::SELL ? Color::RED : Color::DARK_GRAY);
        
        int content_y = tab_y + 2;
        int max_visible = panel_height - 8;
        
        if (mode == TradeMode::BUY) {
            console->draw_string(panel_x + 3, content_y - 1, "Item", Color::CYAN);
            console->draw_string(panel_x + 35, content_y - 1, "Price", Color::CYAN);
            console->draw_string(panel_x + 45, content_y - 1, "Stock", Color::CYAN);
            
            if (shop->items.empty()) {
                console->draw_string(panel_x + 3, content_y + 1, "(No items for sale)", Color::DARK_GRAY);
            } else {
                int visible_start = scroll_offset;
                int visible_end = (std::min)(visible_start + max_visible, static_cast<int>(shop->items.size()));
                
                for (int i = visible_start; i < visible_end; ++i) {
                    const ShopItem& item = shop->items[i];
                    bool is_selected = (i == selected);
                    
                    int actual_price = static_cast<int>(item.buy_price * shop->buy_multiplier);
                    bool can_afford = player_inv->gold >= actual_price;
                    bool in_stock = item.stock != 0 && item.available;
                    
                    Color color;
                    if (!in_stock) {
                        color = Color::DARK_GRAY;
                    } else if (!can_afford) {
                        color = Color::RED;
                    } else if (is_selected) {
                        color = Color::YELLOW;
                    } else {
                        color = Color::WHITE;
                    }
                    
                    std::string prefix = is_selected ? "> " : "  ";
                    std::string symbol_str(1, item.symbol);
                    std::string item_line = prefix + "[" + symbol_str + "] " + item.name;
                    
                    console->draw_string(panel_x + 2, content_y, item_line, color);
                    console->draw_string(panel_x + 35, content_y, std::to_string(actual_price) + "g", 
                                       can_afford ? Color::YELLOW : Color::RED);
                    
                    std::string stock_str = item.stock < 0 ? "INF" : std::to_string(item.stock);
                    console->draw_string(panel_x + 45, content_y, stock_str, 
                                       in_stock ? Color::GREEN : Color::DARK_GRAY);
                    
                    content_y++;
                }
                
                if (scroll_offset > 0) {
                    console->draw_string(panel_x + panel_width - 4, tab_y + 3, "^", Color::CYAN);
                }
                if (visible_end < static_cast<int>(shop->items.size())) {
                    console->draw_string(panel_x + panel_width - 4, panel_y + panel_height - 4, "v", Color::CYAN);
                }
                
                if (selected >= 0 && selected < static_cast<int>(shop->items.size())) {
                    const ShopItem& sel_item = shop->items[selected];
                    int desc_y = panel_y + panel_height - 3;
                    console->draw_string(panel_x + 3, desc_y, sel_item.description, Color::DARK_GRAY);
                }
            }
        } else {
            console->draw_string(panel_x + 3, content_y - 1, "Your Item", Color::CYAN);
            console->draw_string(panel_x + 35, content_y - 1, "Sell Value", Color::CYAN);
            
            if (player_inv->items.empty()) {
                console->draw_string(panel_x + 3, content_y + 1, "(No items to sell)", Color::DARK_GRAY);
            } else {
                int visible_start = scroll_offset;
                int visible_end = (std::min)(visible_start + max_visible, static_cast<int>(player_inv->items.size()));
                
                for (int i = visible_start; i < visible_end; ++i) {
                    EntityId item_id = player_inv->items[i];
                    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
                    ItemComponent* item_comp = manager->get_component<ItemComponent>(item_id);
                    RenderComponent* item_render = manager->get_component<RenderComponent>(item_id);
                    
                    bool is_selected = (i == selected);
                    Color color = is_selected ? Color::YELLOW : Color::WHITE;
                    
                    std::string prefix = is_selected ? "> " : "  ";
                    char sym = item_render ? item_render->ch : '?';
                    std::string name = item_name ? item_name->name : "Unknown";
                    std::string item_line = prefix + "[" + std::string(1, sym) + "] " + name;
                    
                    int sell_value = item_comp ? static_cast<int>(item_comp->value * shop->sell_multiplier) : 1;
                    
                    console->draw_string(panel_x + 2, content_y, item_line, color);
                    console->draw_string(panel_x + 35, content_y, std::to_string(sell_value) + "g", Color::YELLOW);
                    
                    content_y++;
                }
                
                if (scroll_offset > 0) {
                    console->draw_string(panel_x + panel_width - 4, tab_y + 3, "^", Color::CYAN);
                }
                if (visible_end < static_cast<int>(player_inv->items.size())) {
                    console->draw_string(panel_x + panel_width - 4, panel_y + panel_height - 4, "v", Color::CYAN);
                }
            }
        }
        
        auto& bindings = KeyBindings::instance();
        std::string nav = bindings.get_nav_keys();
        std::string confirm = bindings.get_key_hint(GameAction::CONFIRM);
        console->draw_string(panel_x + 2, panel_y + panel_height - 2, 
                           "TAB: Switch Mode | " + nav + ": Select | " + confirm + ": Confirm | Esc: Exit", Color::DARK_GRAY);
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        int list_size = 0;
        if (mode == TradeMode::BUY) {
            list_size = static_cast<int>(shop->items.size());
        } else {
            list_size = static_cast<int>(player_inv->items.size());
        }
        
        switch (key) {
            case Key::TAB:
                mode = (mode == TradeMode::BUY) ? TradeMode::SELL : TradeMode::BUY;
                selected = 0;
                scroll_offset = 0;
                break;
                
            default:
                if (bindings.is_action(key, GameAction::MENU_UP)) {
                    if (selected > 0) {
                        selected--;
                        if (selected < scroll_offset) {
                            scroll_offset = selected;
                        }
                    }
                } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
                    if (selected < list_size - 1) {
                        selected++;
                        if (selected >= scroll_offset + max_visible) {
                            scroll_offset = selected - max_visible + 1;
                        }
                    }
                } else if (bindings.is_action(key, GameAction::CONFIRM) || bindings.is_action(key, GameAction::INTERACT)) {
                    if (mode == TradeMode::BUY && !shop->items.empty()) {
                    const ShopItem& item = shop->items[selected];
                    int price = static_cast<int>(item.buy_price * shop->buy_multiplier);
                    
                    if (player_inv->gold >= price && item.stock != 0 && item.available) {
                        if (confirm_action("Buy " + item.name + " for " + std::to_string(price) + " gold?")) {
                            player_inv->gold -= price;
                            shop->purchase(selected);
                            
                            if (create_item_callback) {
                                EntityId new_item = create_item_callback(item, 0, 0);
                                if (new_item != 0) {
                                    player_inv->add_item(new_item);
                                }
                            }
                            
                            return TradeResult{TradeResult::Action::BOUGHT, static_cast<size_t>(selected), -price};
                        }
                    }
                } else if (mode == TradeMode::SELL && !player_inv->items.empty()) {
                    EntityId item_id = player_inv->items[selected];
                    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
                    ItemComponent* item_comp = manager->get_component<ItemComponent>(item_id);
                    
                    int sell_value = item_comp ? static_cast<int>(item_comp->value * shop->sell_multiplier) : 1;
                    std::string name = item_name ? item_name->name : "item";
                    
                    if (confirm_action("Sell " + name + " for " + std::to_string(sell_value) + " gold?")) {
                        player_inv->gold += sell_value;
                        player_inv->remove_item(item_id);
                        manager->destroy_entity(item_id);
                        
                        if (selected >= static_cast<int>(player_inv->items.size()) && selected > 0) {
                            selected--;
                        }
                        
                        return TradeResult{TradeResult::Action::SOLD, static_cast<size_t>(selected), sell_value};
                    }
                }
                } else if (bindings.is_action(key, GameAction::CANCEL)) {
                    return TradeResult{TradeResult::Action::CANCELLED, 0, 0};
                }
                break;
        }
    }
    
    return TradeResult{TradeResult::Action::CANCELLED, 0, 0};
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
