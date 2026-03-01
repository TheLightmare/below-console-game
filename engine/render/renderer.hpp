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
    int camera_z = 0;   // Current z-level being viewed
    
    // UI Layout - configurable side panel width (0 = no side panel)
    int side_panel_width = 0;

    // How many z-levels below the camera to show through transparent tiles
    int z_see_through_depth = 1;
    
public:
    Renderer(Console* con, ComponentManager* mgr) : console(con), manager(mgr) {}
    
    void set_side_panel_width(int width) { side_panel_width = width; }
    int get_side_panel_width() const { return side_panel_width; }

    void set_z_see_through_depth(int d) { z_see_through_depth = d; }
    int get_z_see_through_depth() const { return z_see_through_depth; }
    
    void set_camera(int x, int y, int z = 0) {
        camera_x = x;
        camera_y = y;
        camera_z = z;
    }
    
    void center_camera_on_entity(EntityId id) {
        if (PositionComponent* pos = manager->get_component<PositionComponent>(id)) {
            int viewport_width = console->get_width() - side_panel_width;
            camera_x = pos->x - viewport_width / 2;
            camera_y = pos->y - console->get_height() / 2;
            camera_z = pos->z;
        }
    }

    // ---- Fixed-size World (3D) -----------------------------------------------

    void render_map(World* world, int view_z = 0) {
        int viewport_width = console->get_width() - side_panel_width;
        int screen_height = console->get_height();
        
        for (int y = 0; y < screen_height; ++y) {
            for (int x = 0; x < viewport_width; ++x) {
                int world_x = x + camera_x;
                int world_y = y + camera_y;
                
                if (world_x >= 0 && world_x < world->get_width() && 
                    world_y >= 0 && world_y < world->get_height() &&
                    world->valid_z(view_z)) {

                    Tile& tile = world->get_tile(world_x, world_y, view_z);

                    // If tile has no floor and is transparent, show level below (dimmed)
                    if (!tile.has_floor && tile.transparent && view_z > 0 && world->valid_z(view_z - 1)) {
                        Tile& below = world->get_tile(world_x, world_y, view_z - 1);
                        Color dim = string_to_color(below.below_color.empty() ? "dark_gray" : below.below_color);
                        char sym = below.below_symbol != ' ' ? below.below_symbol : below.symbol;
                        console->set_cell(x, y, sym, dim, Color::BLACK);
                    } else {
                        Color tile_color = string_to_color(tile.color);
                        console->set_cell(x, y, tile.symbol, tile_color, Color::BLACK);
                    }
                } else {
                    console->set_cell(x, y, ' ', Color::BLACK, Color::BLACK);
                }
            }
        }
    }

    // ---- Chunked World (3D) --------------------------------------------------

    void render_chunked_map(ChunkedWorld* world, [[maybe_unused]] bool show_chunk_outlines = false) {
        int viewport_width = console->get_width() - side_panel_width;
        int screen_height = console->get_height();
        
        for (int y = 0; y < screen_height; ++y) {
            for (int x = 0; x < viewport_width; ++x) {
                int world_x = x + camera_x;
                int world_y = y + camera_y;
                
                Tile& tile = world->get_tile(world_x, world_y, camera_z);

                // See-through for transparent / no-floor tiles
                if (!tile.has_floor && tile.transparent) {
                    // Look through up to z_see_through_depth levels below
                    bool found_below = false;
                    for (int dz = 1; dz <= z_see_through_depth; ++dz) {
                        int below_z = camera_z - dz;
                        Tile* below = world->get_tile_ptr(world_x, world_y, below_z);
                        if (below && below->has_floor) {
                            Color dim = string_to_color(below->below_color.empty() ? "dark_gray" : below->below_color);
                            char sym = below->below_symbol != ' ' ? below->below_symbol : below->symbol;
                            console->set_cell(x, y, sym, dim, Color::BLACK);
                            found_below = true;
                            break;
                        }
                    }
                    if (!found_below) {
                        console->set_cell(x, y, ' ', Color::BLACK, Color::BLACK);
                    }
                } else {
                    Color tile_color = string_to_color(tile.color);
                    console->set_cell(x, y, tile.symbol, tile_color, Color::BLACK);
                }
            }
        }
    }
    
    void render_entities() {
        auto positioned_entities = manager->get_entities_with_component<PositionComponent>();
        
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
                // Only render entities on the current z-level
                if (pos->z != camera_z) continue;

                int screen_x = pos->x - camera_x;
                int screen_y = pos->y - camera_y;
                
                if (screen_x >= 0 && screen_x < console->get_width() &&
                    screen_y >= 0 && screen_y < console->get_height()) {
                    to_render.push_back({id, screen_x, screen_y, render->priority});
                }
            }
        }
        
        std::sort(to_render.begin(), to_render.end(), 
            [](const RenderInfo& a, const RenderInfo& b) {
                return a.priority < b.priority;
            });
        
        for (const auto& info : to_render) {
            RenderComponent* render = manager->get_component<RenderComponent>(info.id);
            Color fg = string_to_color(render->color);
            Color bg = render->bg_color.empty() ? Color::BLACK : string_to_color(render->bg_color);
            console->set_cell(info.x, info.y, render->ch, fg, bg);
        }
    }
    
    // ---- Convenience wrappers ------------------------------------------------

    void render_all(World* world) {
        console->clear();
        render_map(world, camera_z);
        render_entities();
    }
    
    void render_all_chunked(ChunkedWorld* world) {
        console->clear();
        render_chunked_map(world);
        render_entities();
    }
    
    void render_chunks(ChunkedWorld* world) {
        render_all_chunked(world);
    }
    
    void present() {
        console->present();
    }
    
    int get_camera_x() const { return camera_x; }
    int get_camera_y() const { return camera_y; }
    int get_camera_z() const { return camera_z; }
};
