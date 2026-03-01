// =============================================================================
// Underlight - Game Project
// =============================================================================
// Built on the ConsoleEngine Vulkan 2D renderer.
//
// Build:  cmake -B build -DBUILD_VULKAN_RENDERER=ON
//         cmake --build build --config Release --target underlight
// Run:    ./build/Release/underlight
//
// Controls:
//   WASD / Arrows  - Move the player
//   +/-            - Zoom in/out
//   ESC            - Quit
// =============================================================================

#include "render/vulkan/vk_renderer.hpp"
#include <cmath>

// =============================================================================
// Constants - tweak these to your liking
// =============================================================================

static constexpr int   WINDOW_W   = 1280;
static constexpr int   WINDOW_H   = 720;
static constexpr float TILE_SIZE  = 16.0f;
static constexpr int   MAP_W      = 40;
static constexpr int   MAP_H      = 25;
static constexpr float MOVE_SPEED = TILE_SIZE; // One tile per keypress

// =============================================================================
// Entry point
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    // ---- Initialise the renderer ----
    VkRenderer renderer;
    renderer.init("Underlight", WINDOW_W, WINDOW_H, /*fullscreen=*/false, /*validation=*/true);

    // ---- Set up the camera ----
    // Centre it on the middle of our tiny map
    float map_pixel_w = MAP_W * TILE_SIZE;
    float map_pixel_h = MAP_H * TILE_SIZE;
    renderer.camera().look_at(map_pixel_w / 2.0f, map_pixel_h / 2.0f);

    // ---- Player state ----
    float player_x = (MAP_W / 2) * TILE_SIZE;
    float player_y = (MAP_H / 2) * TILE_SIZE;

    // ---- Game loop ----
    while (renderer.is_running()) {
        // -- Input --
        Key key = renderer.poll_input();

        if (key == Key::ESCAPE) break;

        // Player movement (grid-based)
        if (key == Key::W || key == Key::UP)    player_y -= MOVE_SPEED;
        if (key == Key::S || key == Key::DOWN)  player_y += MOVE_SPEED;
        if (key == Key::A || key == Key::LEFT)  player_x -= MOVE_SPEED;
        if (key == Key::D || key == Key::RIGHT) player_x += MOVE_SPEED;

        // Camera zoom
        if (key == Key::PLUS)  renderer.camera().zoom *= 1.1f;
        if (key == Key::MINUS) renderer.camera().zoom *= 0.9f;

        // Camera follows player
        renderer.camera().look_at(player_x + TILE_SIZE / 2.0f,
                                  player_y + TILE_SIZE / 2.0f);

        // -- Begin frame (dark background) --
        renderer.begin_frame(0.05f, 0.05f, 0.08f);

        // -- Draw the tile map --
        for (int y = 0; y < MAP_H; ++y) {
            for (int x = 0; x < MAP_W; ++x) {
                // Checkerboard grass pattern
                float brightness = ((x + y) % 2 == 0) ? 0.18f : 0.14f;
                renderer.draw_rect(
                    { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE },
                    { 0.0f, brightness, 0.0f, 1.0f }
                );
            }
        }

        // -- Draw the player --
        renderer.draw_rect(
            { player_x, player_y, TILE_SIZE, TILE_SIZE },
            Color4::yellow()
        );

        // -- Draw HUD text (screen-space, relative to camera) --
        auto& cam = renderer.camera();
        float ui_x = cam.left() + 10.0f;
        float ui_y = cam.top()  + 10.0f;

        renderer.draw_text(ui_x, ui_y, "Underlight", Color4::white(), 2.0f);
        renderer.draw_text(ui_x, ui_y + 24.0f,
                           "WASD: move  +/-: zoom  ESC: quit",
                           { 0.6f, 0.6f, 0.6f, 1.0f }, 1.5f);

        // Show player grid position
        int grid_x = static_cast<int>(player_x / TILE_SIZE);
        int grid_y = static_cast<int>(player_y / TILE_SIZE);
        char pos_buf[64];
        snprintf(pos_buf, sizeof(pos_buf), "Pos: (%d, %d)", grid_x, grid_y);
        renderer.draw_text(ui_x, ui_y + 44.0f, pos_buf, Color4::green(), 1.5f);

        // -- End frame --
        renderer.end_frame();
    }

    // ---- Cleanup ----
    renderer.destroy();
    return 0;
}
