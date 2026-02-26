// gameplay_interaction.inl - Interaction system methods for BaseGameplayScene
// This file is included by base_gameplay_scene.hpp - do not include directly

// ==================== Interaction System Methods ====================

// Get interactable entities near the player (within 1 tile)
std::vector<EntityId> get_nearby_interactables() {
    std::vector<EntityId> nearby;
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    if (!player_pos) return nearby;
    
    // Check all entities with positions
    auto entities = manager->get_entities_with_component<PositionComponent>();
    
    for (EntityId id : entities) {
        if (id == player_id) continue;  // Skip player
        
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        if (!pos) continue;
        
        // Check if within 1 tile (including diagonals)
        int dx = std::abs(pos->x - player_pos->x);
        int dy = std::abs(pos->y - player_pos->y);
        
        if (dx <= 1 && dy <= 1) {
            // Include any entity that has a name (can be examined)
            // or has specific interactable components
            if (manager->get_component<NameComponent>(id) ||
                manager->get_component<NPCComponent>(id) ||
                manager->get_component<InteractableComponent>(id) ||
                manager->get_component<DoorComponent>(id) ||
                manager->get_component<ChestComponent>(id) ||
                manager->get_component<ItemComponent>(id) ||
                manager->get_component<StatsComponent>(id)) {
                nearby.push_back(id);
            }
        }
    }
    
    return nearby;
}

// Get faction name as string
std::string get_faction_name(FactionId faction) {
    switch (faction) {
        case FactionId::PLAYER: return "Player";
        case FactionId::VILLAGER: return "Villager";
        case FactionId::GUARD: return "Guard";
        case FactionId::WILDLIFE: return "Wildlife";
        case FactionId::PREDATOR: return "Predator";
        case FactionId::GOBLIN: return "Goblin";
        case FactionId::UNDEAD: return "Undead";
        case FactionId::BANDIT: return "Bandit";
        case FactionId::DEMON: return "Demon";
        case FactionId::DRAGON: return "Dragon";
        case FactionId::SPIDER: return "Spider";
        case FactionId::ORC: return "Orc";
        case FactionId::ELEMENTAL: return "Elemental";
        case FactionId::BEAST: return "Beast";
        default: return "Unknown";
    }
}

