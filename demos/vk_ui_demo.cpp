// =============================================================================
// VkUI Demo - Immediate-Mode UI Showcase
// =============================================================================
// Demonstrates every feature of the VkUI system:
//   - Windows with auto-layout, title bars, close buttons
//   - Buttons, labels, selectables, progress bars, separators
//   - Tooltips
//   - same_line() for horizontal layouts
//   - Message log
//   - Style theming (push/pop)
//   - Modal confirmation dialog
//   - Custom draw callbacks
//   - Full mouse interaction
//
// Controls:
//   Mouse        - Interact with UI
//   1            - Toggle Stats window
//   2            - Toggle Inventory window
//   3            - Toggle Theme Switcher
//   4            - Toggle Custom Draw demo
//   Q            - Show quit confirmation (modal)
//   WASD/Arrows  - Move camera (UI stays fixed on screen)
//   +/-          - Zoom camera (UI stays fixed on screen)
//   ESC          - Quit
// =============================================================================

#include "render/vulkan/vk_renderer.hpp"

#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <deque>
#include <cstdio>

// =============================================================================
// Demo State
// =============================================================================

struct DemoState {
    // Window visibility toggles
    bool show_stats      = true;
    bool show_inventory  = true;
    bool show_themes     = false;
    bool show_custom     = false;
    bool show_quit_modal = false;
    bool show_log        = true;

    // Stats
    float player_hp      = 75.0f;
    float player_max_hp  = 100.0f;
    float player_mp      = 30.0f;
    float player_max_mp  = 50.0f;
    float player_xp      = 0.65f;  // fraction
    int   player_level   = 5;
    int   gold           = 1234;

    // Inventory
    struct Item {
        std::string name;
        std::string description;
        int         quantity;
    };
    std::vector<Item> items = {
        { "Iron Sword",     "A sturdy blade. +5 ATK.",           1 },
        { "Health Potion",  "Restores 25 HP.",                   3 },
        { "Mana Potion",    "Restores 15 MP.",                   2 },
        { "Leather Armor",  "Basic protection. +3 DEF.",         1 },
        { "Torch",          "Illuminates dark areas.",            5 },
        { "Gold Key",       "Opens a locked golden door.",       1 },
        { "Scroll of Fire", "Casts a fireball. Single use.",     1 },
        { "Bread",          "Simple food. Restores 5 HP.",       8 },
    };
    int selected_item = -1;

    // Theme choice
    int current_theme = 0; // 0=default, 1=dark blue, 2=green, 3=warm

    // Message log
    std::deque<std::string> messages;
    float message_timer = 0;

    // Animation
    float time = 0;

    void add_message(const std::string& msg) {
        messages.push_back(msg);
        if (messages.size() > 50) messages.pop_front();
    }
};

// =============================================================================
// Theme definitions
// =============================================================================

static UIStyle make_default_style()   { return UIStyle{}; }

static UIStyle make_blue_style() {
    UIStyle s;
    s.window_bg       = Color4::from_hex(0x0c1628);
    s.window_border   = Color4::from_hex(0x2060a0);
    s.window_title_bg = Color4::from_hex(0x102040);
    s.window_title_fg = Color4::from_hex(0x80c0ff);
    s.button_bg       = Color4::from_hex(0x152040);
    s.button_hover    = Color4::from_hex(0x203060);
    s.button_active   = Color4::from_hex(0x3050a0);
    s.button_fg       = Color4::from_hex(0x90c0ff);
    s.selectable_hover    = Color4::from_hex(0x182848);
    s.selectable_selected = Color4::from_hex(0x203860);
    s.selectable_fg       = Color4::from_hex(0xb0d0ff);
    s.progress_bg    = Color4::from_hex(0x0c1628);
    s.progress_fill  = Color4::from_hex(0x2070c0);
    s.text_color     = Color4::from_hex(0xc0d8f0);
    s.separator_color = Color4::from_hex(0x204060);
    s.tooltip_bg      = Color4::from_hex(0x081020);
    s.tooltip_fg      = Color4::from_hex(0xa0c0e0);
    return s;
}

