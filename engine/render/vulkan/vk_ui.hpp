#pragma once

// =============================================================================
// Vulkan 2D Engine - Immediate-Mode UI System
// =============================================================================
// Lightweight immediate-mode UI built on top of VkRenderer's sprite batch.
// No retained widget tree — call functions each frame to describe your UI.
//
// Features:
//   - Panels, windows (with title bar + close button)
//   - Buttons, labels, selectables, progress bars, separators
//   - Tooltips
//   - Auto-layout inside windows (vertical stacking, same_line)
//   - Skinnable: flat colors (UIStyle), textured 9-slice (UISkin), or custom draw callbacks
//   - Screen-space rendering (ignores camera)
//
// Usage:
//   VkUI ui;
//   // In your game loop:
//   ui.begin(renderer);
//   if (ui.button("Play")) { start_game(); }
//   ui.end();
// =============================================================================

#include "vk_types.hpp"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

class VkRenderer;

// ---------------------------------------------------------------------------
// Mouse state (populated each frame by VkUI::begin)
// ---------------------------------------------------------------------------

struct UIMouseState {
    float x        = 0;  // Screen-space X
    float y        = 0;  // Screen-space Y
    bool  down     = false;  // Left button currently held
    bool  pressed  = false;  // Left button went down this frame
    bool  released = false;  // Left button went up this frame
};

// ---------------------------------------------------------------------------
// Widget visual state (passed to custom draw callbacks)
// ---------------------------------------------------------------------------

enum class WidgetState {
    Normal,
    Hovered,
    Active,     // Mouse held down on the widget
    Disabled,
};

// ---------------------------------------------------------------------------
// Window flags
// ---------------------------------------------------------------------------

namespace WindowFlags {
    constexpr uint32_t None       = 0;
    constexpr uint32_t NoTitleBar = 1 << 0;
    constexpr uint32_t NoClose    = 1 << 1;
    constexpr uint32_t Modal      = 1 << 2;  // Dims background, blocks input to other windows
    constexpr uint32_t Centered   = 1 << 3;  // Center on screen
}

// ---------------------------------------------------------------------------
// UIStyle - flat color theming
// ---------------------------------------------------------------------------

struct UIStyle {
    // Window
    Color4 window_bg        = Color4::from_hex(0x1a1a24);
    Color4 window_border    = Color4::from_hex(0x404060);
    Color4 window_title_bg  = Color4::from_hex(0x2a2a40);
    Color4 window_title_fg  = Color4::white();

    // Button
    Color4 button_bg        = Color4::from_hex(0x303050);
    Color4 button_hover     = Color4::from_hex(0x404070);
    Color4 button_active    = Color4::from_hex(0x5050a0);
    Color4 button_fg        = Color4::white();

    // Selectable
    Color4 selectable_bg       = { 0, 0, 0, 0 };
    Color4 selectable_hover    = Color4::from_hex(0x303050);
    Color4 selectable_selected = Color4::from_hex(0x404070);
    Color4 selectable_fg       = Color4::white();

    // Progress bar
    Color4 progress_bg      = Color4::from_hex(0x202030);
    Color4 progress_fill    = Color4::from_hex(0x4080c0);

    // General
    Color4 text_color       = Color4::white();
    Color4 separator_color  = Color4::from_hex(0x404060);
    Color4 tooltip_bg       = Color4::from_hex(0x101018);
    Color4 tooltip_fg       = Color4::white();
    Color4 modal_dim        = { 0, 0, 0, 0.5f };
    Color4 close_btn_fg     = Color4::from_hex(0xc08080);
    Color4 close_btn_hover  = Color4::from_hex(0xff4040);

    // Sizing
    float  padding          = 6.0f;    // Interior padding
    float  item_spacing     = 4.0f;    // Vertical space between widgets
    float  border_width     = 1.0f;
    float  title_bar_height = 22.0f;
    float  text_scale       = 2.0f;  // BitmapFont scale for UI text
    float  button_height    = 24.0f;
    float  scrollbar_width  = 10.0f;
};

// ---------------------------------------------------------------------------
// 9-Slice definition (for textured skins)
// ---------------------------------------------------------------------------

struct NineSlice {
    VkDescriptorSet texture = VK_NULL_HANDLE;
    UVRect uv  = UVRect::full();   // Region within the atlas (normalized UV)
    float  bl  = 0;  // Border left   (pixels in source texture)
    float  bt  = 0;  // Border top
    float  br  = 0;  // Border right
    float  bb  = 0;  // Border bottom

    bool valid() const { return texture != VK_NULL_HANDLE; }
};

struct Sprite {
    VkDescriptorSet texture = VK_NULL_HANDLE;
    UVRect uv = UVRect::full();

    bool valid() const { return texture != VK_NULL_HANDLE; }
};

// ---------------------------------------------------------------------------
// UISkin - textured theme (overrides flat colors where set)
// ---------------------------------------------------------------------------

struct UISkin {
    NineSlice window_frame;
    NineSlice window_title;

    NineSlice button_normal;
    NineSlice button_hover;
    NineSlice button_pressed;

    NineSlice selectable_hover;
    NineSlice selectable_selected;

    NineSlice progress_bg;
    NineSlice progress_fill;

    NineSlice tooltip;

    Sprite    checkbox_on;
    Sprite    checkbox_off;
    Sprite    slider_track;
    Sprite    slider_thumb;
    Sprite    close_button;
};

// ---------------------------------------------------------------------------
// Custom draw callback signatures
// ---------------------------------------------------------------------------

using CustomRectDraw   = std::function<void(VkRenderer&, Rect2D, WidgetState)>;
using CustomWindowDraw = std::function<void(VkRenderer&, Rect2D)>;