// Show detailed examine window for an entity
void show_examine_window(EntityId target_id) {
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "Unknown";
    
    // Gather all components for the entity
    FactionComponent* faction = manager->get_component<FactionComponent>(target_id);
    StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
    ItemComponent* item = manager->get_component<ItemComponent>(target_id);
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(target_id);
    AnatomyComponent* anatomy = manager->get_component<AnatomyComponent>(target_id);
    InventoryComponent* inv = manager->get_component<InventoryComponent>(target_id);
    DoorComponent* door = manager->get_component<DoorComponent>(target_id);
    ChestComponent* chest = manager->get_component<ChestComponent>(target_id);
    NPCComponent* npc = manager->get_component<NPCComponent>(target_id);
    PropertiesComponent* props = manager->get_component<PropertiesComponent>(target_id);
    
    // Window dimensions
    int panel_width = 70;
    int panel_height = 30;
    int panel_x = (console->get_width() - panel_width) / 2;
    int panel_y = (console->get_height() - panel_height) / 2;
    
    // Tabs for different sections
    enum class Tab { GENERAL, ANATOMY, INVENTORY, EQUIPMENT };
    Tab current_tab = Tab::GENERAL;
    int scroll_offset = 0;
    
    bool running = true;
    while (running) {
        console->clear();
        
        // Draw main panel
        ui->draw_panel(panel_x, panel_y, panel_width, panel_height, "Examining: " + target_name, Color::CYAN);
        
        int x = panel_x + 2;
        int y = panel_y + 2;
        
        // Draw tabs
        std::vector<std::pair<Tab, std::string>> tabs;
        tabs.push_back({Tab::GENERAL, "General"});
        if (anatomy && !anatomy->limbs.empty()) {
            tabs.push_back({Tab::ANATOMY, "Anatomy"});
        }
        if (inv && !inv->items.empty()) {
            tabs.push_back({Tab::INVENTORY, "Inventory"});
        }
        if (equip && !equip->equipped_items.empty()) {
            tabs.push_back({Tab::EQUIPMENT, "Equipment"});
        }
        
        int tab_x = x;
        for (const auto& tab : tabs) {
            bool is_selected = (tab.first == current_tab);
            Color tab_color = is_selected ? Color::YELLOW : Color::DARK_GRAY;
            std::string tab_text = is_selected ? "[" + tab.second + "]" : " " + tab.second + " ";
            console->draw_string(tab_x, y, tab_text, tab_color);
            tab_x += static_cast<int>(tab_text.length()) + 1;
        }
        y += 2;
        
        // Draw separator
        for (int i = 0; i < panel_width - 4; i++) {
            console->set_cell(x + i, y, '-', Color::DARK_GRAY);
        }
        y += 1;
        
        int content_start_y = y;
        int content_height = panel_height - 8;  // Leave room for footer
        
        // Draw content based on current tab
        if (current_tab == Tab::GENERAL) {
            // Description
            if (name && !name->description.empty()) {
                console->draw_string(x, y++, name->description, Color::WHITE);
                y++;
            }
            
            // Properties / Traits
            if (props && !props->properties.empty()) {
                console->draw_string(x, y++, "== Traits ==", Color::YELLOW);
                for (const auto& prop : props->properties) {
                    console->draw_string(x, y, "  * ", Color::DARK_GRAY);
                    console->draw_string(x + 4, y++, prop, Color::MAGENTA);
                }
                y++;
            }
            
            // Faction and relationship
            if (faction) {
                FactionManager& fm = get_faction_manager();
                // Use player's actual faction (for soul swap support)
                FactionComponent* player_faction = manager->get_component<FactionComponent>(player_id);
                FactionId player_faction_id = player_faction ? player_faction->faction : FactionId::PLAYER;
                FactionRelation rel = fm.get_relation(player_faction_id, faction->faction);
                std::string rel_str;
                Color rel_color = Color::WHITE;
                switch (rel) {
                    case FactionRelation::ALLY: rel_str = "Allied"; rel_color = Color::GREEN; break;
                    case FactionRelation::FRIENDLY: rel_str = "Friendly"; rel_color = Color::CYAN; break;
                    case FactionRelation::NEUTRAL: rel_str = "Neutral"; rel_color = Color::WHITE; break;
                    case FactionRelation::UNFRIENDLY: rel_str = "Unfriendly"; rel_color = Color::YELLOW; break;
                    case FactionRelation::HOSTILE: rel_str = "Hostile"; rel_color = Color::RED; break;
                    default: rel_str = "Unknown"; break;
                }
                console->draw_string(x, y, "Faction: ", Color::GRAY);
                console->draw_string(x + 9, y, get_faction_name(faction->faction), Color::WHITE);
                console->draw_string(x + 9 + static_cast<int>(get_faction_name(faction->faction).length()) + 1, y, "(" + rel_str + ")", rel_color);
                y++;
            }
            
            // NPC disposition
            if (npc) {
                std::string disp_str;
                Color disp_color = Color::WHITE;
                switch (npc->disposition) {
                    case NPCComponent::Disposition::FRIENDLY: disp_str = "Friendly"; disp_color = Color::GREEN; break;
                    case NPCComponent::Disposition::NEUTRAL: disp_str = "Neutral"; disp_color = Color::WHITE; break;
                    case NPCComponent::Disposition::HOSTILE: disp_str = "Hostile"; disp_color = Color::RED; break;
                    case NPCComponent::Disposition::FEARFUL: disp_str = "Fearful"; disp_color = Color::YELLOW; break;
                }
                console->draw_string(x, y, "Disposition: ", Color::GRAY);
                console->draw_string(x + 13, y++, disp_str, disp_color);
                
                if (npc->can_trade) {
                    console->draw_string(x, y++, "  * Can trade", Color::CYAN);
                }
            }
            y++;
            
            // Stats
            if (stats) {
                console->draw_string(x, y++, "== Stats ==", Color::YELLOW);
                console->draw_string(x, y, "Level: ", Color::GRAY);
                console->draw_string(x + 7, y++, std::to_string(stats->level), Color::WHITE);
                
                // Health bar
                console->draw_string(x, y, "HP: ", Color::GRAY);
                std::string hp_str = std::to_string(stats->current_hp) + "/" + std::to_string(stats->max_hp);
                float hp_pct = static_cast<float>(stats->current_hp) / stats->max_hp;
                Color hp_color = hp_pct > 0.5f ? Color::GREEN : (hp_pct > 0.25f ? Color::YELLOW : Color::RED);
                console->draw_string(x + 4, y++, hp_str, hp_color);
                
                console->draw_string(x, y, "Attack: ", Color::GRAY);
                console->draw_string(x + 8, y++, std::to_string(stats->attack), Color::WHITE);
                
                console->draw_string(x, y, "Defense: ", Color::GRAY);
                console->draw_string(x + 9, y++, std::to_string(stats->defense), Color::WHITE);
                y++;
            }
            
            // Item details
            if (item) {
                console->draw_string(x, y++, "== Item Info ==", Color::YELLOW);
                console->draw_string(x, y, "Type: ", Color::GRAY);
                console->draw_string(x + 6, y++, get_item_type_name(item->type), Color::WHITE);
                
                if (item->attack_bonus > 0) {
                    console->draw_string(x, y, "Attack Bonus: ", Color::GRAY);
                    console->draw_string(x + 14, y++, "+" + std::to_string(item->attack_bonus), Color::GREEN);
                }
                if (item->defense_bonus > 0) {
                    console->draw_string(x, y, "Defense Bonus: ", Color::GRAY);
                    console->draw_string(x + 15, y++, "+" + std::to_string(item->defense_bonus), Color::GREEN);
                }
                if (item->value > 0) {
                    console->draw_string(x, y, "Value: ", Color::GRAY);
                    console->draw_string(x + 7, y++, std::to_string(item->value) + " gold", Color::YELLOW);
                }
                y++;
            }
            
            // Door state
            if (door) {
                console->draw_string(x, y, "State: ", Color::GRAY);
                console->draw_string(x + 7, y++, door->is_open ? "Open" : "Closed", 
                                    door->is_open ? Color::GREEN : Color::RED);
            }
            
            // Chest contents
            if (chest) {
                console->draw_string(x, y++, "== Container ==", Color::YELLOW);
                if (chest->items.empty()) {
                    console->draw_string(x, y++, "Empty", Color::DARK_GRAY);
                } else {
                    console->draw_string(x, y, "Contains: ", Color::GRAY);
                    console->draw_string(x + 10, y++, std::to_string(chest->items.size()) + " item(s)", Color::WHITE);
                }
            }
        }
        else if (current_tab == Tab::ANATOMY && anatomy) {
            console->draw_string(x, y++, "== Body Parts ==", Color::YELLOW);
            y++;
            
            int limb_idx = 0;
            for (const auto& limb : anatomy->limbs) {
                if (limb_idx >= scroll_offset && y < content_start_y + content_height) {
                    Color limb_color = limb.functional ? Color::WHITE : Color::RED;
                    std::string status = limb.functional ? "" : " [DAMAGED]";
                    
                    console->draw_string(x, y, limb.name + status, limb_color);
                    
                    // Show what's equipped on this limb
                    if (equip) {
                        EntityId equipped_id = equip->get_equipped(limb.name);
                        if (equipped_id != 0) {
                            NameComponent* eq_name = manager->get_component<NameComponent>(equipped_id);
                            if (eq_name) {
                                console->draw_string(x + 20, y, "-> " + eq_name->name, Color::CYAN);
                            }
                        }
                    }
                    y++;
                }
                limb_idx++;
            }
        }
        else if (current_tab == Tab::INVENTORY && inv) {
            console->draw_string(x, y, "== Inventory ==", Color::YELLOW);
            console->draw_string(x + 20, y++, "(" + std::to_string(inv->items.size()) + "/" + 
                                std::to_string(inv->max_items) + ")", Color::GRAY);
            y++;
            
            if (inv->items.empty()) {
                console->draw_string(x, y++, "Empty", Color::DARK_GRAY);
            } else {
                int item_idx = 0;
                for (EntityId item_id : inv->items) {
                    if (item_idx >= scroll_offset && y < content_start_y + content_height) {
                        NameComponent* item_name = manager->get_component<NameComponent>(item_id);
                        ItemComponent* item_comp = manager->get_component<ItemComponent>(item_id);
                        
                        std::string display = item_name ? item_name->name : "Unknown Item";
                        if (item_comp) {
                            display += " (" + get_item_type_name(item_comp->type) + ")";
                        }
                        console->draw_string(x, y++, "- " + display, Color::WHITE);
                    }
                    item_idx++;
                }
            }
        }
        else if (current_tab == Tab::EQUIPMENT && equip) {
            console->draw_string(x, y++, "== Equipment ==", Color::YELLOW);
            y++;
            
            if (equip->equipped_items.empty() && equip->back == 0 && equip->neck == 0 && equip->waist == 0) {
                console->draw_string(x, y++, "Nothing equipped", Color::DARK_GRAY);
            } else {
                int eq_idx = 0;
                for (const auto& pair : equip->equipped_items) {
                    if (eq_idx >= scroll_offset && y < content_start_y + content_height) {
                        NameComponent* eq_name = manager->get_component<NameComponent>(pair.second);
                        std::string item_str = eq_name ? eq_name->name : "Unknown";
                        console->draw_string(x, y, pair.first + ": ", Color::GRAY);
                        console->draw_string(x + static_cast<int>(pair.first.length()) + 2, y++, item_str, Color::WHITE);
                    }
                    eq_idx++;
                }
                
                // Special slots
                if (equip->back != 0 && y < content_start_y + content_height) {
                    NameComponent* eq_name = manager->get_component<NameComponent>(equip->back);
                    console->draw_string(x, y, "Back: ", Color::GRAY);
                    console->draw_string(x + 6, y++, eq_name ? eq_name->name : "Unknown", Color::WHITE);
                }
                if (equip->neck != 0 && y < content_start_y + content_height) {
                    NameComponent* eq_name = manager->get_component<NameComponent>(equip->neck);
                    console->draw_string(x, y, "Neck: ", Color::GRAY);
                    console->draw_string(x + 6, y++, eq_name ? eq_name->name : "Unknown", Color::WHITE);
                }
                if (equip->waist != 0 && y < content_start_y + content_height) {
                    NameComponent* eq_name = manager->get_component<NameComponent>(equip->waist);
                    console->draw_string(x, y, "Waist: ", Color::GRAY);
                    console->draw_string(x + 7, y++, eq_name ? eq_name->name : "Unknown", Color::WHITE);
                }
            }
        }
        
        // Draw footer with controls
        int footer_y = panel_y + panel_height - 2;
        console->draw_string(x, footer_y, "LEFT/RIGHT: Switch Tab | UP/DOWN: Scroll | ESC: Close", Color::DARK_GRAY);
        
        console->present();
        
        // Handle input
        Key key = Input::wait_for_key();
        auto& bindings = KeyBindings::instance();
        
        if (bindings.is_action(key, GameAction::CANCEL) || key == Key::X) {
            running = false;
        }
        else if (bindings.is_action(key, GameAction::MOVE_LEFT) || key == Key::LEFT) {
            // Switch to previous tab
            for (size_t i = 0; i < tabs.size(); i++) {
                if (tabs[i].first == current_tab && i > 0) {
                    current_tab = tabs[i - 1].first;
                    scroll_offset = 0;
                    break;
                }
            }
        }
        else if (bindings.is_action(key, GameAction::MOVE_RIGHT) || key == Key::RIGHT) {
            // Switch to next tab
            for (size_t i = 0; i < tabs.size(); i++) {
                if (tabs[i].first == current_tab && i < tabs.size() - 1) {
                    current_tab = tabs[i + 1].first;
                    scroll_offset = 0;
                    break;
                }
            }
        }
        else if (bindings.is_action(key, GameAction::MENU_UP) || key == Key::UP) {
            if (scroll_offset > 0) scroll_offset--;
        }
        else if (bindings.is_action(key, GameAction::MENU_DOWN) || key == Key::DOWN) {
            scroll_offset++;
        }
    }
}