static UIStyle make_green_style() {
    UIStyle s;
    s.window_bg       = Color4::from_hex(0x0c1a0c);
    s.window_border   = Color4::from_hex(0x208020);
    s.window_title_bg = Color4::from_hex(0x0a200a);
    s.window_title_fg = Color4::from_hex(0x60e060);
    s.button_bg       = Color4::from_hex(0x102810);
    s.button_hover    = Color4::from_hex(0x184018);
    s.button_active   = Color4::from_hex(0x206020);
    s.button_fg       = Color4::from_hex(0x80ff80);
    s.selectable_hover    = Color4::from_hex(0x143014);
    s.selectable_selected = Color4::from_hex(0x1c4020);
    s.selectable_fg       = Color4::from_hex(0xa0ffa0);
    s.progress_bg    = Color4::from_hex(0x0a140a);
    s.progress_fill  = Color4::from_hex(0x20a020);
    s.text_color     = Color4::from_hex(0xb0f0b0);
    s.separator_color = Color4::from_hex(0x206020);
    s.tooltip_bg      = Color4::from_hex(0x081008);
    s.tooltip_fg      = Color4::from_hex(0x80d080);
    return s;
}

static UIStyle make_warm_style() {
    UIStyle s;
    s.window_bg       = Color4::from_hex(0x1a1008);
    s.window_border   = Color4::from_hex(0x806030);
    s.window_title_bg = Color4::from_hex(0x2a1810);
    s.window_title_fg = Color4::from_hex(0xf0c080);
    s.button_bg       = Color4::from_hex(0x282010);
    s.button_hover    = Color4::from_hex(0x403018);
    s.button_active   = Color4::from_hex(0x604820);
    s.button_fg       = Color4::from_hex(0xf0d8a0);
    s.selectable_hover    = Color4::from_hex(0x302418);
    s.selectable_selected = Color4::from_hex(0x403020);
    s.selectable_fg       = Color4::from_hex(0xf0e0c0);
    s.progress_bg    = Color4::from_hex(0x1a1008);
    s.progress_fill  = Color4::from_hex(0xa06020);
    s.text_color     = Color4::from_hex(0xe0d0b0);
    s.separator_color = Color4::from_hex(0x604020);
    s.tooltip_bg      = Color4::from_hex(0x100a04);
    s.tooltip_fg      = Color4::from_hex(0xd0b080);
    return s;
}

static UIStyle get_theme(int idx) {
    switch (idx) {
        case 1:  return make_blue_style();
        case 2:  return make_green_style();
        case 3:  return make_warm_style();
        default: return make_default_style();
    }
}

static const char* theme_names[] = { "Default", "Ocean Blue", "Forest Green", "Warm Amber" };
static const int   NUM_THEMES    = 4;

// =============================================================================
// UI Drawing Functions (from the "user" perspective - no engine internals)
// =============================================================================

static void draw_stats_window(VkUI& ui, DemoState& state) {
    if (!ui.begin_window("Character Stats", { 20, 20, 260, 230 })) {
        state.show_stats = false;
        return;
    }

    // Level
    char lvl[32];
    snprintf(lvl, sizeof(lvl), "Level %d Warrior", state.player_level);
    ui.label(std::string(lvl));

    ui.separator();

    // HP bar
    ui.label("HP");
    ui.progress_bar(state.player_hp / state.player_max_hp,
                    Color4::from_hex(0xc02020));
    if (ui.hover()) {
        char hp_tip[64];
        snprintf(hp_tip, sizeof(hp_tip), "%.0f / %.0f HP",
                 state.player_hp, state.player_max_hp);
        ui.tooltip(std::string(hp_tip));
    }

    // MP bar
    ui.label("MP");
    ui.progress_bar(state.player_mp / state.player_max_mp,
                    Color4::from_hex(0x2060c0));
    if (ui.hover()) {
        char mp_tip[64];
        snprintf(mp_tip, sizeof(mp_tip), "%.0f / %.0f MP",
                 state.player_mp, state.player_max_mp);
        ui.tooltip(std::string(mp_tip));
    }

    // XP bar
    ui.label("XP");
    ui.progress_bar(state.player_xp, Color4::from_hex(0x80a020));

    ui.separator();

    // Gold
    char gold_str[64];
    snprintf(gold_str, sizeof(gold_str), "Gold: %d", state.gold);
    ui.label(std::string(gold_str), Color4::from_hex(0xffd700));

    ui.end_window();
}

