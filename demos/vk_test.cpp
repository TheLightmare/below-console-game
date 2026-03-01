// =============================================================================
// Vulkan 2D Renderer - Diagnostic Test Suite
// =============================================================================
// Cycles through isolated test pages, each exercising a specific renderer
// capability.  Use LEFT/RIGHT arrows (or A/D) to switch pages.
// Press ESC to quit.
//
// Test pages:
//   1. Solid Rects          - basic draw_rect at known positions/sizes/colors
//   2. Rect Alignment       - pixel-perfect grid alignment check
//   3. Color Accuracy       - full color spectrum, gradients, hex values
//   4. Alpha Blending       - layered semi-transparent rects
//   5. Text Basics          - draw_text at multiple scales and positions
//   6. Text Alignment       - text bounding boxes, baseline, measure_width
//   7. Camera Identity      - camera at (0,0) zoom=1; rects at screen coords
//   8. Camera Pan           - camera offset; fixed-world rects stay correct
//   9. Camera Zoom          - zoom in/out; world rects should scale correctly
//  10. Draw Order           - overlapping rects to test painter's order
//  11. Procedural Texture   - load_texture_from_pixels checkerboard
//  12. Coordinate Edges     - rects at viewport boundaries (clip test)
//  13. Stress Test          - 5000+ quads, FPS counter
//  14. Negative / Zero      - zero-size, negative-size, off-screen rects
// =============================================================================

#include "render/vulkan/vk_renderer.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

// =============================================================================
// Helpers
// =============================================================================

static constexpr int NUM_TESTS = 14;

static const char* TEST_NAMES[NUM_TESTS] = {
    "1. Solid Rects",
    "2. Rect Alignment",
    "3. Color Accuracy",
    "4. Alpha Blending",
    "5. Text Basics",
    "6. Text Alignment",
    "7. Camera Identity",
    "8. Camera Pan",
    "9. Camera Zoom",
    "10. Draw Order",
    "11. Procedural Texture",
    "12. Coordinate Edges",
    "13. Stress Test",
    "14. Negative / Zero",
};

// World-space coords for a 1280x720 viewport with camera at center
// When camera is at (640, 360) and zoom=1, visible area = [0..1280, 0..720]
static constexpr float VW = 1280.0f;
static constexpr float VH = 720.0f;

// Label helper: draws white text with a dark background tag
static void draw_label(VkRenderer& r, float x, float y, const std::string& text,
                       Color4 color = Color4::white(), float scale = 1.5f) {
    float w = text.size() * 8.0f * scale; // approximate
    r.draw_rect({ x - 2, y - 2, w + 4, 8.0f * scale + 4 }, { 0, 0, 0, 0.7f });
    r.draw_text(x, y, text, color, scale);
}

// =============================================================================
// Test 1: Solid Rects
// =============================================================================
static void test_solid_rects(VkRenderer& r) {
    // Left column: primary colors at known positions
    struct { float x, y, w, h; Color4 c; const char* label; } rects[] = {
        {  50, 80, 100, 60, Color4::red(),    "Red"     },
        { 200, 80, 100, 60, Color4::green(),  "Green"   },
        { 350, 80, 100, 60, Color4::blue(),   "Blue"    },
        { 500, 80, 100, 60, Color4::yellow(), "Yellow"  },
        { 650, 80, 100, 60, Color4::cyan(),   "Cyan"    },
        { 800, 80, 100, 60, Color4::white(),  "White"   },
        { 950, 80, 100, 60, Color4::black(),  "Black"   },
    };

    for (auto& rc : rects) {
        r.draw_rect({ rc.x, rc.y, rc.w, rc.h }, rc.c);
        draw_label(r, rc.x, rc.y + rc.h + 4, rc.label);
    }

    // Row 2: varying sizes  (check w/h are respected)
    r.draw_rect({  50, 200, 10, 10 },  Color4::white());
    draw_label(r, 50, 215, "10x10");
    r.draw_rect({ 100, 200, 30, 30 },  Color4::white());
    draw_label(r, 100, 235, "30x30");
    r.draw_rect({ 170, 200, 50, 25 },  Color4::white());
    draw_label(r, 170, 230, "50x25");
    r.draw_rect({ 260, 200, 25, 50 },  Color4::white());
    draw_label(r, 260, 255, "25x50");
    r.draw_rect({ 330, 200, 200, 5 },  Color4::white());
    draw_label(r, 330, 210, "200x5");
    r.draw_rect({ 330, 225, 5, 50 },   Color4::white());
    draw_label(r, 340, 230, "5x50");

    // Row 3: 1-pixel rects (stress the minimum)
    for (int i = 0; i < 20; i++) {
        r.draw_rect({ 50.0f + i * 12.0f, 310.0f, 1, 1 }, Color4::white());
    }
    draw_label(r, 50, 320, "1x1 pixels (should be 20 dots)");

    // Single pixel at exact integer coord vs half-pixel offset
    r.draw_rect({ 50, 350, 1, 1 }, Color4::green());
    draw_label(r, 60, 350, "int coord (50,350)");
    r.draw_rect({ 50.5f, 370, 1, 1 }, Color4::red());
    draw_label(r, 60, 370, "half-pixel (50.5,370)");
}

