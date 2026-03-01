// =============================================================================
// Vulkan 2D Engine - Demo / Smoke Test
// =============================================================================
// Minimal program that opens a window, renders colored rectangles and text
// using the Vulkan 2D backend. Press ESC to quit.
//
// Build:  cmake -B build -DBUILD_VULKAN_RENDERER=ON && cmake --build build
// Run:    ./build/vk_demo
// =============================================================================

#include "render/vulkan/vk_renderer.hpp"
#include <cmath>

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    VkRenderer renderer;
    renderer.init("Vulkan 2D Demo", 1280, 720, false, true);

    // Centre camera at (640, 360) -- middle of a 1280x720 world
    renderer.camera().look_at(640.0f, 360.0f);

    float time = 0.0f;

    while (renderer.is_running()) {
        Key key = renderer.poll_input();

        // Camera movement
        const float speed = 4.0f;
        auto& cam = renderer.camera();
        if (key == Key::W || key == Key::UP)    cam.y -= speed;
        if (key == Key::S || key == Key::DOWN)  cam.y += speed;
        if (key == Key::A || key == Key::LEFT)  cam.x -= speed;
        if (key == Key::D || key == Key::RIGHT) cam.x += speed;
        if (key == Key::PLUS)  cam.zoom *= 1.1f;
        if (key == Key::MINUS) cam.zoom *= 0.9f;
        if (key == Key::ESCAPE) break;

        renderer.begin_frame(0.05f, 0.05f, 0.1f); // Dark blue clear

        // ---- Draw a grid of tiles (simulating your world) ----
        for (int ty = 0; ty < 40; ++ty) {
            for (int tx = 0; tx < 80; ++tx) {
                float brightness = ((tx + ty) % 2 == 0) ? 0.15f : 0.12f;
                renderer.draw_rect(
                    { tx * 16.0f, ty * 16.0f, 16.0f, 16.0f },
                    { 0.0f, brightness, 0.0f, 1.0f }
                );
            }
        }

        // ---- Draw some "entities" ----
        // Player (yellow square)
        float px = 640.0f + std::sin(time) * 100.0f;
        float py = 360.0f + std::cos(time) * 80.0f;
        renderer.draw_rect({ px - 8, py - 8, 16, 16 }, Color4::yellow());

        // A red enemy
        renderer.draw_rect({ 300, 200, 16, 16 }, Color4::red());

        // A blue NPC
        renderer.draw_rect({ 500, 400, 16, 16 }, Color4::cyan());

        // ---- Draw UI text ----
        // Text is drawn in world-space by default.
        // For screen-space UI, you'd use a separate camera or draw at camera-relative coords.
        float ui_x = cam.left() + 10;
        float ui_y = cam.top()  + 10;
        renderer.draw_text(ui_x, ui_y, "Vulkan 2D Engine - Demo", Color4::white(), 2.0f);
        renderer.draw_text(ui_x, ui_y + 20, "WASD/Arrows: move camera  +/-: zoom  ESC: quit",
                           { 0.7f, 0.7f, 0.7f, 1.0f }, 1.5f);

        // Stats
        char stats[128];
        snprintf(stats, sizeof(stats), "Quads: %u  DrawCalls: %u",
                 renderer.last_quad_count(), renderer.last_draw_calls());
        renderer.draw_text(ui_x, ui_y + 40, stats, Color4::green(), 1.5f);

        renderer.end_frame();

        time += 0.016f;
    }

    renderer.destroy();
    return 0;
}
