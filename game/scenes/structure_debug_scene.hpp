#pragma once
#include "../../engine/scene/scene.hpp"
#include "../../engine/scene/scene_ids.hpp"
#include "../../engine/render/console.hpp"
#include "../../engine/render/renderer.hpp"
#include "../../engine/render/ui.hpp"
#include "../../engine/world/chunk.hpp"
#include "../../engine/loaders/structure_loader.hpp"
#include "../../engine/input/input.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <ctime>

// Debug scene for testing and previewing structures
class StructureDebugScene : public Scene {
private:
    StructureLoader structure_loader;
    std::vector<std::string> structure_ids;
    int current_structure_index = 0;
    
    std::unique_ptr<Chunk> preview_chunk;
    unsigned int current_seed;
    std::mt19937 rng;
    
    int camera_x = 0;
    int camera_y = 0;
    
    bool show_help = true;
    std::string status_message;
    
public:
    StructureDebugScene(Console* con, Renderer* rend, UI* ui_sys)
        : Scene(con, nullptr, rend, ui_sys) {
        current_seed = static_cast<unsigned int>(time(nullptr));
        rng.seed(current_seed);
    }
    
    void on_enter() override {
        // Load all structures
        int count = structure_loader.load_all_structures("assets/structures");
        status_message = "Loaded " + std::to_string(count) + " structures";
        
        // Get list of structure IDs
        structure_ids.clear();
        for (const auto& [id, def] : structure_loader.get_all_structures()) {
            structure_ids.push_back(id);
        }
        
        // Sort alphabetically
        std::sort(structure_ids.begin(), structure_ids.end());
        
        if (!structure_ids.empty()) {
            regenerate_preview();
        }
    }
    
    void on_exit() override {}
    