// =============================================================================
// Test 2: Rect Alignment (pixel grid)
// =============================================================================
static void test_rect_alignment(VkRenderer& r) {
    // Draw a grid of 16x16 cells; borders should be crisp with no gaps
    int cols = 30, rows = 12;
    float ox = 50, oy = 80;
    float cell = 16.0f;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float brightness = ((x + y) % 2 == 0) ? 0.25f : 0.15f;
            r.draw_rect({ ox + x * cell, oy + y * cell, cell, cell },
                        { brightness, brightness, brightness * 1.5f, 1.0f });
        }
    }

    draw_label(r, ox, oy + rows * cell + 4,
               "Grid should have NO gaps/lines between cells");

    // Adjacent rects (no gap test)
    float ax = 600, ay = 80;
    for (int i = 0; i < 10; i++) {
        Color4 c = (i % 2 == 0) ? Color4::red() : Color4::blue();
        r.draw_rect({ ax + i * 30.0f, ay, 30, 60 }, c);
    }
    draw_label(r, ax, ay + 65, "Adjacent rects: no gap between red/blue");

    // Horizontal line made of 1-px-tall rects
    for (int i = 0; i < 200; i++) {
        float shade = (float)i / 200.0f;
        r.draw_rect({ 50.0f + i * 2.0f, 360.0f, 2, 1 }, { shade, 1.0f - shade, 0.5f, 1.0f });
    }
    draw_label(r, 50, 370, "Gradient line (1px tall, 400px wide) - should be continuous");

    // Diagonal staircase (check for off-by-one)
    for (int i = 0; i < 20; i++) {
        r.draw_rect({ 50.0f + i * 8.0f, 400.0f + i * 4.0f, 8, 4 }, Color4::yellow());
    }
    draw_label(r, 50, 490, "Staircase - each step 8x4, no overlap/gap");
}

// =============================================================================
// Test 3: Color Accuracy
// =============================================================================
static void test_colors(VkRenderer& r) {
    // Hex color swatches
    struct { uint32_t hex; const char* label; } swatches[] = {
        { 0xFF0000, "#FF0000" }, { 0x00FF00, "#00FF00" }, { 0x0000FF, "#0000FF" },
        { 0xFF00FF, "#FF00FF" }, { 0xFFFF00, "#FFFF00" }, { 0x00FFFF, "#00FFFF" },
        { 0xFFFFFF, "#FFFFFF" }, { 0x808080, "#808080" }, { 0x404040, "#404040" },
        { 0xFF8800, "#FF8800" }, { 0x8800FF, "#8800FF" }, { 0x00FF88, "#00FF88" },
    };

    float sx = 50, sy = 80;
    for (int i = 0; i < 12; i++) {
        int col = i % 6, row = i / 6;
        float x = sx + col * 160.0f;
        float y = sy + row * 90.0f;
        r.draw_rect({ x, y, 80, 50 }, Color4::from_hex(swatches[i].hex));
        draw_label(r, x, y + 55, swatches[i].label);
    }

    // RGB gradient bars
    float gy = 290;
    for (int i = 0; i < 256; i++) {
        float fi = i / 255.0f;
        float x = 50.0f + i * 3.0f;
        r.draw_rect({ x, gy,      3, 20 }, { fi, 0, 0, 1 }); // Red ramp
        r.draw_rect({ x, gy + 25, 3, 20 }, { 0, fi, 0, 1 }); // Green ramp
        r.draw_rect({ x, gy + 50, 3, 20 }, { 0, 0, fi, 1 }); // Blue ramp
        r.draw_rect({ x, gy + 75, 3, 20 }, { fi, fi, fi, 1 }); // Gray ramp
    }
    draw_label(r, 50, gy - 15, "R / G / B / Gray gradients (0..255)");

    // from_rgb test
    r.draw_rect({ 50, 480, 60, 30 }, Color4::from_rgb(128, 0, 255));
    draw_label(r, 50, 515, "from_rgb(128,0,255)");
    r.draw_rect({ 150, 480, 60, 30 }, Color4::from_rgb(0, 200, 100));
    draw_label(r, 150, 515, "from_rgb(0,200,100)");
}

