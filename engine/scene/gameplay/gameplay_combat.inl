// gameplay_combat.inl - Combat-related methods for BaseGameplayScene
// This file is included by base_gameplay_scene.hpp - do not include directly

// ==================== Effect Message Formatting ====================

// Helper function to replace "%target%" placeholder with target name in effect messages
std::string format_effect_message(const std::string& message, const std::string& target_name, bool is_player) {
    std::string result = message;
    std::string display_name = is_player ? "You" : target_name;
    
    // Replace all instances of %target% with the display name
    size_t pos = 0;
    while ((pos = result.find("%target%", pos)) != std::string::npos) {
        result.replace(pos, 8, display_name);
        pos += display_name.length();
    }
    
    return result;
}

// ==================== Ranged Combat ======================================

// Check if player has a ranged weapon equipped
bool has_ranged_weapon() {
    return combat_system->has_ranged_weapon(player_id);
}

// Get the best ranged weapon for displaying info
EntityId get_best_ranged_weapon() {
    return combat_system->get_best_ranged_weapon(player_id);
}

// Perform a ranged attack against a target
void ranged_attack_enemy(EntityId enemy_id) {
    EntityId weapon_id = combat_system->get_best_ranged_weapon(player_id);
    if (weapon_id == 0) {
        add_message("No ranged weapon equipped!");
        return;
    }
    combat_system->perform_ranged_attack(player_id, enemy_id, weapon_id, true);
}

// Show ranged targeting mode - returns true if an attack was made
bool show_ranged_targeting() {
    if (!has_ranged_weapon()) {
        add_message("No ranged weapon equipped!");
        return false;
    }
    
    EntityId weapon_id = get_best_ranged_weapon();
    int range = combat_system->get_weapon_range(weapon_id);
    
    // Get all targets in range (entities with stats that are alive)
    auto targets = combat_system->get_targets_in_range(player_id, range, 
        [](EntityId id, ComponentManager* mgr) {
            StatsComponent* stats = mgr->get_component<StatsComponent>(id);
            return stats && stats->is_alive();
        });
    
    if (targets.empty()) {
        add_message("No targets in range!");
        return false;
    }
    
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    if (!player_pos) return false;
    
    // Get weapon and ammo info for display
    NameComponent* weapon_name = manager->get_component<NameComponent>(weapon_id);
    RangedWeaponComponent* ranged = manager->get_component<RangedWeaponComponent>(weapon_id);
    std::string w_name = weapon_name ? weapon_name->name : "Ranged Weapon";
    
    // Check ammo
    std::string ammo_info = "";
    if (ranged && ranged->requires_ammo()) {
        EntityId ammo_id = combat_system->find_ammo_for_weapon(player_id, weapon_id);
        if (ammo_id == 0) {
            add_message("No ammo for your " + w_name + "!");
            return false;
        }
        AmmoComponent* ammo = manager->get_component<AmmoComponent>(ammo_id);
        if (ammo) {
            ammo_info = " [" + std::to_string(ammo->quantity) + " ammo]";
        }
    } else if (ranged && ranged->consumes_self) {
        ItemComponent* item = manager->get_component<ItemComponent>(weapon_id);
        if (item && item->stackable) {
            ammo_info = " [" + std::to_string(item->stack_count) + " remaining]";
        }
    }
    
    int selected_idx = 0;
    
    while (true) {
        // Render background
        render();
        
        // Draw targeting overlay
        int box_width = 45;
        int box_height = static_cast<int>(targets.size()) + 8;
        int box_x = (120 - box_width) / 2;
        int box_y = (35 - box_height) / 2;
        
        // Draw box
        MenuWindow::draw_themed_box(console, box_x, box_y, box_width, box_height, "");
        
        // Title
        console->draw_string(box_x + 2, box_y + 1, "Ranged Attack - " + w_name, string_to_color("cyan"));
        console->draw_string(box_x + 2, box_y + 2, "Range: " + std::to_string(range) + ammo_info, Color::WHITE);
        
        // List targets
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
            
            // Color code: yellow for selected, red for hostile, green for friendly, white for neutral
            Color text_color = Color::WHITE;
            if (is_hostile) text_color = Color::RED;
            else if (is_friendly) text_color = Color::GREEN;
            
            if (static_cast<int>(i) == selected_idx) {
                console->draw_string(box_x + 2, list_y, "> " + display, Color::YELLOW);
            } else {
                console->draw_string(box_x + 2, list_y, "  " + display, text_color);
            }
            list_y++;
        }
        
        // Instructions
        console->draw_string(box_x + 2, box_y + box_height - 2, "[W/S] Select  [Enter] Fire  [Esc] Cancel", Color::GRAY);
        
        console->present();
        
        // Handle input
        Key key = Input::wait_for_key();
        
        if (key == Key::ESCAPE) {
            return false;
        }
        if (key == Key::W || key == Key::UP) {
            if (selected_idx > 0) selected_idx--;
        }
        if (key == Key::S || key == Key::DOWN) {
            if (selected_idx < static_cast<int>(targets.size()) - 1) selected_idx++;
        }
        if (key == Key::ENTER || key == Key::SPACE) {
            EntityId target_id = targets[selected_idx];
            ranged_attack_enemy(target_id);
            return true;
        }
    }
}