static void draw_inventory_window(VkUI& ui, DemoState& state) {
    if (!ui.begin_window("Inventory", { 300, 20, 300, 380 })) {
        state.show_inventory = false;
        return;
    }

    // Item list as selectables
    for (int i = 0; i < static_cast<int>(state.items.size()); i++) {
        auto& item = state.items[i];
        char label[128];
        snprintf(label, sizeof(label), "%s (x%d)", item.name.c_str(), item.quantity);

        if (ui.selectable(std::string(label), i == state.selected_item)) {
            state.selected_item = i;
            state.add_message("Selected: " + item.name);
        }

        // Tooltip with description
        if (ui.hover()) {
            ui.tooltip(item.description);
        }
    }

    ui.separator();

    // Action buttons (only if something is selected)
    if (state.selected_item >= 0 &&
        state.selected_item < static_cast<int>(state.items.size())) {

        auto& sel = state.items[state.selected_item];

        if (ui.button("Use")) {
            state.add_message("Used " + sel.name + "!");
            if (sel.name == "Health Potion") {
                state.player_hp = std::min(state.player_hp + 25.0f,
                                           state.player_max_hp);
                state.add_message("  Restored 25 HP.");
            } else if (sel.name == "Mana Potion") {
                state.player_mp = std::min(state.player_mp + 15.0f,
                                           state.player_max_mp);
                state.add_message("  Restored 15 MP.");
            }
            if (sel.quantity > 1) {
                sel.quantity--;
            } else {
                state.items.erase(state.items.begin() + state.selected_item);
                state.selected_item = -1;
            }
        }
        ui.same_line();
        if (ui.button("Drop")) {
            state.add_message("Dropped " + sel.name + ".");
            state.items.erase(state.items.begin() + state.selected_item);
            state.selected_item = -1;
        }
        ui.same_line();
        if (ui.button("Info")) {
            state.add_message(sel.name + ": " + sel.description);
        }
    } else {
        ui.label("Select an item above.", Color4::from_hex(0x808080));
    }

    ui.end_window();
}

static void draw_theme_window(VkUI& ui, DemoState& state) {
    if (!ui.begin_window("Theme Switcher", { 620, 20, 220, 180 })) {
        state.show_themes = false;
        return;
    }

    ui.label("Choose a color theme:");
    ui.separator();

    for (int i = 0; i < NUM_THEMES; i++) {
        if (ui.selectable(theme_names[i], i == state.current_theme)) {
            state.current_theme = i;
            state.add_message(std::string("Theme: ") + theme_names[i]);
        }
    }

    ui.end_window();
}