// =============================================================================
// Test 4: Alpha Blending
// =============================================================================
static void test_alpha(VkRenderer& r) {
    // Background reference pattern
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 20; x++) {
            float b = ((x + y) % 2 == 0) ? 0.3f : 0.15f;
            r.draw_rect({ 50.0f + x * 30.0f, 80.0f + y * 30.0f, 30, 30 },
                        { b, b, b, 1.0f });
        }

    // Alpha ramp
    for (int i = 0; i <= 10; i++) {
        float a = i / 10.0f;
        r.draw_rect({ 50.0f + i * 55.0f, 90.0f, 50, 50 }, { 1, 0, 0, a });
        char buf[16]; snprintf(buf, sizeof(buf), "a=%.1f", a);
        draw_label(r, 50.0f + i * 55.0f, 145, buf);
    }

    // Overlapping semi-transparent
    r.draw_rect({ 100, 200, 200, 100 }, { 1, 0, 0, 0.5f });
    r.draw_rect({ 200, 200, 200, 100 }, { 0, 1, 0, 0.5f });
    r.draw_rect({ 150, 250, 200, 100 }, { 0, 0, 1, 0.5f });
    draw_label(r, 100, 360, "Overlapping R/G/B at alpha=0.5");

    // Full transparency (should be invisible)
    r.draw_rect({ 500, 200, 100, 100 }, { 1, 1, 1, 0.0f });
    draw_label(r, 500, 310, "alpha=0 (invisible)");

    // Near-zero alpha (should be nearly invisible, not solid)
    r.draw_rect({ 650, 200, 100, 100 }, { 1, 1, 1, 0.02f });
    draw_label(r, 650, 310, "alpha=0.02 (barely visible)");

    // Alpha + color tint
    r.draw_rect({ 500, 380, 80, 60 }, { 1.0f, 0.5f, 0.0f, 0.7f });
    draw_label(r, 500, 445, "orange a=0.7");
    r.draw_rect({ 620, 380, 80, 60 }, { 0.5f, 0.0f, 1.0f, 0.3f });
    draw_label(r, 620, 445, "purple a=0.3");
}

// =============================================================================
// Test 5: Text Basics
// =============================================================================
static void test_text_basics(VkRenderer& r) {
    r.draw_text(50, 80,  "Scale 1.0",  Color4::white(), 1.0f);
    r.draw_text(50, 100, "Scale 1.5",  Color4::white(), 1.5f);
    r.draw_text(50, 125, "Scale 2.0",  Color4::white(), 2.0f);
    r.draw_text(50, 155, "Scale 3.0",  Color4::white(), 3.0f);
    r.draw_text(50, 200, "Scale 4.0 - BIG TEXT",  Color4::white(), 4.0f);

    // Colored text
    r.draw_text(50, 260, "RED text",    Color4::red(),    2.0f);
    r.draw_text(50, 285, "GREEN text",  Color4::green(),  2.0f);
    r.draw_text(50, 310, "BLUE text",   Color4::blue(),   2.0f);
    r.draw_text(50, 335, "YELLOW text", Color4::yellow(), 2.0f);
    r.draw_text(50, 360, "CYAN text",   Color4::cyan(),   2.0f);

    // Full ASCII printable range
    r.draw_text(50, 400, "!\"#$%&'()*+,-./0123456789:;<=>?", Color4::white(), 1.5f);
    r.draw_text(50, 420, "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_", Color4::white(), 1.5f);
    r.draw_text(50, 440, "`abcdefghijklmnopqrstuvwxyz{|}~",  Color4::white(), 1.5f);

    // Multi-line
    r.draw_text(50, 480, "Line 1", Color4::white(), 2.0f);
    r.draw_text(50, 500, "Line 2", Color4::white(), 2.0f);
    r.draw_text(50, 520, "Line 3", Color4::white(), 2.0f);
    draw_label(r, 50, 545, "Lines should be evenly spaced, 20px apart");
}