// Examine a specific entity - shows detailed information (legacy message-based version)
void examine_entity(EntityId target_id) {
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "something";
    
    // Show name and description
    if (name && !name->description.empty()) {
        add_message(name->description);
    } else {
        add_message("You examine " + target_name + ".");
    }
    
    // Show properties / traits
    PropertiesComponent* props = manager->get_component<PropertiesComponent>(target_id);
    if (props && !props->properties.empty()) {
        for (const auto& prop : props->properties) {
            add_message("  * " + prop);
        }
    }
    
    // Show faction and relationship
    FactionComponent* faction = manager->get_component<FactionComponent>(target_id);
    if (faction) {
        FactionManager& fm = get_faction_manager();
        // Use player's actual faction (for soul swap support)
        FactionComponent* player_faction_comp = manager->get_component<FactionComponent>(player_id);
        FactionId player_faction_id = player_faction_comp ? player_faction_comp->faction : FactionId::PLAYER;
        FactionRelation rel = fm.get_relation(player_faction_id, faction->faction);
        std::string rel_str;
        switch (rel) {
            case FactionRelation::ALLY: rel_str = "Allied"; break;
            case FactionRelation::FRIENDLY: rel_str = "Friendly"; break;
            case FactionRelation::NEUTRAL: rel_str = "Neutral"; break;
            case FactionRelation::UNFRIENDLY: rel_str = "Unfriendly"; break;
            case FactionRelation::HOSTILE: rel_str = "Hostile"; break;
            default: rel_str = "Unknown"; break;
        }
        add_message("Faction: " + get_faction_name(faction->faction) + " (" + rel_str + ")");
    }
    
    // Show stats if available (creature/NPC)
    StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
    if (stats) {
        add_message("Level " + std::to_string(stats->level) + 
                   " | HP: " + std::to_string(stats->current_hp) + "/" + std::to_string(stats->max_hp) +
                   " | ATK: " + std::to_string(stats->attack) + " DEF: " + std::to_string(stats->defense));
    }
    
    // Show item details if it's an item
    ItemComponent* item = manager->get_component<ItemComponent>(target_id);
    if (item) {
        std::string info = get_item_type_name(item->type);
        if (item->attack_bonus > 0) info += " | ATK +" + std::to_string(item->attack_bonus);
        if (item->defense_bonus > 0) info += " | DEF +" + std::to_string(item->defense_bonus);
        if (item->value > 0) info += " | Value: " + std::to_string(item->value) + "g";
        add_message(info);
    }
    
    // Show equipment if creature has any
    EquipmentComponent* equip = manager->get_component<EquipmentComponent>(target_id);
    if (equip && !equip->equipped_items.empty()) {
        std::string equipped_list = "Equipped: ";
        bool first = true;
        for (const auto& pair : equip->equipped_items) {
            NameComponent* eq_name = manager->get_component<NameComponent>(pair.second);
            if (eq_name) {
                if (!first) equipped_list += ", ";
                equipped_list += eq_name->name;
                first = false;
            }
        }
        if (!first) add_message(equipped_list);
    }
    
    // Show door state
    DoorComponent* door = manager->get_component<DoorComponent>(target_id);
    if (door) {
        add_message(door->is_open ? "The door is open." : "The door is closed.");
    }
    
    // Show chest contents hint
    ChestComponent* chest = manager->get_component<ChestComponent>(target_id);
    if (chest) {
        if (chest->items.empty()) {
            add_message("The chest appears to be empty.");
        } else {
            add_message("The chest contains " + std::to_string(chest->items.size()) + " item(s).");
        }
    }
}