static void draw_custom_draw_window(VkUI& ui, DemoState& state, VkRenderer& renderer) {
    // Register a custom button draw callback for this window
    float t = state.time;
    ui.set_custom_draw("button", [&renderer, t](VkRenderer& r, Rect2D bounds, WidgetState ws) {
        (void)r;
        // Animated gradient background based on state
        float pulse = 0.5f + 0.5f * std::sin(t * 3.0f);
        Color4 bg;
        switch (ws) {
            case WidgetState::Active:
                bg = { 0.6f * pulse, 0.2f, 0.8f, 1.0f };
                break;
            case WidgetState::Hovered:
                bg = { 0.3f + 0.2f * pulse, 0.1f, 0.5f + 0.2f * pulse, 1.0f };
                break;
            default:
                bg = { 0.15f + 0.05f * pulse, 0.05f, 0.25f + 0.05f * pulse, 1.0f };
                break;
        }
        renderer.draw_rect(bounds, bg);

        // Animated border
        Color4 border = { 0.5f + 0.5f * pulse, 0.2f, 0.8f, 1.0f };
        float bw = 2.0f;
        renderer.draw_rect({ bounds.x, bounds.y, bounds.w, bw }, border);
        renderer.draw_rect({ bounds.x, bounds.y + bounds.h - bw, bounds.w, bw }, border);
        renderer.draw_rect({ bounds.x, bounds.y, bw, bounds.h }, border);
        renderer.draw_rect({ bounds.x + bounds.w - bw, bounds.y, bw, bounds.h }, border);
    });

    if (!ui.begin_window("Custom Draw Demo", { 620, 220, 240, 200 })) {
        state.show_custom = false;
        // Remove the custom callback so it doesn't affect other windows
        ui.set_custom_draw("button", nullptr);
        return;
    }

    ui.label("Buttons with custom");
    ui.label("animated rendering:");
    ui.separator();

    if (ui.button("Magic Spell")) {
        state.add_message("Custom button: Magic Spell cast!");
    }
    if (ui.button("Dark Portal")) {
        state.add_message("Custom button: Dark Portal opened!");
    }
    if (ui.button("Void Strike")) {
        state.add_message("Custom button: Void Strike unleashed!");
    }

    ui.end_window();

    // Remove the custom callback after this window
    ui.set_custom_draw("button", nullptr);
}

static void draw_quit_modal(VkUI& ui, DemoState& state) {
    ui.begin_window("Quit Game?",
                    { 0, 0, 280, 110 },
                    WindowFlags::Centered | WindowFlags::Modal | WindowFlags::NoClose);

    ui.label("Are you sure you want to quit?");
    ui.separator();

    if (ui.button("Yes, quit")) {
        // Signal quit — handled in main loop
        state.show_quit_modal = false;
        state.add_message("Goodbye!");
    }
    ui.same_line();
    if (ui.button("No, stay")) {
        state.show_quit_modal = false;
        state.add_message("Cancelled quit.");
    }

    ui.end_window();
}

static void draw_message_log_panel(VkUI& ui, DemoState& state) {
    // Convert deque to vector for the message_log widget
    std::vector<std::string> msgs(state.messages.begin(), state.messages.end());
    ui.message_log({ 20, 560, 580, 140 }, msgs);
}