// =============================================================================
// Test 6: Text Alignment
// =============================================================================
static void test_text_alignment(VkRenderer& r) {
    // Draw text with a bounding box behind it to verify alignment
    auto draw_boxed = [&](float x, float y, const std::string& text, float scale) {
        float w = r.font().measure_width(text, scale);
        float h = r.font().line_height(scale);
        r.draw_rect({ x, y, w, h }, { 0.3f, 0.0f, 0.0f, 0.8f }); // dark red box
        r.draw_text(x, y, text, Color4::white(), scale);
        // Draw dot at (x,y) anchor
        r.draw_rect({ x - 1, y - 1, 3, 3 }, Color4::green());
    };

    draw_label(r, 50, 80, "Green dot = anchor point. Red box = measured bounds.");

    draw_boxed(50, 110, "Hello World", 1.0f);
    draw_boxed(50, 135, "Hello World", 1.5f);
    draw_boxed(50, 165, "Hello World", 2.0f);
    draw_boxed(50, 200, "Hello World", 3.0f);

    // Right-aligned text (manual right-align using measure_width)
    float right_edge = 700.0f;
    r.draw_rect({ right_edge, 80, 2, 200 }, Color4::yellow()); // reference line

    auto draw_right = [&](float y, const std::string& text, float scale) {
        float w = r.font().measure_width(text, scale);
        float h = r.font().line_height(scale);
        float x = right_edge - w;
        r.draw_rect({ x, y, w, h }, { 0.0f, 0.0f, 0.3f, 0.8f });
        r.draw_text(x, y, text, Color4::white(), scale);
    };

    draw_right(100, "Right-aligned 1.0", 1.0f);
    draw_right(120, "Right-aligned 1.5", 1.5f);
    draw_right(150, "Right-aligned 2.0", 2.0f);
    draw_right(185, "Right-aligned 3.0", 3.0f);
    draw_label(r, right_edge + 10, 100, "<- yellow line = right edge");

    // Centered text
    float cx = 400.0f;
    r.draw_rect({ cx, 310, 2, 150 }, Color4::yellow());
    for (int i = 0; i < 5; i++) {
        float sc = 1.0f + i * 0.5f;
        std::string s = "Center sc=" + std::to_string(sc).substr(0, 3);
        float w = r.font().measure_width(s, sc);
        float y = 310.0f + i * 28.0f;
        r.draw_text(cx - w / 2.0f, y, s, Color4::white(), sc);
    }
}

// =============================================================================
// Test 7: Camera Identity
// =============================================================================
static void test_camera_identity(VkRenderer& r) {
    // Camera is set to (VW/2, VH/2) zoom=1 by the main loop for this test.
    // World coords should map 1:1 with screen pixels.
    
    // Known screen-space positions
    r.draw_rect({ 0, 0, 20, 20 }, Color4::red());
    draw_label(r, 25, 2, "Top-Left (0,0)");

    r.draw_rect({ VW - 20, 0, 20, 20 }, Color4::green());
    draw_label(r, VW - 200, 2, "Top-Right (1260,0)");

    r.draw_rect({ 0, VH - 20, 20, 20 }, Color4::blue());
    draw_label(r, 25, VH - 18, "Bottom-Left (0,700)");

    r.draw_rect({ VW - 20, VH - 20, 20, 20 }, Color4::yellow());
    draw_label(r, VW - 250, VH - 18, "Bottom-Right (1260,700)");

    // Center crosshair
    r.draw_rect({ VW/2 - 50, VH/2, 100, 1 }, Color4::white());
    r.draw_rect({ VW/2, VH/2 - 50, 1, 100 }, Color4::white());
    draw_label(r, VW/2 + 5, VH/2 + 5, "Center (640,360)");

    // Border outline
    r.draw_rect({ 0, 0, VW, 2 }, { 1, 1, 1, 0.5f });    // top
    r.draw_rect({ 0, VH-2, VW, 2 }, { 1, 1, 1, 0.5f }); // bottom
    r.draw_rect({ 0, 0, 2, VH }, { 1, 1, 1, 0.5f });     // left
    r.draw_rect({ VW-2, 0, 2, VH }, { 1, 1, 1, 0.5f });  // right

    draw_label(r, VW/2 - 140, VH/2 + 60,
               "Corners should touch viewport edges exactly");
}