// ---------------------------------------------------------------------------
// VkUI - Immediate-Mode UI
// ---------------------------------------------------------------------------

class VkUI {
public:
    VkUI() = default;

    // ---- Per-frame lifecycle ----

    /// Call at the start of each frame, after poll_input().
    /// Captures mouse state from SDL and sets up screen-space rendering info.
    void begin(VkRenderer& renderer);

    /// Call at the end of UI drawing, before end_frame().
    /// Draws deferred elements (tooltips, modal overlay) and finalizes state.
    void end();

    // ---- Theming ----

    void push_style(const UIStyle& style);
    void pop_style();

    void set_skin(const UISkin& skin);
    void clear_skin();

    /// Register a custom draw callback for a widget type.
    /// Valid names: "window_bg", "button", "selectable", "progress", "tooltip"
    void set_custom_draw(const std::string& name, CustomRectDraw callback);

    // ---- Windows ----

    /// Begin a window. Returns false if the user closed it (via close button).
    /// Content between begin_window/end_window is auto-laid-out vertically.
    bool begin_window(const std::string& title, Rect2D bounds,
                      uint32_t flags = WindowFlags::None);

    /// End the current window.
    void end_window();

    // ---- Widgets ----

    /// Static text label.
    void label(Rect2D bounds, const std::string& text,
               const Color4& color = { 0, 0, 0, 0 }); // {0,0,0,0} = use style

    /// Overload: label at an absolute position (auto-width).
    void label(float x, float y, const std::string& text,
               const Color4& color = { 0, 0, 0, 0 });

    /// Auto-layout label inside a window (full width).
    void label(const std::string& text,
               const Color4& color = { 0, 0, 0, 0 });

    /// Button. Returns true the frame it is clicked.
    bool button(const std::string& text);

    /// Button with explicit bounds.
    bool button(const std::string& text, Rect2D bounds);

    /// Selectable row. Returns true if clicked. Highlights when hovered.
    bool selectable(const std::string& text, bool selected = false);

    /// Progress/health bar (fraction 0..1).
    void progress_bar(Rect2D bounds, float fraction,
                      const Color4& fill_color = { 0, 0, 0, 0 });

    /// Auto-layout progress bar inside a window.
    void progress_bar(float fraction,
                      const Color4& fill_color = { 0, 0, 0, 0 });

    /// Horizontal separator line.
    void separator();

    /// Next widget is placed to the right of the previous one, instead of below.
    void same_line(float spacing = -1);

    /// Show a tooltip at the mouse position (deferred to end of frame).
    void tooltip(const std::string& text);

    /// Message log: scrollable list of strings pinned to a rect.
    void message_log(Rect2D bounds, const std::vector<std::string>& messages,
                     int max_visible = -1);

    // ---- Query ----

    /// Was the last widget hovered?
    bool hover() const { return last_widget_hovered_; }

    /// Was the last widget clicked?
    bool clicked() const { return last_widget_clicked_; }

    /// Get current mouse state.
    const UIMouseState& mouse() const { return mouse_; }

    /// Is any UI window currently hovered? (Useful for blocking game input.)
    bool any_window_hovered() const { return any_window_hovered_; }

private:
    // ---- Internal types ----

    using WidgetID = uint64_t;

    struct WindowState {
        std::string title;
        Rect2D      bounds;
        uint32_t    flags;
        float       cursor_x;  // Current layout cursor
        float       cursor_y;
        float       content_start_y;
        float       row_height; // Height of current row (for same_line)
        bool        same_line_pending;
        float       same_line_x; // Next X after same_line
    };

    // ---- Helpers ----

    WidgetID  make_id(const std::string& label) const;
    bool      point_in_rect(float px, float py, const Rect2D& r) const;
    Rect2D    layout_next(float width, float height);

    /// Convert a screen-space rect to world-space for rendering.
    Rect2D to_world(const Rect2D& screen_rect) const;
    /// Convert a screen-space point to world-space.
    void   to_world(float sx, float sy, float& wx, float& wy) const;

    void draw_nine_slice(const NineSlice& ns, const Rect2D& dest,
                         const Color4& tint = Color4::white());
    void draw_flat_rect(const Rect2D& dest, const Color4& color);
    void draw_flat_border(const Rect2D& dest, const Color4& color, float width);
    void draw_ui_text(float x, float y, const std::string& text,
                      const Color4& color, float scale = 0);
    float text_width(const std::string& text, float scale = 0) const;
    float text_height(float scale = 0) const;

    const UIStyle& style() const;

    // ---- State ----

    VkRenderer*  renderer_     = nullptr;
    UIMouseState mouse_        = {};
    UIMouseState prev_mouse_   = {};

    // Hot/active tracking
    WidgetID hot_id_    = 0;  // Widget under mouse
    WidgetID active_id_ = 0;  // Widget being pressed

    // Last-widget query
    bool last_widget_hovered_ = false;
    bool last_widget_clicked_ = false;

    // Windows
    std::vector<WindowState> window_stack_;
    bool any_window_hovered_ = false;

    // Style stack
    UIStyle         default_style_;
    std::vector<UIStyle> style_stack_;

    // Skin
    UISkin skin_;
    bool   has_skin_ = false;

    // Custom draw callbacks
    std::unordered_map<std::string, CustomRectDraw> custom_draws_;

    // Deferred tooltip
    bool        has_tooltip_ = false;
    std::string tooltip_text_;

    // Modal state
    bool modal_active_ = false;

    // Screen dimensions (updated each frame)
    float screen_w_ = 0;
    float screen_h_ = 0;
};