// ==================== Throw Item System ====================

// Apply a thrown item's effect to a target (similar to apply_item_effect but for thrown items)
// thrower_id is the entity that threw the item
void apply_thrown_item_effect(EntityId target_id, const ItemEffectComponent::Effect& effect, const std::string& target_name, EntityId thrower_id) {
    StatsComponent* stats = manager->get_component<StatsComponent>(target_id);
    bool is_player = (target_id == player_id);
    
    switch (effect.type) {
        case ItemEffectComponent::EffectType::HEAL_HP:
            if (stats) {
                int healed = stats->heal(effect.magnitude);
                std::string msg = effect.message.empty() ? 
                    target_name + " is healed for " + std::to_string(healed) + " HP!" : 
                    format_effect_message(effect.message, target_name, is_player);
                add_message(msg);
            }
            break;
            
        case ItemEffectComponent::EffectType::RESTORE_HP_PERCENT:
            if (stats) {
                int amount = (stats->max_hp * effect.magnitude) / 100;
                int healed = stats->heal(amount);
                add_message(target_name + " is healed for " + std::to_string(healed) + " HP!");
            }
            break;
            
        case ItemEffectComponent::EffectType::DAMAGE_HP:
            if (stats) {
                stats->take_damage(effect.magnitude);
                std::string msg = effect.message.empty() ? 
                    target_name + " takes " + std::to_string(effect.magnitude) + " damage!" : 
                    format_effect_message(effect.message, target_name, is_player);
                add_message(msg);
            }
            break;
            
        case ItemEffectComponent::EffectType::BUFF_ATTACK:
            if (stats) {
                stats->attack += effect.magnitude;
                add_message(target_name + "'s attack increased by " + std::to_string(effect.magnitude) + "!");
            }
            break;
            
        case ItemEffectComponent::EffectType::BUFF_DEFENSE:
            if (stats) {
                stats->defense += effect.magnitude;
                add_message(target_name + "'s defense increased by " + std::to_string(effect.magnitude) + "!");
            }
            break;
            
        case ItemEffectComponent::EffectType::DEBUFF_ATTACK:
            if (stats) {
                stats->attack = std::max(0, stats->attack - effect.magnitude);
                add_message(target_name + "'s attack decreased by " + std::to_string(effect.magnitude) + "!");
            }
            break;
            
        case ItemEffectComponent::EffectType::DEBUFF_DEFENSE:
            if (stats) {
                stats->defense = std::max(0, stats->defense - effect.magnitude);
                add_message(target_name + "'s defense decreased by " + std::to_string(effect.magnitude) + "!");
            }
            break;
            
        case ItemEffectComponent::EffectType::POISON:
            if (stats) {
                // Apply poison damage over time (simplified: instant damage for now)
                int poison_damage = effect.magnitude;
                stats->take_damage(poison_damage);
                add_message(target_name + " is poisoned for " + std::to_string(poison_damage) + " damage!");
            }
            break;
            
        case ItemEffectComponent::EffectType::CUSTOM: {
            // Execute custom callback (either inline or from registry via effect_id)
            auto callback = effect.get_callback();
            if (callback) {
                callback(target_id, thrower_id, manager, [this, &target_name, is_player](const std::string& msg) {
                    add_message(format_effect_message(msg, target_name, is_player));
                });
            } else if (!effect.message.empty()) {
                add_message(format_effect_message(effect.message, target_name, is_player));
            }
            break;
        }
            
        default:
            if (!effect.message.empty()) {
                add_message(format_effect_message(effect.message, target_name, is_player));
            }
            break;
    }
}