// =============================================================================
// Test 8: Camera Pan
// =============================================================================
static void test_camera_pan(VkRenderer& r, float time) {
    // Camera moves in a circle; world objects stay fixed
    float cx = VW / 2 + sinf(time * 0.5f) * 200.0f;
    float cy = VH / 2 + cosf(time * 0.5f) * 150.0f;
    r.camera().look_at(cx, cy);

    // Fixed world markers
    r.draw_rect({ 200, 200, 40, 40 }, Color4::red());
    draw_label(r, 200, 245, "Fixed (200,200)");

    r.draw_rect({ 600, 300, 40, 40 }, Color4::green());
    draw_label(r, 600, 345, "Fixed (600,300)");

    r.draw_rect({ 1000, 400, 40, 40 }, Color4::blue());
    draw_label(r, 1000, 445, "Fixed (1000,400)");

    // Grid for reference
    for (int gx = 0; gx < 80; gx++) {
        for (int gy = 0; gy < 45; gy++) {
            if (gx % 5 == 0 && gy % 5 == 0) {
                r.draw_rect({ gx * 16.0f, gy * 16.0f, 2, 2 }, { 0.3f, 0.3f, 0.3f, 1.0f });
            }
        }
    }

    // HUD in screen-space (anchored to camera)
    auto& cam = r.camera();
    float hx = cam.left() + 10;
    float hy = cam.top() + 10;
    char buf[128];
    snprintf(buf, sizeof(buf), "Camera: (%.0f, %.0f) - objects should pan smoothly", cx, cy);
    draw_label(r, hx, hy, buf, Color4::yellow());
}

// =============================================================================
// Test 9: Camera Zoom
// =============================================================================
static void test_camera_zoom(VkRenderer& r, float time) {
    // Zoom oscillates between 0.5 and 3.0
    float zoom = 1.5f + sinf(time * 0.3f) * 1.0f;
    zoom = std::max(0.5f, std::min(3.0f, zoom));
    r.camera().look_at(VW / 2, VH / 2);
    r.camera().zoom = zoom;

    // Reference grid
    for (int gx = 0; gx < 80; gx++) {
        for (int gy = 0; gy < 45; gy++) {
            float b = ((gx + gy) % 2 == 0) ? 0.12f : 0.08f;
            r.draw_rect({ gx * 16.0f, gy * 16.0f, 16, 16 }, { b, b, b, 1.0f });
        }
    }

    // Fixed world markers
    r.draw_rect({ VW/2 - 20, VH/2 - 20, 40, 40 }, Color4::red());
    draw_label(r, VW/2 + 25, VH/2 - 5, "Center marker");

    r.draw_rect({ 100, 100, 30, 30 }, Color4::green());
    r.draw_rect({ VW - 130, VH - 130, 30, 30 }, Color4::blue());

    // HUD
    auto& cam = r.camera();
    float hx = cam.left() + 10;
    float hy = cam.top() + 10;
    char buf[128];
    snprintf(buf, sizeof(buf), "Zoom: %.2f (oscillating 0.5-3.0)", zoom);
    draw_label(r, hx, hy, buf, Color4::yellow());
    draw_label(r, hx, hy + 20, "Grid cells should scale uniformly, no shearing");
}

// =============================================================================
// Test 10: Draw Order
// =============================================================================
static void test_draw_order(VkRenderer& r) {
    // Quads drawn later should appear on top (painter's algorithm)
    draw_label(r, 50, 80, "Later draws should be on top (painter's order):");

    // Three overlapping squares
    r.draw_rect({ 100, 120, 120, 120 }, Color4::red());
    r.draw_rect({ 150, 140, 120, 120 }, Color4::green());
    r.draw_rect({ 200, 160, 120, 120 }, Color4::blue());
    draw_label(r, 100, 300, "R under G under B");

    // Same but reversed order
    r.draw_rect({ 500, 160, 120, 120 }, Color4::blue());
    r.draw_rect({ 450, 140, 120, 120 }, Color4::green());
    r.draw_rect({ 400, 120, 120, 120 }, Color4::red());
    draw_label(r, 400, 300, "B under G under R");

    // Text over rect
    r.draw_rect({ 100, 350, 300, 50 }, { 0.5f, 0.0f, 0.0f, 1.0f });
    r.draw_text(110, 365, "Text on red bg", Color4::white(), 2.0f);
    draw_label(r, 100, 410, "Text should render ON TOP of red rect");

    // Rect over text (verify it covers)
    r.draw_text(500, 365, "HIDDEN TEXT!!!", Color4::white(), 2.0f);
    r.draw_rect({ 490, 355, 300, 40 }, { 0.0f, 0.0f, 0.5f, 1.0f });
    draw_label(r, 500, 410, "Blue rect should COVER the white text above");

    draw_label(r, 50, 470,
        "All layering should respect submission order (painter's algorithm).",
        Color4::green());
}

