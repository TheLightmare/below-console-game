// gameplay_ui.inl - UI-related methods for BaseGameplayScene
// This file is included by base_gameplay_scene.hpp - do not include directly

// ==================== Stats & Inventory Screens ====================

void show_stats_screen() {
    console->clear();
    
    StatsComponent* stats = manager->get_component<StatsComponent>(player_id);
    NameComponent* name = manager->get_component<NameComponent>(player_id);
    
    if (!stats) return;
    
    bool in_stats = true;
    int selected = 0;  // 0=HP, 1=Attack, 2=Defense
    const int num_options = 3;
    
    while (in_stats) {
        console->clear();
        
        int panel_width = 60;
        int panel_height = 25;
        int panel_x = (console->get_width() - panel_width) / 2;
        int panel_y = (console->get_height() - panel_height) / 2;
        
        std::string title = name ? name->name + " - Character Stats" : "Character Stats";
        ui->draw_panel(panel_x, panel_y, panel_width, panel_height, title, Color::CYAN);
        
        int y = panel_y + 2;
        int x = panel_x + 2;
        
        // Current stats display
        console->draw_string(x, y++, "Level: " + std::to_string(stats->level), Color::YELLOW);
        console->draw_string(x, y++, "Experience: " + std::to_string(stats->experience) + "/" + std::to_string(stats->get_exp_to_level()), Color::WHITE);
        y++;
        
        console->draw_string(x, y++, "Attribute Points Available: " + std::to_string(stats->attribute_points), 
                           stats->attribute_points > 0 ? Color::GREEN : Color::DARK_GRAY);
        y++;
        
        if (stats->attribute_points > 0) {
            console->draw_string(x, y++, "Use UP/DOWN to select, ENTER to spend point, ESC to exit", Color::DARK_GRAY);
        } else {
            console->draw_string(x, y++, "No points available - gain levels to earn more!", Color::DARK_GRAY);
            console->draw_string(x, y++, "Press ESC to exit", Color::DARK_GRAY);
        }
        y++;
        
        console->draw_string(x, y++, "Current Attributes:", Color::CYAN);
        y++;
        
        // HP option
        Color hp_color = (selected == 0 && stats->attribute_points > 0) ? Color::YELLOW : Color::WHITE;
        std::string hp_marker = (selected == 0 && stats->attribute_points > 0) ? "> " : "  ";
        console->draw_string(x, y++, hp_marker + "Max HP: " + std::to_string(stats->max_hp) + 
                           " (Current: " + std::to_string(stats->current_hp) + ")", hp_color);
        console->draw_string(x + 2, y++, "[+5 HP per point]", Color::DARK_GRAY);
        y++;
        
        // Attack option
        Color atk_color = (selected == 1 && stats->attribute_points > 0) ? Color::YELLOW : Color::WHITE;
        std::string atk_marker = (selected == 1 && stats->attribute_points > 0) ? "> " : "  ";
        console->draw_string(x, y++, atk_marker + "Attack: " + std::to_string(stats->attack), atk_color);
        console->draw_string(x + 2, y++, "[+2 Attack per point]", Color::DARK_GRAY);
        y++;
        
        // Defense option
        Color def_color = (selected == 2 && stats->attribute_points > 0) ? Color::YELLOW : Color::WHITE;
        std::string def_marker = (selected == 2 && stats->attribute_points > 0) ? "> " : "  ";
        console->draw_string(x, y++, def_marker + "Defense: " + std::to_string(stats->defense), def_color);
        console->draw_string(x + 2, y++, "[+2 Defense per point]", Color::DARK_GRAY);
        
        console->present();
        
        // Handle input
        Key key = Input::wait_for_key();
        
        switch (key) {
            case Key::UP:
                if (stats->attribute_points > 0) {
                    selected = (selected - 1 + num_options) % num_options;
                }
                break;
                
            case Key::DOWN:
                if (stats->attribute_points > 0) {
                    selected = (selected + 1) % num_options;
                }
                break;
                
            case Key::ENTER:
                if (stats->attribute_points > 0) {
                    stats->attribute_points--;
                    
                    switch (selected) {
                        case 0:  // HP
                            stats->max_hp += 5;
                            stats->current_hp += 5;  // Also heal when increasing max HP
                            add_message("Max HP increased by 5!");
                            break;
                        case 1:  // Attack
                            stats->attack += 2;
                            add_message("Attack increased by 2!");
                            break;
                        case 2:  // Defense
                            stats->defense += 2;
                            add_message("Defense increased by 2!");
                            break;
                    }
                }
                break;
                
            case Key::ESCAPE:
                in_stats = false;
                break;
                
            default:
                break;
        }
    }
}