// Examine nearby entities (triggered by X key)
void examine_nearby() {
    std::vector<EntityId> nearby = get_nearby_interactables();
    
    if (nearby.empty()) {
        add_message("Nothing nearby to examine.");
        return;
    }
    
    // If multiple entities, let player choose which one
    EntityId target_id = 0;
    if (nearby.size() > 1) {
        target_id = ui->show_target_selection(nearby, [this]() {
            this->render();
        });
    } else {
        target_id = nearby[0];
    }
    
    if (target_id == 0) {
        return;  // Cancelled
    }
    
    // Show detailed examine window for the entity
    show_examine_window(target_id);
}

// Get relation symbol for display
std::string get_relation_symbol(FactionRelation rel) {
    switch (rel) {
        case FactionRelation::ALLY: return "++";
        case FactionRelation::FRIENDLY: return "+ ";
        case FactionRelation::NEUTRAL: return "  ";
        case FactionRelation::UNFRIENDLY: return "- ";
        case FactionRelation::HOSTILE: return "--";
        default: return "? ";
    }
}

// Get relation color for display
Color get_relation_color(FactionRelation rel) {
    switch (rel) {
        case FactionRelation::ALLY: return Color::GREEN;
        case FactionRelation::FRIENDLY: return Color::CYAN;
        case FactionRelation::NEUTRAL: return Color::WHITE;
        case FactionRelation::UNFRIENDLY: return Color::YELLOW;
        case FactionRelation::HOSTILE: return Color::RED;
        default: return Color::WHITE;
    }
}