// Calculate throw range based on item weight and player stats
int get_throw_range(EntityId item_id) {
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    StatsComponent* player_stats = manager->get_component<StatsComponent>(player_id);
    
    // Base range of 5, modified by attack stat
    int base_range = 5;
    if (player_stats) {
        base_range += player_stats->attack / 3;
    }
    
    // Heavier items have shorter range
    if (item && item->value > 100) {
        base_range -= 1;
    }
    
    return std::max(2, std::min(base_range, 10));
}

// Calculate throw damage based on item properties
int calculate_throw_damage(EntityId item_id) {
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    StatsComponent* player_stats = manager->get_component<StatsComponent>(player_id);
    
    int base_damage = 1;
    
    if (item) {
        // Weapons do their attack bonus as throw damage
        if (item->type == ItemComponent::Type::WEAPON) {
            base_damage = std::max(3, item->attack_bonus + item->base_damage / 2);
        } else {
            // Other items do minimal damage based on value (heavier = more damage)
            base_damage = 1 + item->value / 50;
        }
    }
    
    // Add player attack bonus
    if (player_stats) {
        base_damage += player_stats->attack / 4;
    }
    
    return std::max(1, base_damage);
}

// Throw an item at a target
void throw_item_at_target(EntityId item_id, EntityId target_id) {
    NameComponent* item_name = manager->get_component<NameComponent>(item_id);
    NameComponent* target_name = manager->get_component<NameComponent>(target_id);
    StatsComponent* target_stats = manager->get_component<StatsComponent>(target_id);
    ItemComponent* item = manager->get_component<ItemComponent>(item_id);
    ItemEffectComponent* effects = manager->get_component<ItemEffectComponent>(item_id);
    
    std::string i_name = item_name ? item_name->name : "item";
    std::string t_name = target_name ? target_name->name : "target";
    
    add_message("You throw the " + i_name + " at " + t_name + "!");
    
    // Check if this item has consumable effects to apply to the target
    bool applied_effects = false;
    if (effects) {
        auto on_use_effects = effects->get_effects_by_trigger(ItemEffectComponent::Trigger::ON_USE);
        if (!on_use_effects.empty()) {
            applied_effects = true;
            for (const auto& effect : on_use_effects) {
                // Roll chance
                if (effect.chance < 1.0f) {
                    if (!chance(effect.chance)) continue;
                }
                
                // Apply effect to target using the same logic as using items
                // Pass player_id as the thrower
                apply_thrown_item_effect(target_id, effect, t_name, player_id);
            }
        }
    }
    
    // If no consumable effects, deal physical throwing damage
    if (!applied_effects && target_stats) {
        int damage = calculate_throw_damage(item_id);
        int actual_damage = std::max(1, damage - target_stats->defense / 2);
        target_stats->take_damage(actual_damage);
        add_message("The " + i_name + " hits for " + std::to_string(actual_damage) + " damage!");
        
        // Check if target died from throw damage
        if (target_stats->current_hp <= 0) {
            add_message(t_name + " is defeated!");
            
            // Give XP to player
            StatsComponent* player_stats = manager->get_component<StatsComponent>(player_id);
            if (player_stats) {
                int xp_gain = target_stats->level * 10;
                player_stats->gain_experience(xp_gain);
                add_message("Gained " + std::to_string(xp_gain) + " experience!");
            }
            
            manager->destroy_entity(target_id);
        } else {
            // Target becomes hostile from being hit - add them as a personal enemy
            FactionComponent* target_faction = manager->get_component<FactionComponent>(target_id);
            if (target_faction) {
                target_faction->last_attacker = player_id;
                target_faction->add_enemy(player_id);
            }
        }
    }
    
    // Check if target died from effect damage (e.g., poison)
    if (applied_effects && target_stats && target_stats->current_hp <= 0) {
        add_message(t_name + " is defeated!");
        
        StatsComponent* player_stats = manager->get_component<StatsComponent>(player_id);
        if (player_stats) {
            int xp_gain = target_stats->level * 10;
            player_stats->gain_experience(xp_gain);
            add_message("Gained " + std::to_string(xp_gain) + " experience!");
        }
        
        manager->destroy_entity(target_id);
    }
    
    // Remove item from inventory and destroy it (thrown items are lost)
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    if (inv) {
        // Handle stackable items
        if (item && item->stackable && item->stack_count > 1) {
            item->stack_count--;
            add_message("(" + std::to_string(item->stack_count) + " remaining)");
        } else {
            inv->remove_item(item_id);
            manager->destroy_entity(item_id);
        }
    }
}