static void draw_help_bar(VkUI& ui) {
    // Simple labels along the bottom of the screen
    ui.label(20, 715 - 16, "[1] Stats  [2] Inventory  [3] Themes  [4] Custom  [Q] Quit  [WASD] Camera  [+/-] Zoom",
             Color4::from_hex(0x808090));
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    VkRenderer renderer;
    renderer.init("VkUI Demo - Immediate-Mode UI Showcase", 1280, 720, false, true);
    renderer.camera().look_at(640.0f, 360.0f);

    VkUI ui;
    DemoState state;

    // Seed some initial messages
    state.add_message("Welcome to the VkUI demo!");
    state.add_message("Try clicking buttons and items.");
    state.add_message("Press [3] to change themes.");
    state.add_message("Press [Q] for a modal dialog.");

    bool quit = false;

    while (renderer.is_running() && !quit) {
        Key key = renderer.poll_input();

        // ---- Input handling ----
        auto& cam = renderer.camera();
        const float speed = 4.0f;
        if (key == Key::W || key == Key::UP)    cam.y -= speed;
        if (key == Key::S || key == Key::DOWN)  cam.y += speed;
        if (key == Key::A || key == Key::LEFT)  cam.x -= speed;
        if (key == Key::D || key == Key::RIGHT) cam.x += speed;
        if (key == Key::PLUS)  cam.zoom *= 1.1f;
        if (key == Key::MINUS) cam.zoom *= 0.9f;
        if (key == Key::ESCAPE) break;

        // Window toggles
        if (key == Key::NUM_1) state.show_stats     = !state.show_stats;
        if (key == Key::NUM_2) state.show_inventory  = !state.show_inventory;
        if (key == Key::NUM_3) state.show_themes     = !state.show_themes;
        if (key == Key::NUM_4) state.show_custom     = !state.show_custom;
        if (key == Key::Q)     state.show_quit_modal = true;

        // ---- Simulate game state ----
        state.time += 0.016f;
        // Slowly drain HP/MP for visual interest
        state.player_hp -= 0.02f;
        if (state.player_hp < 0) state.player_hp = 0;
        state.player_mp -= 0.01f;
        if (state.player_mp < 0) state.player_mp = 0;
        // XP slowly fills
        state.player_xp += 0.0002f;
        if (state.player_xp > 1.0f) {
            state.player_xp = 0;
            state.player_level++;
            state.add_message("Level up! Now level " + std::to_string(state.player_level) + ".");
        }
        // Periodic messages
        state.message_timer += 0.016f;
        if (state.message_timer > 5.0f) {
            state.message_timer = 0;
            static int msg_count = 0;
            msg_count++;
            if (msg_count % 3 == 0)
                state.add_message("A distant rumble echoes...");
            else if (msg_count % 3 == 1)
                state.add_message("The wind howls through the corridor.");
            else
                state.add_message("You hear footsteps nearby.");
        }

        // ---- Rendering ----
        renderer.begin_frame(0.06f, 0.06f, 0.09f);

        // Draw some background world tiles so camera movement is visible
        for (int ty = 0; ty < 50; ++ty) {
            for (int tx = 0; tx < 80; ++tx) {
                float b = ((tx + ty) % 2 == 0) ? 0.12f : 0.09f;
                // Add some variation
                if ((tx * 7 + ty * 13) % 17 == 0) b += 0.03f;
                renderer.draw_rect(
                    { tx * 16.0f, ty * 16.0f, 16.0f, 16.0f },
                    { 0.0f, b * 0.8f, b, 1.0f }
                );
            }
        }

        // A few "entities" in the world
        float px = 640.0f + std::sin(state.time * 0.5f) * 100.0f;
        float py = 360.0f + std::cos(state.time * 0.7f) * 60.0f;
        renderer.draw_rect({ px - 8, py - 8, 16, 16 }, Color4::yellow());

        renderer.draw_rect({ 300, 200, 16, 16 }, Color4::red());
        renderer.draw_rect({ 500, 400, 16, 16 }, Color4::cyan());
        renderer.draw_rect({ 700, 300, 16, 16 }, Color4::green());

        // ---- UI Layer ----
        // Apply current theme
        UIStyle current_style = get_theme(state.current_theme);
        ui.begin(renderer);
        ui.push_style(current_style);

        // Draw all visible UI elements
        if (state.show_stats)      draw_stats_window(ui, state);
        if (state.show_inventory)  draw_inventory_window(ui, state);
        if (state.show_themes)     draw_theme_window(ui, state);
        if (state.show_custom)     draw_custom_draw_window(ui, state, renderer);
        if (state.show_log)        draw_message_log_panel(ui, state);

        // Help bar along the bottom
        draw_help_bar(ui);

        // Render stats in top-right corner
        {
            char stats[128];
            snprintf(stats, sizeof(stats), "Quads: %u  Draws: %u",
                     renderer.last_quad_count(), renderer.last_draw_calls());
            float sw = ui.mouse().x;  // Just to show mouse pos
            (void)sw;
            ui.label(880, 4, std::string(stats), Color4::from_hex(0x606060));

            char mouse_str[64];
            snprintf(mouse_str, sizeof(mouse_str), "Mouse: %.0f, %.0f",
                     ui.mouse().x, ui.mouse().y);
            ui.label(880, 22, std::string(mouse_str), Color4::from_hex(0x606060));

            if (ui.any_window_hovered()) {
                ui.label(880, 40, "UI hovered (game input blocked)",
                         Color4::from_hex(0xa08040));
            }
        }

        // Modal must be drawn last (on top of everything)
        if (state.show_quit_modal) {
            draw_quit_modal(ui, state);
        }

        ui.pop_style();
        ui.end();

        renderer.end_frame();
    }

    renderer.destroy();
    return 0;
}