// Show faction relationships screen (TAB key)
void show_faction_relations() {
    FactionManager& fm = get_faction_manager();
    
    // List of factions to show (excluding NONE and FACTION_COUNT)
    std::vector<std::pair<FactionId, std::string>> factions = {
        {FactionId::PLAYER, "Player"},
        {FactionId::VILLAGER, "Villager"},
        {FactionId::GUARD, "Guard"},
        {FactionId::WILDLIFE, "Wildlife"},
        {FactionId::PREDATOR, "Predator"},
        {FactionId::GOBLIN, "Goblin"},
        {FactionId::UNDEAD, "Undead"},
        {FactionId::BANDIT, "Bandit"},
        {FactionId::DEMON, "Demon"},
        {FactionId::DRAGON, "Dragon"},
        {FactionId::SPIDER, "Spider"},
        {FactionId::ORC, "Orc"},
        {FactionId::ELEMENTAL, "Elemental"},
        {FactionId::BEAST, "Beast"}
    };
    
    int scroll_offset = 0;
    int selected_faction = 0;
    bool running = true;
    
    while (running) {
        console->clear();
        
        int width = console->get_width();
        int height = console->get_height();
        
        // Draw title
        ui->draw_panel(2, 1, width - 4, height - 2, "Faction Relations", Color::CYAN);
        
        // Legend
        console->draw_string(4, 3, "Legend: ", Color::WHITE);
        console->draw_string(12, 3, "++ Ally  ", Color::GREEN);
        console->draw_string(21, 3, "+ Friendly  ", Color::CYAN);
        console->draw_string(33, 3, "  Neutral  ", Color::WHITE);
        console->draw_string(45, 3, "- Unfriendly  ", Color::YELLOW);
        console->draw_string(59, 3, "-- Hostile", Color::RED);
        
        // Instructions
        console->draw_string(4, height - 3, "UP/DOWN: Select faction | TAB/ESC: Close", Color::GRAY);
        
        // Show selected faction's relationships
        int y = 5;
        console->draw_string(4, y, "Select a faction to see its relationships:", Color::YELLOW);
        y += 2;
        
        // List all factions
        int visible_factions = std::min(static_cast<int>(factions.size()), height - 12);
        
        for (int i = 0; i < visible_factions && (i + scroll_offset) < static_cast<int>(factions.size()); i++) {
            int idx = i + scroll_offset;
            bool is_selected = (idx == selected_faction);
            
            if (is_selected) {
                console->draw_string(4, y + i, "> " + factions[idx].second, Color::YELLOW);
            } else {
                console->draw_string(4, y + i, "  " + factions[idx].second, Color::WHITE);
            }
        }
        
        // Show relationships for selected faction
        int rel_x = 25;
        int rel_y = 7;
        FactionId selected_id = factions[selected_faction].first;
        
        console->draw_string(rel_x, rel_y - 2, factions[selected_faction].second + " relations:", Color::CYAN);
        
        for (size_t i = 0; i < factions.size(); i++) {
            if (factions[i].first == selected_id) continue;  // Skip self
            
            FactionRelation rel = fm.get_relation(selected_id, factions[i].first);
            std::string symbol = get_relation_symbol(rel);
            Color color = get_relation_color(rel);
            
            console->draw_string(rel_x, rel_y, symbol + " " + factions[i].second, color);
            rel_y++;
            
            if (rel_y >= height - 4) {
                rel_x += 20;
                rel_y = 7;
            }
        }
        
        console->present();
        
        // Handle input
        Key key = Input::wait_for_key();
        auto& bindings = KeyBindings::instance();
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            if (selected_faction > 0) {
                selected_faction--;
                if (selected_faction < scroll_offset) {
                    scroll_offset = selected_faction;
                }
            }
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            if (selected_faction < static_cast<int>(factions.size()) - 1) {
                selected_faction++;
                if (selected_faction >= scroll_offset + visible_factions) {
                    scroll_offset = selected_faction - visible_factions + 1;
                }
            }
        } else if (key == Key::TAB || bindings.is_action(key, GameAction::CANCEL)) {
            running = false;
        }
    }
}