void show_inventory() {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(player_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(player_id);
    
    if (!inv || !anatomy || !equip) return;
    
    // Build equipment slot list for selection
    std::vector<std::string> equipment_slots;
    auto build_equipment_slots = [&]() {
        equipment_slots.clear();
        std::unordered_map<LimbType, std::vector<Limb*>> limb_groups;
        for (auto& limb : anatomy->limbs) {
            if (limb.functional) {
                limb_groups[limb.type].push_back(&limb);
            }
        }
        auto add_group = [&](LimbType type) {
            if (limb_groups.find(type) != limb_groups.end()) {
                for (Limb* limb : limb_groups[type]) {
                    equipment_slots.push_back(limb->name);
                }
            }
        };
        add_group(LimbType::HEAD);
        add_group(LimbType::TORSO);
        add_group(LimbType::HAND);
        add_group(LimbType::ARM);
        add_group(LimbType::FOOT);
        add_group(LimbType::LEG);
        add_group(LimbType::OTHER);
        equipment_slots.push_back("back"); // Special slot
    };
    build_equipment_slots();
    
    int selected_index = 0;
    int inventory_scroll = 0;
    int equipment_scroll = 0;
    int equipment_selected = 0;
    bool equipment_mode = false;  // false = inventory mode, true = equipment mode
    bool running = true;
    
    // Calculate visible lines for scrolling (panel height - 2 for borders)
    int mid_x = console->get_width() / 2;
    int equipment_panel_height = 25;
    int equipment_visible_lines = equipment_panel_height - 2;
    int inventory_panel_height = 20;
    int inventory_visible_lines = inventory_panel_height - 4;  // Account for header and gold line
    
    while (running) {
        console->clear();
        
        // Draw inventory items with selection (highlight if in inventory mode)
        draw_inventory_interactive(5, 5, mid_x - 10, inventory_panel_height, player_id, equipment_mode ? -1 : selected_index, inventory_scroll);
        int selected_line = draw_equipment_scrollable_selectable(mid_x + 5, 5, mid_x - 10, equipment_panel_height, player_id, equipment_scroll, 
                                              equipment_mode ? equipment_selected : -1, equipment_slots);
        
        // Auto-scroll equipment panel to keep selection visible (using line index)
        if (equipment_mode && selected_line >= 0) {
            if (selected_line < equipment_scroll) {
                equipment_scroll = selected_line;
            } else if (selected_line >= equipment_scroll + equipment_visible_lines) {
                equipment_scroll = selected_line - equipment_visible_lines + 1;
            }
        }
        
        ui->draw_stats(5, 27, player_id);
        
        // Draw controls
        console->draw_string(5, console->get_height() - 4, "Controls:", Color::YELLOW);
        if (equipment_mode) {
            console->draw_string(5, console->get_height() - 3, "UP/DOWN: Select | TAB: Inventory | X: Unequip | ESC: Close", Color::WHITE);
        } else {
            console->draw_string(5, console->get_height() - 3, "UP/DN: Select | TAB: Equip | E: Equip | U: Use | R: Drop | T: Throw | I: Info | ESC", Color::WHITE);
        }
        
        console->present();
        
        // Handle input
        Key key = Input::get_key();
        
        switch (key) {
            case Key::UP:
                if (equipment_mode) {
                    if (equipment_selected > 0) {
                        equipment_selected--;
                    }
                } else {
                    if (selected_index > 0) {
                        selected_index--;
                        // Scroll up if needed to keep selection visible
                        if (selected_index < inventory_scroll) {
                            inventory_scroll = selected_index;
                        }
                    }
                }
                break;
                
            case Key::DOWN:
                if (equipment_mode) {
                    if (equipment_selected < static_cast<int>(equipment_slots.size()) - 1) {
                        equipment_selected++;
                    }
                } else {
                    if (selected_index < static_cast<int>(inv->items.size()) - 1) {
                        selected_index++;
                        // Scroll down if needed to keep selection visible
                        if (selected_index >= inventory_scroll + inventory_visible_lines) {
                            inventory_scroll = selected_index - inventory_visible_lines + 1;
                        }
                    }
                }
                break;
                
            case Key::TAB:
                // Toggle between inventory and equipment mode
                equipment_mode = !equipment_mode;
                break;
                
            case Key::X:
                // Unequip selected equipment slot
                if (equipment_mode && equipment_selected >= 0 && 
                    equipment_selected < static_cast<int>(equipment_slots.size())) {
                    const std::string& slot_name = equipment_slots[equipment_selected];
                    inventory_system->unequip_item(player_id, slot_name);
                }
                break;
                
            case Key::E:
                if (!equipment_mode) {
                    equip_item(selected_index);
                }
                break;
                
            case Key::U:
                if (!equipment_mode) {
                    use_item(selected_index);
                }
                break;
                
            case Key::R:
                if (!equipment_mode) {
                    drop_item(selected_index);
                    if (selected_index >= static_cast<int>(inv->items.size()) && selected_index > 0) {
                        selected_index--;
                    }
                }
                break;
                
            case Key::T:
                if (!equipment_mode && selected_index < static_cast<int>(inv->items.size())) {
                    // Throw selected item - close inventory and go to targeting
                    running = false;
                    throw_item_from_inventory(selected_index);
                }
                break;
                
            case Key::I:
                if (!equipment_mode) {
                    show_item_info(selected_index);
                }
                break;
                
            case Key::ESCAPE:
                running = false;
                break;
                
            default:
                break;
        }
    }
}

// ==================== Private Drawing Methods ====================

private:
void draw_chest_contents(int x, int y, int width, int height, EntityId chest_entity_id, int selected_index) {
    ChestComponent* chest = manager->get_component<ChestComponent>(chest_entity_id);
    if (!chest) return;
    
    NameComponent* chest_name = manager->get_component<NameComponent>(chest_entity_id);
    std::string title = chest_name ? chest_name->name : "Chest";
    
    ui->draw_panel(x, y, width, height, title);
    
    int current_y = y + 1;
    console->draw_string(x + 2, current_y++, "Items: " + std::to_string(chest->items.size()), Color::YELLOW);
    current_y++;
    
    if (chest->items.empty()) {
        console->draw_string(x + 2, current_y, "(Empty)", Color::DARK_GRAY);
    } else {
        for (size_t i = 0; i < chest->items.size() && current_y < y + height - 1; ++i) {
            EntityId item_id = chest->items[i];
            NameComponent* item_name = manager->get_component<NameComponent>(item_id);
            ItemComponent* item = manager->get_component<ItemComponent>(item_id);
            
            if (item_name) {
                bool is_selected = (static_cast<int>(i) == selected_index);
                std::string prefix = is_selected ? "> " : "  ";
                std::string item_text = prefix + std::to_string(i + 1) + ". " + item_name->name;
                
                // Add item type indicator
                if (item) {
                    switch (item->type) {
                        case ItemComponent::Type::WEAPON: item_text += " [W]"; break;
                        case ItemComponent::Type::ARMOR: item_text += " [A]"; break;
                        case ItemComponent::Type::HELMET: item_text += " [H]"; break;
                        case ItemComponent::Type::BOOTS: item_text += " [B]"; break;
                        case ItemComponent::Type::SHIELD: item_text += " [S]"; break;
                        case ItemComponent::Type::CONSUMABLE: item_text += " [C]"; break;
                        default: break;
                    }
                }
                
                Color color = is_selected ? Color::YELLOW : Color::CYAN;
                console->draw_string(x + 2, current_y++, item_text, color);
            }
        }
    }
}

void draw_player_inventory_for_chest(int x, int y, int width, int height, EntityId entity_id, int selected_index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    if (!inv) return;
    
    ui->draw_panel(x, y, width, height, "Your Inventory");
    
    int current_y = y + 1;
    console->draw_string(x + 2, current_y++, "Gold: " + std::to_string(inv->gold), Color::YELLOW);
    console->draw_string(x + 2, current_y++, "Items:", Color::WHITE);
    
    if (inv->items.empty()) {
        console->draw_string(x + 2, current_y, "(Empty)", Color::DARK_GRAY);
    } else {
        for (size_t i = 0; i < inv->items.size() && current_y < y + height - 1; ++i) {
            EntityId item_id = inv->items[i];
            NameComponent* item_name = manager->get_component<NameComponent>(item_id);
            ItemComponent* item = manager->get_component<ItemComponent>(item_id);
            
            if (item_name) {
                bool is_selected = (static_cast<int>(i) == selected_index);
                std::string prefix = is_selected ? "> " : "  ";
                std::string item_text = prefix + std::to_string(i + 1) + ". " + item_name->name;
                
                // Add item type indicator
                if (item) {
                    switch (item->type) {
                        case ItemComponent::Type::WEAPON: item_text += " [W]"; break;
                        case ItemComponent::Type::ARMOR: item_text += " [A]"; break;
                        case ItemComponent::Type::HELMET: item_text += " [H]"; break;
                        case ItemComponent::Type::BOOTS: item_text += " [B]"; break;
                        case ItemComponent::Type::SHIELD: item_text += " [S]"; break;
                        case ItemComponent::Type::CONSUMABLE: item_text += " [C]"; break;
                        default: break;
                    }
                }
                
                Color color = is_selected ? Color::YELLOW : Color::CYAN;
                console->draw_string(x + 2, current_y++, item_text, color);
            }
        }
    }
}

void draw_equipment_scrollable(int x, int y, int width, int height, EntityId entity_id, int scroll_offset) {
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(entity_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    
    if (!anatomy || !equip) {
        ui->draw_panel(x, y, width, height, "Equipment");
        console->draw_string(x + 2, y + 2, "No equipment data", Color::DARK_GRAY);
        return;
    }
    
    ui->draw_panel(x, y, width, height, "Equipment");
    
    // Build list of all equipment lines
    std::vector<std::pair<std::string, Color>> equipment_lines;
    
    // Group limbs by type
    std::unordered_map<LimbType, std::vector<Limb*>> limb_groups;
    for (auto& limb : anatomy->limbs) {
        if (limb.functional) {
            limb_groups[limb.type].push_back(&limb);
        }
    }
    
    // Add equipment by limb type
    auto add_limb_group = [&](LimbType type, const std::string& group_name) {
        if (limb_groups.find(type) != limb_groups.end()) {
            equipment_lines.push_back({group_name + ":", Color::YELLOW});
            
            for (Limb* limb : limb_groups[type]) {
                EntityId equipped = equip->get_equipped(limb->name);
                std::string slot_text = "  " + limb->name + ": ";
                
                if (equipped != 0) {
                    NameComponent* item_name = manager->get_component<NameComponent>(equipped);
                    slot_text += item_name ? item_name->name : "???";
                    equipment_lines.push_back({slot_text, Color::GREEN});
                } else {
                    slot_text += "(empty)";
                    equipment_lines.push_back({slot_text, Color::DARK_GRAY});
                }
            }
        }
    };
    
    add_limb_group(LimbType::HEAD, "Head");
    add_limb_group(LimbType::TORSO, "Torso");
    add_limb_group(LimbType::HAND, "Hands");
    add_limb_group(LimbType::ARM, "Arms");
    add_limb_group(LimbType::FOOT, "Feet");
    add_limb_group(LimbType::LEG, "Legs");
    add_limb_group(LimbType::OTHER, "Other");
    
    // Add special slots
    equipment_lines.push_back({"Other:", Color::YELLOW});
    std::string back_text = "  back: ";
    if (equip->back != 0) {
        NameComponent* item_name = manager->get_component<NameComponent>(equip->back);
        back_text += item_name ? item_name->name : "???";
        equipment_lines.push_back({back_text, Color::GREEN});
    } else {
        back_text += "(empty)";
        equipment_lines.push_back({back_text, Color::DARK_GRAY});
    }
    
    // Render visible lines with scroll
    int current_y = y + 1;
    int max_visible_lines = height - 2;
    int start_line = scroll_offset;
    int end_line = (std::min)(start_line + max_visible_lines, static_cast<int>(equipment_lines.size()));
    
    for (int i = start_line; i < end_line && current_y < y + height - 1; ++i) {
        console->draw_string(x + 2, current_y++, equipment_lines[i].first, equipment_lines[i].second);
    }
    
    // Draw scroll indicators if needed
    if (scroll_offset > 0) {
        console->draw_string(x + width - 3, y + 1, "▲", Color::CYAN);
    }
    if (end_line < static_cast<int>(equipment_lines.size())) {
        console->draw_string(x + width - 3, y + height - 2, "▼", Color::CYAN);
    }
}

// Version with selection support for unequipping
// Returns the line index where the selected slot appears (for scroll calculation)
int draw_equipment_scrollable_selectable(int x, int y, int width, int height, EntityId entity_id, 
                                           int scroll_offset, int selected_slot, 
                                           const std::vector<std::string>& slot_names) {
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(entity_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(entity_id);
    
    if (!anatomy || !equip) {
        ui->draw_panel(x, y, width, height, "Equipment");
        console->draw_string(x + 2, y + 2, "No equipment data", Color::DARK_GRAY);
        return 0;
    }
    
    ui->draw_panel(x, y, width, height, "Equipment");
    
    // Build list of equipment lines with slot index tracking
    struct EquipLine {
        std::string text;
        Color color;
        int slot_index;  // -1 for headers, otherwise index into slot_names
    };
    std::vector<EquipLine> equipment_lines;
    int selected_line_index = -1;  // Track which line the selected slot is on
    
    // Group limbs by type
    std::unordered_map<LimbType, std::vector<Limb*>> limb_groups;
    for (auto& limb : anatomy->limbs) {
        if (limb.functional) {
            limb_groups[limb.type].push_back(&limb);
        }
    }
    
    // Track current slot index as we build lines
    int current_slot_idx = 0;
    
    // Add equipment by limb type
    auto add_limb_group = [&](LimbType type, const std::string& group_name) {
        if (limb_groups.find(type) != limb_groups.end() && !limb_groups[type].empty()) {
            equipment_lines.push_back({group_name + ":", Color::YELLOW, -1});
            
            for (Limb* limb : limb_groups[type]) {
                EntityId equipped = equip->get_equipped(limb->name);
                bool is_selected = (current_slot_idx == selected_slot);
                
                if (is_selected) {
                    selected_line_index = static_cast<int>(equipment_lines.size());
                }
                
                std::string prefix = is_selected ? "> " : "  ";
                std::string slot_text = prefix + limb->name + ": ";
                
                if (equipped != 0) {
                    NameComponent* item_name = manager->get_component<NameComponent>(equipped);
                    slot_text += item_name ? item_name->name : "???";
                    equipment_lines.push_back({slot_text, is_selected ? Color::CYAN : Color::GREEN, current_slot_idx});
                } else {
                    slot_text += "(empty)";
                    equipment_lines.push_back({slot_text, is_selected ? Color::CYAN : Color::DARK_GRAY, current_slot_idx});
                }
                current_slot_idx++;
            }
        }
    };
    
    add_limb_group(LimbType::HEAD, "Head");
    add_limb_group(LimbType::TORSO, "Torso");
    add_limb_group(LimbType::HAND, "Hands");
    add_limb_group(LimbType::ARM, "Arms");
    add_limb_group(LimbType::FOOT, "Feet");
    add_limb_group(LimbType::LEG, "Legs");
    add_limb_group(LimbType::OTHER, "Other");
    
    // Add special slots
    if (current_slot_idx < static_cast<int>(slot_names.size())) {
        equipment_lines.push_back({"Special:", Color::YELLOW, -1});
        
        // Back slot
        bool back_selected = (current_slot_idx == selected_slot);
        if (back_selected) {
            selected_line_index = static_cast<int>(equipment_lines.size());
        }
        std::string back_prefix = back_selected ? "> " : "  ";
        std::string back_text = back_prefix + "back: ";
        if (equip->back != 0) {
            NameComponent* item_name = manager->get_component<NameComponent>(equip->back);
            back_text += item_name ? item_name->name : "???";
            equipment_lines.push_back({back_text, back_selected ? Color::CYAN : Color::GREEN, current_slot_idx});
        } else {
            back_text += "(empty)";
            equipment_lines.push_back({back_text, back_selected ? Color::CYAN : Color::DARK_GRAY, current_slot_idx});
        }
    }
    
    // Render visible lines with scroll
    int current_y = y + 1;
    int max_visible_lines = height - 2;
    int start_line = scroll_offset;
    int end_line = (std::min)(start_line + max_visible_lines, static_cast<int>(equipment_lines.size()));
    
    for (int i = start_line; i < end_line && current_y < y + height - 1; ++i) {
        console->draw_string(x + 2, current_y++, equipment_lines[i].text, equipment_lines[i].color);
    }
    
    // Draw scroll indicators if needed
    if (scroll_offset > 0) {
        console->draw_string(x + width - 3, y + 1, "^", Color::CYAN);
    }
    if (end_line < static_cast<int>(equipment_lines.size())) {
        console->draw_string(x + width - 3, y + height - 2, "v", Color::CYAN);
    }
    
    return selected_line_index;
}

void draw_inventory_interactive(int x, int y, int width, int height, EntityId entity_id, int selected_index, int scroll_offset = 0) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(entity_id);
    if (!inv) return;
    
    ui->draw_panel(x, y, width, height, "Inventory");
    
    int current_y = y + 1;
    console->draw_string(x + 2, current_y++, "Gold: " + std::to_string(inv->gold), Color::YELLOW);
    console->draw_string(x + 2, current_y++, "Items: " + std::to_string(inv->items.size()) + "/" + std::to_string(inv->max_items), Color::WHITE);
    
    int max_visible_items = height - 4;  // Account for header and border
    int start_item = scroll_offset;
    int end_item = (std::min)(start_item + max_visible_items, static_cast<int>(inv->items.size()));
    
    for (int i = start_item; i < end_item && current_y < y + height - 1; ++i) {
        EntityId item_id = inv->items[i];
        NameComponent* item_name = manager->get_component<NameComponent>(item_id);
        ItemComponent* item = manager->get_component<ItemComponent>(item_id);
        
        if (item_name) {
            bool is_selected = (i == selected_index);
            std::string prefix = is_selected ? "> " : "  ";
            std::string item_text = prefix + std::to_string(i + 1) + ". " + item_name->name;
            
            // Add item type indicator
            if (item) {
                switch (item->type) {
                    case ItemComponent::Type::WEAPON: item_text += " [W]"; break;
                    case ItemComponent::Type::ARMOR: item_text += " [A]"; break;
                    case ItemComponent::Type::HELMET: item_text += " [H]"; break;
                    case ItemComponent::Type::BOOTS: item_text += " [B]"; break;
                    case ItemComponent::Type::CONSUMABLE: item_text += " [C]"; break;
                    default: break;
                }
            }
            
            Color color = is_selected ? Color::YELLOW : Color::CYAN;
            console->draw_string(x + 2, current_y++, item_text, color);
        }
    }
    
    if (inv->items.empty()) {
        console->draw_string(x + 4, current_y, "(empty)", Color::DARK_GRAY);
    }
    
    // Draw scroll indicators if needed
    if (scroll_offset > 0) {
        console->draw_string(x + width - 3, y + 3, "^", Color::CYAN);
    }
    if (end_item < static_cast<int>(inv->items.size())) {
        console->draw_string(x + width - 3, y + height - 2, "v", Color::CYAN);
    }
}

// ==================== Item Effect Application ====================

// Apply an item effect to a target entity using data-driven behavior from JSON
// user_id is the entity that used/threw the item (0 if self-use or unknown)
void apply_item_effect(EntityId target_id, const ItemEffectComponent::Effect& effect, EntityId user_id = 0) {
    StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "Target";
    bool is_player = (target_id == player_id);
    
    // Get effect type ID from enum
    std::string effect_type_id = item_effect_type_to_string(static_cast<int>(effect.type));
    
    // Look up effect definition in registry
    const ItemEffectTypeDefinition* def = ItemEffectTypeRegistry::instance().get(effect_type_id);
    
    // Helper lambda to get the appropriate message
    auto get_message = [&](int amount) -> std::string {
        if (!effect.message.empty()) {
            return format_effect_message(effect.message, target_name, is_player);
        }
        if (def) {
            const std::string& tmpl = is_player ? def->message_player : def->message_other;
            if (!tmpl.empty()) {
                return ItemEffectTypeRegistry::format_message(tmpl, target_name, amount);
            }
        }
        return "";
    };
    
    // If no definition found, fall back to legacy handling for CUSTOM type
    if (!def) {
        if (effect.type == ItemEffectComponent::EffectType::CUSTOM) {
            auto callback = effect.get_callback();
            if (callback) {
                callback(target_id, user_id, manager, [this, &target_name, is_player](const std::string& msg) {
                    add_message(format_effect_message(msg, target_name, is_player));
                });
            } else if (!effect.message.empty()) {
                add_message(format_effect_message(effect.message, target_name, is_player));
            }
        }
        return;
    }
    
    // Process based on behavior type from JSON
    if (def->behavior == "instant_heal") {
        if (stats) {
            int amount;
            if (def->is_percent) {
                amount = (stats->max_hp * effect.magnitude) / 100;
            } else {
                amount = effect.magnitude;
            }
            int healed = stats->heal(amount);
            std::string msg = get_message(healed);
            if (!msg.empty()) add_message(msg);
        }
    }
    else if (def->behavior == "instant_damage") {
        if (stats) {
            stats->take_damage(effect.magnitude);
            std::string msg = get_message(effect.magnitude);
            if (!msg.empty()) add_message(msg);
        }
    }
    else if (def->behavior == "instant_stat") {
        if (stats) {
            if (def->stat == "experience") {
                stats->gain_experience(effect.magnitude);
            } else if (def->stat == "attack") {
                if (def->modifier_type == "add") stats->attack += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->attack = std::max(1, stats->attack - effect.magnitude);
            } else if (def->stat == "defense") {
                if (def->modifier_type == "add") stats->defense += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->defense = std::max(0, stats->defense - effect.magnitude);
            }
            std::string msg = get_message(effect.magnitude);
            if (!msg.empty()) add_message(msg);
        }
    }
    else if (def->behavior == "stat_modifier") {
        // If duration specified, apply as status effect; otherwise instant
        if (status_effect_system && effect.duration > 0 && !def->status_effect_id.empty()) {
            status_effect_system->apply_effect(target_id, def->status_effect_id, 
                effect.magnitude, effect.duration, user_id, false, effect.message);
        } else if (stats) {
            if (def->stat == "attack") {
                if (def->modifier_type == "add") stats->attack += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->attack = std::max(1, stats->attack - effect.magnitude);
            } else if (def->stat == "defense") {
                if (def->modifier_type == "add") stats->defense += effect.magnitude;
                else if (def->modifier_type == "subtract") stats->defense = std::max(0, stats->defense - effect.magnitude);
            }
            std::string msg = get_message(effect.magnitude);
            if (!msg.empty()) add_message(msg);
        }
    }
    else if (def->behavior == "apply_status") {
        if (status_effect_system && !def->status_effect_id.empty()) {
            int duration = effect.duration > 0 ? effect.duration : def->default_duration;
            status_effect_system->apply_effect(target_id, def->status_effect_id, 
                effect.magnitude, duration, user_id, false, effect.message);
        }
    }
    else if (def->behavior == "custom") {
        // Custom behavior - use callback or custom handler name
        auto callback = effect.get_callback();
        if (callback) {
            callback(target_id, user_id, manager, [this, &target_name, is_player](const std::string& msg) {
                add_message(format_effect_message(msg, target_name, is_player));
            });
        } else {
            // Could expand this to look up handlers by def->custom_handler
            std::string msg = get_message(effect.magnitude);
            if (!msg.empty()) add_message(msg);
        }
    }
    else {
        // Unknown behavior - just show message
        std::string msg = get_message(effect.magnitude);
        if (!msg.empty()) add_message(msg);
    }
}

// ==================== Inventory Item Operations ====================

void equip_item(int index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(player_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(player_id);
    
    if (!inv || !anatomy || !equip) return;
    if (index < 0 || index >= static_cast<int>(inv->items.size())) return;
    
    EntityId item_id = inv->items[index];
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(item_id);
    
    if (!item) return;
    
    // Check if item is equippable
    if (item->equip_slot == ItemComponent::EquipSlot::NONE) {
        add_message("This item cannot be equipped.");
        return;
    }
    
    // Find available limbs for this item
    std::vector<Limb*> available_limbs = anatomy->get_limbs_of_type(item->required_limb_type);
    
    if (available_limbs.empty()) {
        add_message("You don't have the right limbs to equip this!");
        return;
    }
    
    // Check if we have enough limbs
    if (static_cast<int>(available_limbs.size()) < item->required_limbs) {
        add_message("You need " + std::to_string(item->required_limbs) + " " + 
                   get_limb_type_name(item->required_limb_type) + "(s) to equip this!");
        return;
    }
    
    // Find first available limb(s) that aren't already equipped
    std::vector<Limb*> limbs_to_use;
    for (auto* limb : available_limbs) {
        if (equip->get_equipped(limb->name) == 0) {
            limbs_to_use.push_back(limb);
            if (static_cast<int>(limbs_to_use.size()) >= item->required_limbs) break;
        }
    }
    
    if (static_cast<int>(limbs_to_use.size()) < item->required_limbs) {
        add_message("Not enough free limbs! Unequip something first.");
        return;
    }
    
    // Equip to limbs
    for (auto* limb : limbs_to_use) {
        equip->equip_to_limb(limb->name, item_id);
    }
    
    // Remove from inventory
    inv->remove_item(item_id);
    
    std::string name = item_name ? item_name->name : "Item";
    add_message("Equipped " + name + ".");
    
    // Apply stat bonuses
    apply_item_stats(item_id, true);
    
    // Apply ON_EQUIP effects
    if (effects) {
        auto on_equip_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_EQUIP);
        for (const auto& effect : on_equip_effects) {
            apply_item_effect(player_id, effect);
        }
    }
}

void use_item(int index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    if (!inv) return;
    if (index < 0 || index >= static_cast<int>(inv->items.size())) return;
    
    EntityId item_id = inv->items[index];
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(item_id);
    
    if (!item) return;
    
    if (item->type == ItemComponent::Type::CONSUMABLE) {
        std::string name = item_name ? item_name->name : "Item";
        
        // Process item effects
        if (effects) {
            auto on_use_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_USE);
            for (const auto& effect : on_use_effects) {
                // Roll chance
                if (effect.chance < 1.0f) {
                    if (!chance(effect.chance)) continue;
                }
                
                apply_item_effect(player_id, effect);
            }
        } else {
            // Fallback for items without effects
            add_message("Used " + name + ".");
        }
        
        // Remove consumable
        inv->remove_item(item_id);
        manager->destroy_entity(item_id);
    } else {
        add_message("This item cannot be used directly. Try equipping it.");
    }
}

void drop_item(int index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    
    if (!inv || !player_pos) return;
    if (index < 0 || index >= static_cast<int>(inv->items.size())) return;
    
    EntityId item_id = inv->items[index];
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    RenderComponent* item_render = manager->get_component<RenderComponent>(item_id);
    
    // Confirm drop
    std::string name = item_name ? item_name->name : "Item";
    add_message("Dropped " + name + ".");
    
    // Remove from inventory
    inv->remove_item(item_id);
    
    // Place in world at player position
    if (!manager->get_component<PositionComponent>(item_id)) {
        manager->add_component(item_id, PositionComponent{player_pos->x, player_pos->y, 0});
    } else {
        PositionComponent* pos = manager->get_component<PositionComponent>(item_id);
        pos->x = player_pos->x;
        pos->y = player_pos->y;
    }
    
    // Restore render component if missing
    if (!item_render) {
        manager->add_component(item_id, RenderComponent{'?', "cyan", "", 3});
    }
}

void throw_item_from_inventory(int index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    
    if (!inv || !player_pos) return;
    if (index < 0 || index >= static_cast<int>(inv->items.size())) return;
    
    EntityId item_id = inv->items[index];
    int throw_range = get_throw_range(item_id);
    
    // Get targets in range (entities with stats that are alive)
    auto targets = combat_system->get_targets_in_range(player_id, throw_range,
        [](EntityId id, ComponentManager* mgr) {
            StatsComponent* stats = mgr->get_component<StatsComponent>(id);
            return stats && stats->is_alive();
        });
    
    if (targets.empty()) {
        add_message("No targets in range!");
        return;
    }
    
    int selected_target = 0;
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    std::string i_name = item_name ? item_name->name : "item";
    
    while (true) {
        render();
        
        int box_width = 45;
        int box_height = static_cast<int>(targets.size()) + 8;
        int box_x = (120 - box_width) / 2;
        int box_y = (35 - box_height) / 2;
        
        MenuWindow::draw_themed_box(console, box_x, box_y, box_width, box_height, "");
        
        console->draw_string(box_x + 2, box_y + 1, "Throw " + i_name, string_to_color("cyan"));
        console->draw_string(box_x + 2, box_y + 2, "Range: " + std::to_string(throw_range) + 
                           "  Damage: " + std::to_string(calculate_throw_damage(item_id)), Color::WHITE);
        
        int list_y = box_y + 4;
        for (size_t i = 0; i < targets.size(); ++i) {
            EntityId target_id = targets[i];
            NameComponent* name = manager->get_component<NameComponent>(target_id);
            StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
            PositionComponent* pos = manager->get_component<PositionComponent>(target_id);
            FactionComponent* faction = manager->get_component<FactionComponent>(target_id);
            
            std::string target_name = name ? name->name : "Target";
            std::string hp_str = stats ? " HP:" + std::to_string(stats->current_hp) + "/" + std::to_string(stats->max_hp) : "";
            
            // Calculate distance
            float dist = 0.0f;
            if (pos && player_pos) {
                int ddx = pos->x - player_pos->x;
                int ddy = pos->y - player_pos->y;
                dist = std::sqrt(static_cast<float>(ddx * ddx + ddy * ddy));
            }
            
            std::string dist_str = " [" + std::to_string(static_cast<int>(dist)) + "]";
            
            // Determine hostility for color coding
            bool is_hostile = false;
            bool is_friendly = false;
            if (faction) {
                FactionComponent* player_faction = manager->get_component<FactionComponent>(player_id);
                if (player_faction) {
                    FactionManager& fm = get_faction_manager();
                    FactionRelation relation = fm.get_relation(player_faction->faction, faction->faction);
                    is_hostile = (relation == FactionRelation::HOSTILE || relation == FactionRelation::UNFRIENDLY);
                    is_friendly = (relation == FactionRelation::FRIENDLY || relation == FactionRelation::ALLY);
                }
            }
            
            std::string display = target_name + hp_str + dist_str;
            
            Color text_color = Color::WHITE;
            if (is_hostile) text_color = Color::RED;
            else if (is_friendly) text_color = Color::GREEN;
            
            if (static_cast<int>(i) == selected_target) {
                console->draw_string(box_x + 2, list_y, "> " + display, Color::YELLOW);
            } else {
                console->draw_string(box_x + 2, list_y, "  " + display, text_color);
            }
            list_y++;
        }
        
        console->draw_string(box_x + 2, box_y + box_height - 2, "[W/S] Select  [Enter] Throw  [Esc] Cancel", Color::GRAY);
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (key == Key::ESCAPE) {
            return;
        }
        if (key == Key::W || key == Key::UP) {
            if (selected_target > 0) selected_target--;
        }
        if (key == Key::S || key == Key::DOWN) {
            if (selected_target < static_cast<int>(targets.size()) - 1) selected_target++;
        }
        if (key == Key::ENTER || key == Key::SPACE) {
            EntityId target_id = targets[selected_target];
            throw_item_at_target(item_id, target_id);
            process_turn();
            return;
        }
    }
}

void show_item_info(int index) {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    if (!inv) return;
    if (index < 0 || index >= static_cast<int>(inv->items.size())) return;
    
    EntityId item_id = inv->items[index];
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    
    if (!item || !item_name) return;
    
    console->clear();
    
    int panel_width = 60;
    int panel_height = 20;
    int panel_x = (console->get_width() - panel_width) / 2;
    int panel_y = (console->get_height() - panel_height) / 2;
    
    ui->draw_panel(panel_x, panel_y, panel_width, panel_height, "Item Details");
    
    int y = panel_y + 2;
    console->draw_string(panel_x + 2, y++, "Name: " + item_name->name, Color::YELLOW);
    console->draw_string(panel_x + 2, y++, "Description: " + item_name->description, Color::WHITE);
    y++;
    
    console->draw_string(panel_x + 2, y++, "Type: " + get_item_type_name(item->type), Color::CYAN);
    console->draw_string(panel_x + 2, y++, "Value: " + std::to_string(item->value) + " gold", Color::YELLOW);
    
    if (item->equip_slot != ItemComponent::EquipSlot::NONE) {
        y++;
        console->draw_string(panel_x + 2, y++, "Equipment Slot: " + get_equip_slot_name(item->equip_slot), Color::GREEN);
        console->draw_string(panel_x + 2, y++, "Required Limbs: " + std::to_string(item->required_limbs) + 
                           " " + get_limb_type_name(item->required_limb_type) + "(s)", Color::WHITE);
        y++;
        
        if (item->attack_bonus > 0) {
            console->draw_string(panel_x + 2, y++, "Attack Bonus: +" + std::to_string(item->attack_bonus), Color::RED);
        }
        if (item->defense_bonus > 0) {
            console->draw_string(panel_x + 2, y++, "Defense Bonus: +" + std::to_string(item->defense_bonus), Color::BLUE);
        }
        if (item->armor_bonus > 0) {
            console->draw_string(panel_x + 2, y++, "Armor Bonus: +" + std::to_string(item->armor_bonus), Color::BLUE);
        }
    }
    
    console->draw_string(panel_x + 2, panel_y + panel_height - 2, "Press any key to return...", Color::DARK_GRAY);
    console->present();
    Input::wait_for_key();
}

void apply_item_stats(EntityId item_id, bool apply) {
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    StatsComponent* stats = manager->get_component<StatsComponent>(player_id);
    
    if (!item || !stats) return;
    
    int multiplier = apply ? 1 : -1;
    stats->attack += item->attack_bonus * multiplier;
    stats->defense += item->defense_bonus * multiplier;
}

// ==================== Helper Name Methods ====================

std::string get_item_type_name(ItemComponent::Type type) {
    switch (type) {
        case ItemComponent::Type::WEAPON: return "Weapon";
        case ItemComponent::Type::ARMOR: return "Armor";
        case ItemComponent::Type::HELMET: return "Helmet";
        case ItemComponent::Type::BOOTS: return "Boots";
        case ItemComponent::Type::GLOVES: return "Gloves";
        case ItemComponent::Type::RING: return "Ring";
        case ItemComponent::Type::SHIELD: return "Shield";
        case ItemComponent::Type::CONSUMABLE: return "Consumable";
        case ItemComponent::Type::QUEST: return "Quest Item";
        case ItemComponent::Type::MISC: return "Miscellaneous";
        default: return "Unknown";
    }
}

std::string get_equip_slot_name(ItemComponent::EquipSlot slot) {
    switch (slot) {
        case ItemComponent::EquipSlot::HEAD: return "Head";
        case ItemComponent::EquipSlot::TORSO: return "Torso";
        case ItemComponent::EquipSlot::HAND: return "Hand";
        case ItemComponent::EquipSlot::TWO_HAND: return "Two Hands";
        case ItemComponent::EquipSlot::FOOT: return "Feet";
        case ItemComponent::EquipSlot::FINGER: return "Finger";
        case ItemComponent::EquipSlot::BACK: return "Back";
        case ItemComponent::EquipSlot::NECK: return "Neck";
        case ItemComponent::EquipSlot::WAIST: return "Waist";
        default: return "None";
    }
}

std::string get_limb_type_name(LimbType type) {
    switch (type) {
        case LimbType::HEAD: return "head";
        case LimbType::TORSO: return "torso";
        case LimbType::ARM: return "arm";
        case LimbType::HAND: return "hand";
        case LimbType::LEG: return "leg";
        case LimbType::FOOT: return "foot";
        case LimbType::OTHER: return "other";
        default: return "limb";
    }
}
