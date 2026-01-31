#pragma once
#include "console.hpp"
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../world/world.hpp"
#include "../world/chunk.hpp"
#include <algorithm>
#include <vector>

class Renderer {
private:
    Console* console;
    ComponentManager* manager;
    int camera_x = 0;
    int camera_y = 0;
    
    // UI Layout constants
    static constexpr int SIDE_PANEL_WIDTH = 30;
    
public:
    Renderer(Console* con, ComponentManager* mgr) : console(con), manager(mgr) {}
    
    void set_camera(int x, int y) {
        camera_x = x;
        camera_y = y;
    }
    
    void center_camera_on_entity(EntityId id) {
        if (PositionComponent* pos = manager->get_component<PositionComponent>(id)) {
            int viewport_width = console->get_width() - SIDE_PANEL_WIDTH;
            camera_x = pos->x - viewport_width / 2;
            camera_y = pos->y - console->get_height() / 2;
        }
    }
    
    void render_map(World* world) {
        int viewport_width = console->get_width() - SIDE_PANEL_WIDTH;
        int screen_height = console->get_height();
        
        // Render tiles from world
        for (int y = 0; y < screen_height; ++y) {
            for (int x = 0; x < viewport_width; ++x) {
                int world_x = x + camera_x;
                int world_y = y + camera_y;
                
                if (world_x >= 0 && world_x < world->get_width() && 
                    world_y >= 0 && world_y < world->get_height()) {
                    Tile& tile = world->get_tile(world_x, world_y);
                    Color tile_color = string_to_color(tile.color);
                    console->set_cell(x, y, tile.symbol, tile_color, Color::BLACK);
                } else {
                    console->set_cell(x, y, ' ', Color::BLACK, Color::BLACK);
                }
            }
        }
    }
    
    // Render the chunked world map (show_chunk_outlines reserved for debug visualization)
    void render_chunked_map(ChunkedWorld* world, [[maybe_unused]] bool show_chunk_outlines = false) {
        int viewport_width = console->get_width() - SIDE_PANEL_WIDTH;
        int screen_height = console->get_height();
        
        // Render tiles from chunked world
        for (int y = 0; y < screen_height; ++y) {
            for (int x = 0; x < viewport_width; ++x) {
                int world_x = x + camera_x;
                int world_y = y + camera_y;
                
                Tile& tile = world->get_tile(world_x, world_y);
                Color tile_color = string_to_color(tile.color);
                
                console->set_cell(x, y, tile.symbol, tile_color, Color::BLACK);
            }
        }
    }
    
    void render_entities() {
        // Get all entities with position and render components
        auto positioned_entities = manager->get_entities_with_component<PositionComponent>();
        
        // Create a list of entities to render with their priority
        struct RenderInfo {
            EntityId id;
            int x, y;
            int priority;
        };
        
        std::vector<RenderInfo> to_render;
        
        for (EntityId id : positioned_entities) {
            PositionComponent* pos = manager->get_component<PositionComponent>(id);
            RenderComponent* render = manager->get_component<RenderComponent>(id);
            
            if (pos && render) {
                // Calculate screen position
                int screen_x = pos->x - camera_x;
                int screen_y = pos->y - camera_y;
                
                // Check if on screen
                if (screen_x >= 0 && screen_x < console->get_width() &&
                    screen_y >= 0 && screen_y < console->get_height()) {
                    to_render.push_back({id, screen_x, screen_y, render->priority});
                }
            }
        }
        
        // Sort by priority (lower priority renders first)
        std::sort(to_render.begin(), to_render.end(), 
            [](const RenderInfo& a, const RenderInfo& b) {
                return a.priority < b.priority;
            });
        
        // Render entities
        for (const auto& info : to_render) {
            RenderComponent* render = manager->get_component<RenderComponent>(info.id);
            Color fg = string_to_color(render->color);
            Color bg = render->bg_color.empty() ? Color::BLACK : string_to_color(render->bg_color);
            console->set_cell(info.x, info.y, render->ch, fg, bg);
        }
    }
    
    void render_all(World* world) {
        console->clear();
        render_map(world);
        render_entities();
    }
    
    void render_all_chunked(ChunkedWorld* world) {
        console->clear();
        render_chunked_map(world);
        render_entities();
    }
    
    // Alias for compatibility
    void render_chunks(ChunkedWorld* world) {
        render_all_chunked(world);
    }
    
    void present() {
        console->present();
    }
    
    int get_camera_x() const { return camera_x; }
    int get_camera_y() const { return camera_y; }
};