// Open the interaction window
void open_interaction_menu() {
    std::vector<EntityId> nearby = get_nearby_interactables();
    
    if (nearby.empty()) {
        add_message("Nothing nearby to interact with.");
        return;
    }
    
    // If multiple entities, let player choose which one
    EntityId target_id = 0;
    if (nearby.size() > 1) {
        target_id = ui->show_target_selection(nearby, [this]() {
            this->render();
        });
    } else {
        target_id = nearby[0];
    }
    
    if (target_id == 0) {
        return;  // Cancelled
    }
    
    // Show interaction options for this entity
    InteractionResult result = ui->show_interaction_window(target_id, [this]() {
        this->render();
    });
    
    if (result.cancelled) {
        return;
    }
    
    // Handle the interaction
    handle_interaction(result);
}

// Handle an interaction result
void handle_interaction(const InteractionResult& result) {
    EntityId target_id = result.target_id;
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "target";
    
    switch (result.type) {
        case InteractionType::TALK: {
            // Start dialogue with NPC
            DialogueComponent* dialogue = manager->get_component<DialogueComponent>(target_id);
            if (dialogue) {
                dialogue_system->start_dialogue(target_id);
            } else {
                add_message(target_name + " has nothing to say.");
            }
            break;
        }
        
        case InteractionType::TRADE: {
            // Open trade interface
            NPCComponent* npc = manager->get_component<NPCComponent>(target_id);
            if (npc && npc->can_trade) {
                open_trade_interface(target_id);
            } else {
                add_message(target_name + " doesn't want to trade.");
            }
            break;
        }
        
        case InteractionType::STEAL: {
            // Attempt to steal from NPC
            attempt_steal(target_id);
            break;
        }
        
        case InteractionType::ATTACK: {
            // Attack the NPC
            NPCComponent* npc = manager->get_component<NPCComponent>(target_id);
            StatsComponent* target_stats = manager->get_component<StatsComponent>(target_id);
            
            if (npc) {
                npc->disposition = NPCComponent::Disposition::HOSTILE;
                add_message("You attack " + target_name + "!");
                
                // Add player as a personal enemy so AI will pursue
                FactionComponent* target_faction = manager->get_component<FactionComponent>(target_id);
                if (target_faction) {
                    target_faction->add_enemy(player_id);
                    target_faction->last_attacker = player_id;
                }
            }
            
            if (target_stats) {
                attack_enemy(target_id);
            } else {
                add_message(target_name + " cannot be attacked.");
            }
            break;
        }
        
        case InteractionType::EXAMINE: {
            // Examine the entity - show faction, level, HP
            examine_entity(target_id);
            break;
        }
        
        case InteractionType::OPEN: {
            // Open chest or door
            ChestComponent* chest = manager->get_component<ChestComponent>(target_id);
            DoorComponent* door = manager->get_component<DoorComponent>(target_id);
            
            if (chest) {
                if (!chest->is_open) {
                    chest->toggle();
                    RenderComponent* render = manager->get_component<RenderComponent>(target_id);
                    if (render) {
                        render->ch = chest->open_char;
                        render->color = chest->open_color;
                    }
                }
                show_chest_interface(target_id);
            } else if (door) {
                interaction_system->toggle_door(target_id, door, world.get());
            }
            break;
        }
        
        case InteractionType::PICKUP: {
            // Pick up item
            ItemComponent* item = manager->get_component<ItemComponent>(target_id);
            InventoryComponent* player_inv = manager->get_component<InventoryComponent>(player_id);
            
            if (item && player_inv) {
                if (player_inv->add_item(target_id)) {
                    manager->remove_component<PositionComponent>(target_id);
                    manager->remove_component<RenderComponent>(target_id);
                    add_message("Picked up " + target_name + ".");
                } else {
                    add_message("Inventory full!");
                }
            }
            break;
        }
        
        case InteractionType::USE: {
            // Use interactable
            DoorComponent* door = manager->get_component<DoorComponent>(target_id);
            if (door) {
                interaction_system->toggle_door(target_id, door, world.get());
            }
            break;
        }
        
        case InteractionType::NONE:
            // No interaction selected (cancelled or no options)
            break;
        
        default:
            add_message("Interacted with " + target_name + ".");
            break;
    }
}

