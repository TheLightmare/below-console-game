// gameplay_help.inl - Help/Wiki system for BaseGameplayScene
// This file is included by base_gameplay_scene.hpp - do not include directly

// ==================== Help System ====================

// Virtual method to get help content - override in derived classes
virtual std::vector<std::pair<std::string, std::vector<std::string>>> get_help_content() {
    return {
        {"Movement:", {"WASD or Arrow Keys - Move"}},
        {"Commands:", {
            "E - Pickup items",
            "F - Talk/Interact with NPCs",
            "R - Ranged Attack (when equipped)",
            "T - Throw item at target",
            "I - Open Inventory",
            "C - Character Stats",
            "H - Entity Wiki",
            "ESC - Pause game",
            "Q - Quit to menu",
            "[ / ] - Scroll message log"
        }},
        {"Objective:", {"Explore and defeat enemies!"}}
    };
}

virtual std::string get_help_title() {
    return "Help";
}

// Common show_help implementation - Wiki-style entity browser
void show_help() {
    // Build wiki content from entity templates
    struct WikiEntry {
        char symbol;
        std::string color;
        std::string name;
        std::string description;
    };
    
    std::map<std::string, std::vector<WikiEntry>> wiki_sections;
    
    // Category display names
    std::map<std::string, std::string> category_names = {
        {"enemy", "Enemies"},
        {"npc", "NPCs & Merchants"},
        {"item", "Items"},
        {"creature", "Creatures"},
        {"structure", "Structures"}
    };
    
    // Order of categories
    std::vector<std::string> category_order = {"npc", "enemy", "creature", "item", "structure"};
    
    // Get all templates from the entity loader
    if (factory) {
        auto& loader = factory->get_entity_loader();
        auto template_ids = loader.get_template_ids();
        
        for (const auto& id : template_ids) {
            const EntityTemplate* tmpl = loader.get_template(id);
            if (!tmpl || !tmpl->valid) continue;
            
            WikiEntry entry;
            entry.name = tmpl->name;
            entry.description = tmpl->description;
            entry.symbol = '?';
            entry.color = "white";
            
            // Extract render info from components
            if (tmpl->components.count("render")) {
                const json::Value& render = tmpl->components.at("render");
                std::string ch_str = render["char"].get_string("?");
                entry.symbol = ch_str.empty() ? '?' : ch_str[0];
                entry.color = render["color"].get_string("white");
            }
            
            // Categorize
            std::string category = tmpl->category;
            if (wiki_sections.find(category) == wiki_sections.end()) {
                wiki_sections[category] = {};
            }
            wiki_sections[category].push_back(entry);
        }
    }
    
    // Add controls section
    wiki_sections["controls"] = {};
    
    // Build flat list for display with section headers
    struct DisplayLine {
        std::string text;
        std::string color;
        char symbol = 0;
        std::string symbol_color;
        bool is_header = false;
    };
    std::vector<DisplayLine> lines;
    
    // Add controls section first
    lines.push_back({"=== Controls ===", "yellow", 0, "", true});
    lines.push_back({"  WASD / Arrows - Move", "white", 0, "", false});
    lines.push_back({"  E - Pickup items", "white", 0, "", false});
    lines.push_back({"  F - Talk/Interact with NPCs", "white", 0, "", false});
    lines.push_back({"  R - Ranged Attack (when equipped)", "white", 0, "", false});
    lines.push_back({"  T - Throw item at target", "white", 0, "", false});
    lines.push_back({"  I - Open Inventory", "white", 0, "", false});
    lines.push_back({"  C - Character Stats", "white", 0, "", false});
    lines.push_back({"  H - Show this wiki", "white", 0, "", false});
    lines.push_back({"  ESC - Pause game", "white", 0, "", false});
    lines.push_back({"  [ / ] - Scroll message log", "white", 0, "", false});
    lines.push_back({"", "white", 0, "", false});
    
    // Add entity sections
    for (const auto& cat : category_order) {
        if (wiki_sections.find(cat) == wiki_sections.end()) continue;
        auto& entries = wiki_sections[cat];
        if (entries.empty()) continue;
        
        // Sort entries by name
        std::sort(entries.begin(), entries.end(), [](const WikiEntry& a, const WikiEntry& b) {
            return a.name < b.name;
        });
        
        std::string header = "=== " + (category_names.count(cat) ? category_names[cat] : cat) + " ===";
        lines.push_back({header, "yellow", 0, "", true});
        
        for (const auto& entry : entries) {
            std::string desc = entry.description;
            if (desc.length() > 40) desc = desc.substr(0, 37) + "...";
            std::string line_text = "  " + entry.name;
            if (!desc.empty()) line_text += " - " + desc;
            lines.push_back({line_text, "white", entry.symbol, entry.color, false});
        }
        lines.push_back({"", "white", 0, "", false});
    }
    
    // Display with scrolling
    int scroll = 0;
    int panel_width = 70;
    int panel_height = console->get_height() - 4;
    int panel_x = (console->get_width() - panel_width) / 2;
    int panel_y = 2;
    int visible_lines = panel_height - 4;
    int max_scroll = std::max(0, static_cast<int>(lines.size()) - visible_lines);
    
    while (true) {
        console->clear();
        
        // Draw panel
        ui->draw_panel(panel_x, panel_y, panel_width, panel_height, "Entity Wiki", Color::YELLOW);
        
        // Draw visible lines
        int y = panel_y + 2;
        for (int i = scroll; i < static_cast<int>(lines.size()) && i < scroll + visible_lines; ++i) {
            const auto& line = lines[i];
            
            if (line.symbol != 0) {
                // Draw entity symbol
                Color sym_color = string_to_color(line.symbol_color);
                std::string sym_str(1, line.symbol);
                console->draw_string(panel_x + 2, y, sym_str, sym_color);
                console->draw_string(panel_x + 4, y, line.text, 
                                    line.is_header ? Color::CYAN : Color::WHITE);
            } else {
                console->draw_string(panel_x + 2, y, line.text, 
                                    line.is_header ? Color::CYAN : Color::WHITE);
            }
            y++;
        }
        
        // Draw scroll indicator
        if (max_scroll > 0) {
            int scroll_bar_height = std::max(1, visible_lines * visible_lines / static_cast<int>(lines.size()));
            int scroll_bar_pos = (scroll * (visible_lines - scroll_bar_height)) / max_scroll;
            for (int i = 0; i < visible_lines; ++i) {
                std::string scroll_ch = (i >= scroll_bar_pos && i < scroll_bar_pos + scroll_bar_height) ? "#" : "|";
                console->draw_string(panel_x + panel_width - 2, panel_y + 2 + i, scroll_ch, Color::DARK_GRAY);
            }
        }
        
        // Draw footer
        std::string footer = "[Up/Down/W/S] Scroll  [PgUp/PgDn] Page  [ESC/H] Close";
        console->draw_string(panel_x + (panel_width - static_cast<int>(footer.length())) / 2, 
                           panel_y + panel_height - 1, footer, Color::YELLOW);
        
        console->present();
        
        // Handle input
        Key key = Input::wait_for_key();
        switch (key) {
            case Key::UP:
            case Key::W:
                if (scroll > 0) scroll--;
                break;
            case Key::DOWN:
            case Key::S:
                if (scroll < max_scroll) scroll++;
                break;
            case Key::PAGE_UP:
                scroll = std::max(0, scroll - visible_lines);
                break;
            case Key::PAGE_DOWN:
                scroll = std::min(max_scroll, scroll + visible_lines);
                break;
            case Key::ESCAPE:
            case Key::H:
            case Key::Q:
                return;
            default:
                break;
        }
    }
}