// =============================================================================
// Test 11: Procedural Texture
// =============================================================================
static VkDescriptorSet g_checker_tex = VK_NULL_HANDLE;
static VkDescriptorSet g_gradient_tex = VK_NULL_HANDLE;

static void init_test_textures(VkRenderer& r) {
    // 8x8 checkerboard
    uint8_t checker[8 * 8 * 4];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int idx = (y * 8 + x) * 4;
            uint8_t v = ((x + y) % 2 == 0) ? 255 : 0;
            checker[idx + 0] = v;
            checker[idx + 1] = v;
            checker[idx + 2] = v;
            checker[idx + 3] = 255;
        }
    }
    g_checker_tex = r.load_texture_from_pixels(checker, 8, 8, false);

    // 256x1 gradient
    uint8_t gradient[256 * 1 * 4];
    for (int x = 0; x < 256; x++) {
        int idx = x * 4;
        gradient[idx + 0] = (uint8_t)x;
        gradient[idx + 1] = (uint8_t)(255 - x);
        gradient[idx + 2] = 128;
        gradient[idx + 3] = 255;
    }
    g_gradient_tex = r.load_texture_from_pixels(gradient, 256, 1, true);
}

static void test_procedural_texture(VkRenderer& r) {
    if (g_checker_tex == VK_NULL_HANDLE) {
        draw_label(r, 50, 80, "ERROR: Checker texture failed to load!", Color4::red());
        return;
    }

    // Checker at 1:1
    r.draw_sprite(g_checker_tex, { 50, 80, 8, 8 });
    draw_label(r, 50, 95, "8x8 1:1 (nearest)");

    // Checker scaled up
    r.draw_sprite(g_checker_tex, { 50, 120, 64, 64 });
    draw_label(r, 50, 195, "8x8 -> 64x64 (should be sharp blocks)");

    // Checker scaled up more
    r.draw_sprite(g_checker_tex, { 150, 120, 128, 128 });
    draw_label(r, 150, 260, "8x8 -> 128x128");

    // Checker with partial UV
    r.draw_sprite(g_checker_tex, { 320, 120, 128, 128 },
                  { 0.0f, 0.0f, 0.5f, 0.5f });
    draw_label(r, 320, 260, "UV (0,0)-(0.5,0.5) = top-left quarter");

    // Gradient texture
    if (g_gradient_tex != VK_NULL_HANDLE) {
        r.draw_sprite(g_gradient_tex, { 50, 300, 512, 40 });
        draw_label(r, 50, 350, "256x1 gradient stretched to 512x40");

        // Tinted
        r.draw_sprite(g_gradient_tex, { 50, 380, 512, 40 },
                      UVRect::full(), { 1, 0.5f, 0.5f, 1 });
        draw_label(r, 50, 430, "Same gradient, red tint");
    }

    // White texture rect (verify the built-in white pixel works)
    r.draw_sprite(r.white_texture_set(), { 50, 470, 100, 40 },
                  UVRect::full(), Color4::green());
    draw_label(r, 50, 520, "White tex + green tint = solid green");
}