// Attempt to steal from an NPC
void attempt_steal(EntityId target_id) {
    NameComponent* name = manager->get_component<NameComponent>(target_id);
    std::string target_name = name ? name->name : "target";
    
    NPCComponent* npc = manager->get_component<NPCComponent>(target_id);
    InventoryComponent* target_inv = manager->get_component<InventoryComponent>(target_id);
    InventoryComponent* player_inv = manager->get_component<InventoryComponent>(player_id);
    
    if (!target_inv || !player_inv) {
        add_message(target_name + " has nothing to steal.");
        return;
    }
    
    // Calculate success chance based on NPC role and player luck
    int base_chance = 50;  // 50% base chance
    
    if (npc) {
        switch (npc->role) {
            case NPCComponent::Role::GUARD:
                base_chance = 20;  // Guards are vigilant
                break;
            case NPCComponent::Role::MERCHANT:
                base_chance = 30;  // Merchants are careful
                break;
            case NPCComponent::Role::VILLAGER:
                base_chance = 60;  // Villagers are easy
                break;
            default:
                break;
        }
    }
    
    bool success = random_int(0, 99) < base_chance;
    
    if (success) {
        // Steal some gold
        int steal_amount = random_int(1, 10);
        if (steal_amount > target_inv->gold) {
            steal_amount = target_inv->gold;
        }
        
        if (steal_amount > 0) {
            target_inv->gold -= steal_amount;
            player_inv->gold += steal_amount;
            add_message("You stole " + std::to_string(steal_amount) + " gold from " + target_name + "!");
        } else {
            add_message(target_name + " has no gold to steal.");
        }
    } else {
        add_message("You failed to steal from " + target_name + "!");
        
        // Make NPC hostile
        if (npc) {
            npc->disposition = NPCComponent::Disposition::HOSTILE;
            add_message(target_name + " is now hostile!");
            
            // Add player as a personal enemy so AI will pursue
            FactionComponent* target_faction = manager->get_component<FactionComponent>(target_id);
            if (target_faction) {
                target_faction->add_enemy(player_id);
                target_faction->last_attacker = player_id;
            }
            
            // Guards might attack
            if (npc->role == NPCComponent::Role::GUARD) {
                add_message(target_name + " attacks you!");
                StatsComponent* guard_stats = manager->get_component<StatsComponent>(target_id);
                if (guard_stats) {
                    perform_attack(target_id, player_id);
                }
            }
        }
    }
}

// Open trade interface with a merchant
void open_trade_interface(EntityId merchant_id) {
    NameComponent* name = manager->get_component<NameComponent>(merchant_id);
    std::string merchant_name = name ? name->name : "Merchant";
    
    ShopInventoryComponent* shop = manager->get_component<ShopInventoryComponent>(merchant_id);
    if (!shop) {
        add_message(merchant_name + " has nothing to sell.");
        return;
    }
    
    // Create item callback to convert ShopItem to actual entity
    auto create_item = [this](const ShopItem& shop_item, int x, int y) -> EntityId {
        (void)x; (void)y;  // Unused, items go to inventory
        
        // If template_id is set, create from template instead
        if (!shop_item.template_id.empty() && factory) {
            Entity entity = factory->get_entity_loader().create_entity_from_template(
                manager, shop_item.template_id, -1, -1);
            if (entity.get_id() != 0) {
                // Override price from shop item
                ItemComponent* item_comp = entity.get_component<ItemComponent>();
                if (item_comp) {
                    item_comp->value = shop_item.buy_price;
                }
                return entity.get_id();
            }
            // Fall through to manual creation if template not found
        }
        
        EntityId id = manager->create_entity();
        Entity entity(id, manager);
        
        // NOTE: Items going directly to inventory should NOT have a PositionComponent
        // Only add position when item is dropped into the world
        entity.add_component(RenderComponent{shop_item.symbol, shop_item.color, "", 3});
        entity.add_component(NameComponent{shop_item.name, shop_item.description});
        
        ItemComponent item;
        item.type = shop_item.item_type;
        item.equip_slot = shop_item.equip_slot;
        item.value = shop_item.buy_price;
        item.attack_bonus = shop_item.attack_bonus;
        item.defense_bonus = shop_item.defense_bonus;
        item.armor_bonus = shop_item.armor_bonus;
        entity.add_component(item);
        
        // Add effects for consumables (supports any effect type via lambda or standard effects)
        if (shop_item.is_consumable && !shop_item.effects.empty()) {
            for (const auto& effect_component : shop_item.effects) {
                entity.add_component(effect_component);
            }
        }
        
        // Add ranged weapon component if this is a ranged weapon
        if (shop_item.is_ranged) {
            RangedWeaponComponent ranged;
            ranged.range = shop_item.range;
            ranged.min_range = shop_item.min_range;
            ranged.ammo_type = shop_item.ammo_type;
            ranged.consumes_self = shop_item.consumes_self;
            ranged.accuracy_bonus = shop_item.accuracy_bonus;
            entity.add_component(ranged);
        }
        
        // Add ammo component if this is ammo
        if (shop_item.is_ammo) {
            AmmoComponent ammo;
            ammo.ammo_type = shop_item.ammo_type;
            ammo.quantity = shop_item.ammo_quantity;
            ammo.damage_bonus = shop_item.ammo_damage_bonus;
            entity.add_component(ammo);
        }
        
        return id;
    };
    
    // Render background callback
    auto render_bg = [this]() {
        render();
    };
    
    // Loop trade interface - allow multiple trades
    bool trading = true;
    while (trading) {
        auto result = ui->show_trade_interface(player_id, merchant_id, render_bg, create_item);
        
        switch (result.action) {
            case UI::TradeResult::Action::BOUGHT:
                add_message("You purchased an item for " + std::to_string(-result.gold_change) + " gold.");
                break;
                
            case UI::TradeResult::Action::SOLD:
                add_message("You sold an item for " + std::to_string(result.gold_change) + " gold.");
                break;
                
            case UI::TradeResult::Action::CANCELLED:
                trading = false;
                add_message("Finished trading with " + merchant_name + ".");
                break;
        }
    }
}