// Show throw item interface - select item then target
bool show_throw_item() {
    InventoryComponent* inv = manager->get_component<InventoryComponent>(player_id);
    if (!inv || inv->items.empty()) {
        add_message("You have nothing to throw!");
        return false;
    }
    
    PositionComponent* player_pos = manager->get_component<PositionComponent>(player_id);
    if (!player_pos) return false;
    
    int selected_item = 0;
    
    // Phase 1: Select item to throw
    while (true) {
        render();
        
        int box_width = 50;
        int box_height = std::min(static_cast<int>(inv->items.size()) + 8, 25);
        int box_x = (120 - box_width) / 2;
        int box_y = (35 - box_height) / 2;
        
        MenuWindow::draw_themed_box(console, box_x, box_y, box_width, box_height, "");
        
        console->draw_string(box_x + 2, box_y + 1, "Select Item to Throw", string_to_color("cyan"));
        console->draw_string(box_x + 2, box_y + 2, "[W/S] Select  [Enter] Confirm  [Esc] Cancel", Color::GRAY);
        
        int list_y = box_y + 4;
        int max_display = box_height - 6;
        int scroll_offset = std::max(0, selected_item - max_display + 3);
        
        for (size_t i = scroll_offset; i < inv->items.size() && list_y < box_y + box_height - 2; ++i) {
            EntityId item_id = inv->items[i];
            NameComponent* name = manager->get_component<NameComponent>(item_id);
            ItemComponent* item = manager->get_component<ItemComponent>(item_id);
            
            std::string item_name = name ? name->name : "Unknown Item";
            
            // Show stack count
            if (item && item->stackable && item->stack_count > 1) {
                item_name += " (x" + std::to_string(item->stack_count) + ")";
            }
            
            // Show throw range and damage estimate
            int range = get_throw_range(item_id);
            int damage = calculate_throw_damage(item_id);
            std::string stats = " [R:" + std::to_string(range) + " D:" + std::to_string(damage) + "]";
            
            if (static_cast<int>(i) == selected_item) {
                console->draw_string(box_x + 2, list_y, "> " + item_name + stats, Color::YELLOW);
            } else {
                console->draw_string(box_x + 2, list_y, "  " + item_name + stats, Color::WHITE);
            }
            list_y++;
        }
        
        console->present();
        
        Key key = Input::wait_for_key();
        
        if (key == Key::ESCAPE) {
            return false;
        }
        if (key == Key::W || key == Key::UP) {
            if (selected_item > 0) selected_item--;
        }
        if (key == Key::S || key == Key::DOWN) {
            if (selected_item < static_cast<int>(inv->items.size()) - 1) selected_item++;
        }
        if (key == Key::ENTER || key == Key::SPACE) {
            break;  // Item selected, move to target selection
        }
    }
    
    EntityId throw_item_id = inv->items[selected_item];
    int throw_range = get_throw_range(throw_item_id);
    
    // Phase 2: Select target (any entity)
    auto targets = combat_system->get_targets_in_range(player_id, throw_range);
    
    if (targets.empty()) {
        add_message("No targets in range!");
        return false;
    }
    
    int selected_target = 0;
    NameComponent* item_name = manager->get_component<NameComponent>(throw_item_id);
    std::string i_name = item_name ? item_name->name : "item";
    
    while (true) {
        render();
        
        int box_width = 45;
        int box_height = static_cast<int>(targets.size()) + 8;
        int box_x = (120 - box_width) / 2;
        int box_y = (35 - box_height) / 2;
        
        MenuWindow::draw_themed_box(console, box_x, box_y, box_width, box_height, "");
        
        console->draw_string(box_x + 2, box_y + 1, "Throw " + i_name, string_to_color("cyan"));
        console->draw_string(box_x + 2, box_y + 2, "Range: " + std::to_string(throw_range), Color::WHITE);
        
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
            return false;
        }
        if (key == Key::W || key == Key::UP) {
            if (selected_target > 0) selected_target--;
        }
        if (key == Key::S || key == Key::DOWN) {
            if (selected_target < static_cast<int>(targets.size()) - 1) selected_target++;
        }
        if (key == Key::ENTER || key == Key::SPACE) {
            EntityId target_id = targets[selected_target];
            throw_item_at_target(throw_item_id, target_id);
            return true;
        }
    }
}