// =============================================================================
// Test 12: Coordinate Edges
// =============================================================================
static void test_coord_edges(VkRenderer& r) {
    // Rects that sit exactly at the viewport boundaries
    // These test clipping behavior

    draw_label(r, 50, 80, "Rects at viewport edges (half on, half off):");

    // Partially off-screen: left
    r.draw_rect({ -25, 200, 50, 50 }, Color4::red());
    draw_label(r, 30, 200, "<- half off-left");

    // Partially off-screen: right
    r.draw_rect({ VW - 25, 200, 50, 50 }, Color4::green());
    draw_label(r, VW - 200, 200, "half off-right ->");

    // Partially off-screen: top
    r.draw_rect({ 300, -25, 50, 50 }, Color4::blue());
    draw_label(r, 360, 5, "^ half off-top");

    // Partially off-screen: bottom
    r.draw_rect({ 300, VH - 25, 50, 50 }, Color4::yellow());
    draw_label(r, 360, VH - 40, "v half off-bottom");

    // Fully off-screen (should not crash or affect anything)
    r.draw_rect({ -200, -200, 50, 50 }, Color4::red());
    r.draw_rect({ VW + 100, VH + 100, 50, 50 }, Color4::red());
    draw_label(r, 50, 350, "2 fully off-screen rects drawn (you shouldn't see them)");

    // Huge rect going off all edges
    r.draw_rect({ -100, 420, VW + 200, 80 }, { 0.2f, 0.05f, 0.2f, 1.0f });
    draw_label(r, 50, 445, "Wide rect extends 100px past each side");

    // Very small rects near edges
    r.draw_rect({ 0, 0, 3, 3 }, Color4::white());
    r.draw_rect({ VW - 3, 0, 3, 3 }, Color4::white());
    r.draw_rect({ 0, VH - 3, 3, 3 }, Color4::white());
    r.draw_rect({ VW - 3, VH - 3, 3, 3 }, Color4::white());
    draw_label(r, 50, 530, "3x3 white dots in each corner");
}

// =============================================================================
// Test 13: Stress Test
// =============================================================================
static void test_stress(VkRenderer& r, float time) {
    int quad_count = 5000;

    for (int i = 0; i < quad_count; i++) {
        float fi = (float)i / (float)quad_count;
        float angle = fi * 6.28318f * 5.0f + time;
        float radius = 50.0f + fi * 280.0f;
        float cx = VW / 2.0f + cosf(angle) * radius;
        float cy = VH / 2.0f + sinf(angle) * radius;
        float size = 4.0f + sinf(fi * 20.0f + time * 2.0f) * 3.0f;

        Color4 c;
        c.r = 0.5f + sinf(fi * 6.28f) * 0.5f;
        c.g = 0.5f + sinf(fi * 6.28f + 2.09f) * 0.5f;
        c.b = 0.5f + sinf(fi * 6.28f + 4.18f) * 0.5f;
        c.a = 0.4f + fi * 0.6f;

        r.draw_rect({ cx - size/2, cy - size/2, size, size }, c);
    }

    auto& cam = r.camera();
    char buf[128];
    snprintf(buf, sizeof(buf), "Stress: %d quads | DrawCalls: %u | Quads: %u",
             quad_count, r.last_draw_calls(), r.last_quad_count());
    draw_label(r, cam.left() + 10, cam.top() + 10, buf, Color4::yellow());
    draw_label(r, cam.left() + 10, cam.top() + 30,
               "Should animate smoothly without hitching");
}