    void regenerate_preview() {
        if (structure_ids.empty()) return;
        
        // Create a fresh chunk
        preview_chunk = std::make_unique<Chunk>(ChunkCoord{0, 0});
        
        // Fill with background
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                preview_chunk->set_tile(x, y, Tile(true, '.', "dark_gray"));
            }
        }
        
        // Get current structure
        const std::string& structure_id = structure_ids[current_structure_index];
        const StructureDefinition* def = structure_loader.get_structure(structure_id);
        
        if (def && def->valid) {
            // Generate at center of chunk
            int center_x = CHUNK_SIZE / 2;
            int center_y = CHUNK_SIZE / 2;
            
            // Use village generation for structures with "village" tag
            if (def->has_tag("village") || def->id == "village") {
                structure_loader.generate_village(preview_chunk.get(), *def, center_x, center_y, rng);
                status_message = "Generated village: " + def->name;
            } else {
                structure_loader.generate_structure(preview_chunk.get(), *def, center_x, center_y, rng);
                status_message = "Generated: " + def->name;
            }
        }
        
        preview_chunk->set_generated(true);
    }
    
    void update() override {
        // Input is handled in handle_input
    }
    
    void handle_input(Key key) override {
        bool changed = false;
        
        switch (key) {
            case Key::ESCAPE:
                change_scene(SceneId::MENU);
                return;
                
            case Key::Q:
                // Previous structure
                if (current_structure_index > 0) {
                    current_structure_index--;
                    camera_x = 0;
                    camera_y = 0;
                    changed = true;
                }
                break;
                
            case Key::E:
                // Next structure
                if (current_structure_index < static_cast<int>(structure_ids.size()) - 1) {
                    current_structure_index++;
                    camera_x = 0;
                    camera_y = 0;
                    changed = true;
                }
                break;
                
            case Key::SPACE:
            case Key::R:
                current_seed = static_cast<unsigned int>(time(nullptr)) ^ rng();
                rng.seed(current_seed);
                changed = true;
                status_message = "Regenerated with new seed";
                break;
                
            default:
                {
                    auto& bindings = KeyBindings::instance();
                    if (bindings.is_action(key, GameAction::MOVE_UP)) {
                        camera_y = std::max(0, camera_y - 1);
                    } else if (bindings.is_action(key, GameAction::MOVE_DOWN)) {
                        camera_y = std::min(CHUNK_SIZE - 20, camera_y + 1);
                    } else if (bindings.is_action(key, GameAction::MOVE_LEFT)) {
                        camera_x = std::max(0, camera_x - 1);
                    } else if (bindings.is_action(key, GameAction::MOVE_RIGHT)) {
                        camera_x = std::min(CHUNK_SIZE - 1, camera_x + 1);
                    } else if (key == Key::H) {
                        show_help = !show_help;
                    } else if (key >= Key::NUM_1 && key <= Key::NUM_9) {
                        int target = static_cast<int>(key) - static_cast<int>(Key::NUM_1);
                        if (target < static_cast<int>(structure_ids.size())) {
                            current_structure_index = target;
                            changed = true;
                        }
                    }
                }
                break;
        }
        
        if (changed) {
            regenerate_preview();
        }
    }
    
    void render() override {
        console->clear();
        
        int screen_width = console->get_width();
        int screen_height = console->get_height();
        
        // Calculate viewport
        int viewport_height = screen_height - 8;  // Leave room for UI
        int viewport_width = std::min(screen_width - 2, CHUNK_SIZE);
        int start_x = (screen_width - viewport_width) / 2;
        int start_y = 3;
        
        // Draw title bar
        std::string title = "=== STRUCTURE DEBUG MODE ===";
        console->draw_string((screen_width - static_cast<int>(title.length())) / 2, 0, title, Color::CYAN);
        
        // Draw structure name and info
        if (!structure_ids.empty() && current_structure_index < static_cast<int>(structure_ids.size())) {
            const std::string& structure_id = structure_ids[current_structure_index];
            const StructureDefinition* def = structure_loader.get_structure(structure_id);
            
            std::string info = "[" + std::to_string(current_structure_index + 1) + "/" + 
                              std::to_string(structure_ids.size()) + "] ";
            if (def) {
                info += def->name + " (" + structure_id + ")";
                if (def->is_composite()) {
                    info += " [COMPOSITE]";
                }
            }
            console->draw_string((screen_width - static_cast<int>(info.length())) / 2, 1, info, Color::YELLOW);
            
            // Show description
            if (def && !def->description.empty()) {
                std::string desc = def->description;
                if (desc.length() > static_cast<size_t>(screen_width - 4)) {
                    desc = desc.substr(0, screen_width - 7) + "...";
                }
                console->draw_string((screen_width - static_cast<int>(desc.length())) / 2, 2, desc, Color::GRAY);
            }
        }
        
        // Draw the chunk preview
        if (preview_chunk) {
            // Draw border around viewport
            console->draw_box(start_x - 1, start_y - 1, viewport_width + 2, viewport_height + 2, 
                              Color::GRAY, Color::BLACK, '-', '|', '+', '+', '+', '+');
            
            // Draw tiles
            for (int y = 0; y < viewport_height && (y + camera_y) < CHUNK_SIZE; ++y) {
                for (int x = 0; x < viewport_width && (x + camera_x) < CHUNK_SIZE; ++x) {
                    const Tile& tile = preview_chunk->get_tile(x + camera_x, y + camera_y);
                    std::string sym(1, tile.symbol);
                    Color color = string_to_color(tile.color);
                    console->draw_string(start_x + x, start_y + y, sym, color);
                }
            }
            
            // Draw center marker
            int center_screen_x = start_x + CHUNK_SIZE / 2 - camera_x;
            int center_screen_y = start_y + CHUNK_SIZE / 2 - camera_y;
            if (center_screen_x >= start_x && center_screen_x < start_x + viewport_width &&
                center_screen_y >= start_y && center_screen_y < start_y + viewport_height) {
                // Show crosshair for center
                console->draw_string(center_screen_x, center_screen_y, "+", Color::MAGENTA);
            }
        }
        
        // Draw status bar
        int status_y = screen_height - 4;
        console->draw_string(2, status_y, status_message, Color::GREEN);
        console->draw_string(2, status_y + 1, "Seed: " + std::to_string(current_seed), Color::GRAY);
        
        // Draw camera position
        std::string cam_pos = "Camera: (" + std::to_string(camera_x) + ", " + std::to_string(camera_y) + ")";
        console->draw_string(screen_width - static_cast<int>(cam_pos.length()) - 2, status_y + 1, cam_pos, Color::GRAY);
        
        // Draw help
        if (show_help) {
            int help_y = screen_height - 2;
            std::string help = "[Q/E] Prev/Next  [WASD/Arrows] Pan  [R/Space] Regenerate  [H] Help  [ESC] Exit";
            console->draw_string((screen_width - static_cast<int>(help.length())) / 2, help_y, help, Color::CYAN);
        }
        
        // Draw structure list on the right if space
        if (screen_width > 80) {
            int list_x = start_x + viewport_width + 3;
            int list_y = start_y;
            console->draw_string(list_x, list_y - 1, "Structures:", Color::YELLOW);
            
            int visible_count = std::min(static_cast<int>(structure_ids.size()), viewport_height - 2);
            int start_idx = std::max(0, current_structure_index - visible_count / 2);
            
            for (int i = 0; i < visible_count && (start_idx + i) < static_cast<int>(structure_ids.size()); ++i) {
                int idx = start_idx + i;
                std::string prefix = (idx == current_structure_index) ? "> " : "  ";
                Color color = (idx == current_structure_index) ? Color::WHITE : Color::GRAY;
                
                std::string name = structure_ids[idx];
                if (name.length() > 15) name = name.substr(0, 12) + "...";
                
                console->draw_string(list_x, list_y + i, prefix + name, color);
            }
        }
        
        console->present();
    }
};