// Common item pickup logic
void pickup_items() {
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    InventoryComponent* player_inv = manager->get_component<InventoryComponent>(player_id);
    
    if (!player_pos || !player_inv) return;
    
    // Find items at player position
    auto entities = manager->get_entities_with_component<ItemComponent>();
    std::vector<EntityId> items_here;
    
    for (EntityId id : entities) {
        PositionComponent* pos = manager->get_component<PositionComponent>(id);
        if (pos && pos->x == player_pos->x && pos->y == player_pos->y) {
            items_here.push_back(id);
        }
    }
    
    if (items_here.empty()) {
        add_message("Nothing to pick up here.");
        return;
    }
    
    // Pick up all items
    for (EntityId item_id : items_here) {
        NameComponent* item_name = manager->get_component<NameComponent>(item_id);
        std::string name = item_name ? item_name->name : "Item";
        
        if (player_inv->add_item(item_id)) {
            // Remove from world (no longer render it)
            manager->remove_component<PositionComponent>(item_id);
            manager->remove_component<RenderComponent>(item_id);
            add_message("Picked up " + name + ".");
        } else {
            add_message("Inventory full! Can't pick up " + name + ".");
            break;
        }
    }
}

// Common UI helpers
void show_chest_interface(EntityId chest_entity_id) {
    ChestComponent* chest = manager->get_component<ChestComponent>(chest_entity_id);
    InventoryComponent* player_inv = manager->get_component<InventoryComponent>(player_id);
    if (!chest || !player_inv) return;
    
    int selected_index = 0;
    bool in_chest = true;  // true = chest list, false = player inventory
    bool running = true;
    
    while (running) {
        console->clear();
        
        // Split screen: chest on left, player inventory on right
        int mid_x = console->get_width() / 2;
        
        // Draw chest contents
        draw_chest_contents(5, 5, mid_x - 10, 20, chest_entity_id, in_chest ? selected_index : -1);
        // Draw player inventory
        draw_player_inventory_for_chest(mid_x + 5, 5, mid_x - 10, 20, player_id, !in_chest ? selected_index : -1);
        
        // Draw controls
        console->draw_string(5, console->get_height() - 4, "Controls:", Color::YELLOW);
        console->draw_string(5, console->get_height() - 3, "UP/DOWN: Select | LEFT/RIGHT: Switch | SPACE: Take/Store | ESC: Close", Color::WHITE);
        
        console->present();
        
        // Handle input
        Key key = Input::get_key();
        auto& bindings = KeyBindings::instance();
        
        int max_index = in_chest ? static_cast<int>(chest->items.size()) - 1 : static_cast<int>(player_inv->items.size()) - 1;
        
        if (bindings.is_action(key, GameAction::MENU_UP)) {
            if (selected_index > 0) selected_index--;
        } else if (bindings.is_action(key, GameAction::MENU_DOWN)) {
            if (selected_index < max_index) selected_index++;
        } else if (bindings.is_action(key, GameAction::MENU_LEFT)) {
            if (!in_chest) {
                in_chest = true;
                selected_index = 0;
            }
        } else if (bindings.is_action(key, GameAction::MENU_RIGHT)) {
            if (in_chest) {
                in_chest = false;
                selected_index = 0;
            }
        } else if (bindings.is_action(key, GameAction::CONFIRM)) {
            if (in_chest) {
                // Take from chest
                if (selected_index >= 0 && selected_index < static_cast<int>(chest->items.size())) {
                    EntityId item_id = chest->items[selected_index];
                    if (player_inv->add_item(item_id)) {
                        chest->remove_item(item_id);
                        if (selected_index >= static_cast<int>(chest->items.size()) && selected_index > 0) {
                            selected_index--;
                        }
                    }
                }
            } else {
                // Store in chest
                if (selected_index >= 0 && selected_index < static_cast<int>(player_inv->items.size())) {
                    EntityId item_id = player_inv->items[selected_index];
                    chest->add_item(item_id);
                    player_inv->items.erase(player_inv->items.begin() + selected_index);
                    if (selected_index >= static_cast<int>(player_inv->items.size()) && selected_index > 0) {
                        selected_index--;
                    }
                }
            }
        } else if (bindings.is_action(key, GameAction::CANCEL)) {
            running = false;
        }
    }
}