// =============================================================================
// Test 14: Negative / Zero / Edge Cases
// =============================================================================
static void test_edge_cases(VkRenderer& r) {
    draw_label(r, 50, 80, "Edge case rects (should not crash or corrupt):");

    // Zero-size rect
    r.draw_rect({ 100, 120, 0, 0 }, Color4::red());
    draw_label(r, 110, 120, "0x0 rect (invisible)");

    // Zero-width rect
    r.draw_rect({ 100, 150, 0, 30 }, Color4::red());
    draw_label(r, 110, 150, "0x30 rect (invisible)");

    // Zero-height rect
    r.draw_rect({ 100, 190, 30, 0 }, Color4::red());
    draw_label(r, 140, 190, "30x0 rect (invisible)");

    // Negative-width rect
    r.draw_rect({ 200, 230, -20, 20 }, Color4::yellow());
    draw_label(r, 100, 230, "Negative w (-20x20)");
    // Reference: positive version next to it
    r.draw_rect({ 300, 230, 20, 20 }, Color4::green());
    draw_label(r, 325, 230, "<- normal 20x20 for reference");

    // Negative-height rect
    r.draw_rect({ 200, 280, 20, -20 }, Color4::yellow());
    draw_label(r, 100, 280, "Negative h (20x-20)");
    r.draw_rect({ 300, 280, 20, 20 }, Color4::green());
    draw_label(r, 325, 280, "<- normal 20x20");

    // Very large rect
    r.draw_rect({ -10000, -10000, 20000, 20000 }, { 0.05f, 0.02f, 0.05f, 0.3f });
    draw_label(r, 50, 330, "20000x20000 rect (fills everything w/ faint tint)");

    // Very tiny rect
    r.draw_rect({ 50, 370, 0.5f, 0.5f }, Color4::white());
    draw_label(r, 60, 370, "0.5x0.5 sub-pixel rect");

    // Text edge cases
    r.draw_text(50, 410, "", Color4::white(), 2.0f); // empty string
    draw_label(r, 50, 410, "Empty string draw_text (nothing visible before this)");

    r.draw_text(50, 440, "A", Color4::white(), 0.0f); // zero scale
    draw_label(r, 50, 440, "Scale=0 text (invisible)");

    r.draw_text(50, 470, "Negative scale", Color4::white(), -1.0f);
    draw_label(r, 50, 490, "Scale=-1 text (undefined behavior check)");

    // NaN / infinity coords (commented out by default - uncomment to test)
    // r.draw_rect({ NAN, 500, 20, 20 }, Color4::red());
    // r.draw_rect({ 50, INFINITY, 20, 20 }, Color4::red());
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    VkRenderer renderer;
    renderer.init("Renderer Diagnostic Tests", 1280, 720, false, true);

    init_test_textures(renderer);

    int current_test = 0;
    float time = 0.0f;

    auto t_start = std::chrono::high_resolution_clock::now();
    float fps_timer = 0.0f;
    int fps_frames = 0;
    float fps_display = 0.0f;

    while (renderer.is_running()) {
        Key key = renderer.poll_input();
        if (key == Key::ESCAPE) break;

        // Navigate tests
        if (key == Key::RIGHT || key == Key::D) {
            current_test = (current_test + 1) % NUM_TESTS;
        }
        if (key == Key::LEFT || key == Key::A) {
            current_test = (current_test - 1 + NUM_TESTS) % NUM_TESTS;
        }
        // Number key shortcuts
        if (key >= Key::NUM_1 && key <= Key::NUM_9) {
            int n = static_cast<int>(key) - static_cast<int>(Key::NUM_1);
            if (n < NUM_TESTS) current_test = n;
        }

        // Reset camera to identity for most tests
        renderer.camera().look_at(VW / 2, VH / 2);
        renderer.camera().zoom = 1.0f;

        // Timing
        auto t_now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(t_now - t_start).count();
        t_start = t_now;
        time += dt;

        fps_frames++;
        fps_timer += dt;
        if (fps_timer >= 0.5f) {
            fps_display = fps_frames / fps_timer;
            fps_frames = 0;
            fps_timer = 0.0f;
        }

        renderer.begin_frame(0.06f, 0.06f, 0.08f);

        // Run current test
        switch (current_test) {
            case 0:  test_solid_rects(renderer); break;
            case 1:  test_rect_alignment(renderer); break;
            case 2:  test_colors(renderer); break;
            case 3:  test_alpha(renderer); break;
            case 4:  test_text_basics(renderer); break;
            case 5:  test_text_alignment(renderer); break;
            case 6:  test_camera_identity(renderer); break;
            case 7:  test_camera_pan(renderer, time); break;
            case 8:  test_camera_zoom(renderer, time); break;
            case 9:  test_draw_order(renderer); break;
            case 10: test_procedural_texture(renderer); break;
            case 11: test_coord_edges(renderer); break;
            case 12: test_stress(renderer, time); break;
            case 13: test_edge_cases(renderer); break;
        }

        // Header bar (screen-space, anchored to camera)
        auto& cam = renderer.camera();
        float hx = cam.left() + 10;
        float hy = cam.bottom() - 30;

        // Background strip
        renderer.draw_rect({ cam.left(), cam.bottom() - 35,
                             cam.viewport_width() / cam.zoom, 35 },
                           { 0, 0, 0, 0.8f });

        char header[256];
        snprintf(header, sizeof(header), "[%d/%d] %s    |  <-/-> switch  |  FPS: %.0f  |  Quads: %u  Draws: %u",
                 current_test + 1, NUM_TESTS,
                 TEST_NAMES[current_test],
                 fps_display,
                 renderer.last_quad_count(),
                 renderer.last_draw_calls());
        renderer.draw_text(hx, hy, header, Color4::white(), 1.5f);

        renderer.end_frame();
    }

    renderer.destroy();
    return 0;
}
