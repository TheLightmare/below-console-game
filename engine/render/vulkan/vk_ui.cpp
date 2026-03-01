#include "vk_ui.hpp"
#include "vk_renderer.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

// =============================================================================
// Helpers
// =============================================================================

VkUI::WidgetID VkUI::make_id(const std::string& label) const {
    // FNV-1a hash
    uint64_t hash = 14695981039346656037ULL;
    for (char c : label) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    // Mix in the current window stack depth so that the same label in
    // different windows produces different IDs.
    hash ^= static_cast<uint64_t>(window_stack_.size()) * 2654435761ULL;
    return hash;
}

bool VkUI::point_in_rect(float px, float py, const Rect2D& r) const {
    return px >= r.x && py >= r.y && px < r.x + r.w && py < r.y + r.h;
}

const UIStyle& VkUI::style() const {
    if (!style_stack_.empty()) return style_stack_.back();
    return default_style_;
}

float VkUI::text_width(const std::string& text, float scale) const {
    if (!renderer_) return 0;
    float s = (scale > 0) ? scale : style().text_scale;
    return renderer_->font().measure_width(text, s);
}

float VkUI::text_height(float scale) const {
    if (!renderer_) return 0;
    float s = (scale > 0) ? scale : style().text_scale;
    return renderer_->font().line_height(s);
}

// =============================================================================
// Screen-to-world coordinate conversion
// =============================================================================
// All UI coordinates are in screen-space (pixels from top-left of window).
// The renderer draws in world-space through the camera, so we convert here.

Rect2D VkUI::to_world(const Rect2D& s) const {
    if (!renderer_) return s;
    const Camera2D& cam = renderer_->camera();
    return {
        cam.left() + s.x / cam.zoom,
        cam.top()  + s.y / cam.zoom,
        s.w / cam.zoom,
        s.h / cam.zoom
    };
}

void VkUI::to_world(float sx, float sy, float& wx, float& wy) const {
    if (!renderer_) { wx = sx; wy = sy; return; }
    renderer_->camera().screen_to_world(sx, sy, wx, wy);
}

// =============================================================================
// Drawing primitives (all take screen-space coords, convert internally)
// =============================================================================

void VkUI::draw_flat_rect(const Rect2D& dest, const Color4& color) {
    if (!renderer_ || color.a <= 0.0f) return;
    renderer_->draw_rect(to_world(dest), color);
}

void VkUI::draw_flat_border(const Rect2D& dest, const Color4& color, float width) {
    if (!renderer_ || color.a <= 0.0f || width <= 0.0f) return;
    float w = width / renderer_->camera().zoom;
    Rect2D d = to_world(dest);
    // Top
    renderer_->draw_rect({ d.x, d.y, d.w, w }, color);
    // Bottom
    renderer_->draw_rect({ d.x, d.y + d.h - w, d.w, w }, color);
    // Left
    renderer_->draw_rect({ d.x, d.y + w, w, d.h - 2 * w }, color);
    // Right
    renderer_->draw_rect({ d.x + d.w - w, d.y + w, w, d.h - 2 * w }, color);
}

void VkUI::draw_ui_text(float x, float y, const std::string& text,
                         const Color4& color, float scale) {
    if (!renderer_) return;
    float s = (scale > 0) ? scale : style().text_scale;
    Color4 c = (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 0)
               ? style().text_color : color;
    // Convert screen position to world, and adjust scale for zoom
    float wx, wy;
    to_world(x, y, wx, wy);
    renderer_->draw_text(wx, wy, text, c, s / renderer_->camera().zoom);
}

void VkUI::draw_nine_slice(const NineSlice& ns, const Rect2D& dest,
                            const Color4& tint) {
    if (!renderer_ || !ns.valid()) return;

    // Source region in UV space
    float su0 = ns.uv.u0, sv0 = ns.uv.v0;
    float su1 = ns.uv.u1, sv1 = ns.uv.v1;
    float sw  = su1 - su0;  // Total source width in UV
    float sh  = sv1 - sv0;  // Total source height in UV

    // We need to know the actual pixel dimensions of the source region to
    // convert pixel border sizes into UV offsets.  However, we don't have
    // the original texture dimensions easily here.  A practical approach:
    // assume border values are given as *fractions of dest size* when no
    // texture size is known.  For simplicity, use the border values directly
    // as pixel sizes in the destination and compute proportional UV splits.
    // This requires the user to specify borders in a way that corresponds to
    // their atlas layout — which is the standard 9-slice convention.
    //
    // We'll need a reference size.  Let's derive it from the UV extents:
    // If the full texture is 256x256 and the region covers 48x48 pixels,
    // the UV range is 48/256.  Border=12px in the source means border_uv = 12/256.
    // But we don't know 256.  Instead, require borders as fractions of the
    // UV region (already done if user specifies borders in [0..1] as fractions
    // of the source rect), OR more practically, the user specifies borders
    // relative to the source rect dimensions.
    //
    // Convention: bl/bt/br/bb are in the same unit as dest pixels.
    // The UV offsets are proportional: border_uv = (bl / dest.w) * sw.
    // This means borders stretch proportionally — which is exactly what
    // 9-slice is supposed to prevent for corners.
    //
    // Better convention: borders are in SOURCE PIXELS, and we need the
    // source rect in pixels.  Let's add a helper or just compute UV offsets.
    //
    // For a robust solution: treat bl/bt/br/bb as pixel sizes in the
    // destination (fixed corner size), and compute UV splits from the ratio
    // of border to full source rect.  This is correct if the user's UV rect
    // exactly covers the 9-slice source.

    float bl = ns.bl;
    float bt = ns.bt;
    float br = ns.br;
    float bb = ns.bb;

    // Clamp borders to destination size
    if (bl + br > dest.w) { float s = dest.w / (bl + br); bl *= s; br *= s; }
    if (bt + bb > dest.h) { float s = dest.h / (bt + bb); bt *= s; bb *= s; }

    // Assume source rect pixels = border sizes map proportionally
    // This means the user must set borders consistent with their atlas.
    // UV border offsets:
    float ubl = sw * (bl / (bl + (dest.w - bl - br) + br));
    float ubr = sw * (br / (bl + (dest.w - bl - br) + br));
    float vbt = sh * (bt / (bt + (dest.h - bt - bb) + bb));
    float vbb = sh * (bb / (bt + (dest.h - bt - bb) + bb));

    // For a proper 9-slice, the UV splits should correspond to fixed pixel
    // locations in the source image.  The simplest correct approach:
    // border UV = border_pixels / source_region_pixels.
    // Since we don't track source pixel size, we rely on the fact that the
    // user presumably made borders proportional to their source region.
    // Let's just use: uv_border = border / total_dest_dim * uv_range.
    // This is a reasonable approximation and works perfectly when the
    // source region has the same aspect ratio as the destination.
    
    // Actually, the cleanest approach: borders are in source pixels, and
    // the full source rect is implicitly (su1-su0) * atlas_width.
    // Since atlas_width cancels out when we compute UV offsets, we just
    // need the ratio  border / source_rect_dimension.
    // In practice: user should set bl=12 if their corner is 12px in the atlas
    // and uv covers the full 48px source.  Then:
    //   ubl = sw * (12.0 / 48.0)
    // But we don't know 48.  So let the user specify borders in UV units?
    // No — pixel units are more intuitive.
    //
    // SOLUTION: Also store source pixel width/height in NineSlice, OR
    // compute UV borders simply as:  ubl = bl / dest.w * sw.  This works
    // because the corners are drawn at exactly bl x bt dest pixels with
    // the corresponding fraction of the source UV.  As long as the user
    // makes dest at least as large as the source, corners look correct.
    
    // Let's go with the simple approach: UV border = (border / dest_dim) * uv_range
    ubl = (bl / dest.w) * sw;
    ubr = (br / dest.w) * sw;
    vbt = (bt / dest.h) * sh;
    vbb = (bb / dest.h) * sh;

    // Destination coordinates
    float dx0 = dest.x;
    float dy0 = dest.y;
    float dx1 = dest.x + bl;
    float dy1 = dest.y + bt;
    float dx2 = dest.x + dest.w - br;
    float dy2 = dest.y + dest.h - bb;

    // UV coordinates
    float u0 = su0;
    float v0 = sv0;
    float u1 = su0 + ubl;
    float v1 = sv0 + vbt;
    float u2 = su1 - ubr;
    float v2 = sv1 - vbb;
    float u3 = su1;
    float v3 = sv1;

    auto quad = [&](float qx, float qy, float qw, float qh,
                    float qu0, float qv0, float qu1, float qv1) {
        if (qw > 0 && qh > 0) {
            // Convert screen-space rect to world-space
            Rect2D world_r = to_world({ qx, qy, qw, qh });
            renderer_->draw_sprite(ns.texture, world_r,
                                   { qu0, qv0, qu1, qv1 }, tint);
        }
    };

    // Row 0: top-left, top-center, top-right
    quad(dx0, dy0, bl,          bt,          u0, v0, u1, v1);
    quad(dx1, dy0, dx2 - dx1,   bt,          u1, v0, u2, v1);
    quad(dx2, dy0, br,          bt,          u2, v0, u3, v1);

    // Row 1: mid-left, center, mid-right
    quad(dx0, dy1, bl,          dy2 - dy1,   u0, v1, u1, v2);
    quad(dx1, dy1, dx2 - dx1,   dy2 - dy1,   u1, v1, u2, v2);
    quad(dx2, dy1, br,          dy2 - dy1,   u2, v1, u3, v2);

    // Row 2: bot-left, bot-center, bot-right
    quad(dx0, dy2, bl,          bb,          u0, v2, u1, v3);
    quad(dx1, dy2, dx2 - dx1,   bb,          u1, v2, u2, v3);
    quad(dx2, dy2, br,          bb,          u2, v2, u3, v3);
}

// =============================================================================
// Per-frame lifecycle
// =============================================================================

void VkUI::begin(VkRenderer& renderer) {
    renderer_ = &renderer;

    screen_w_ = renderer.camera().viewport_width();
    screen_h_ = renderer.camera().viewport_height();

    // Capture mouse state from SDL
    prev_mouse_ = mouse_;

    int mx, my;
    Uint32 buttons = SDL_GetMouseState(&mx, &my);

    mouse_.x    = static_cast<float>(mx);
    mouse_.y    = static_cast<float>(my);
    mouse_.down = (buttons & SDL_BUTTON_LMASK) != 0;
    mouse_.pressed  = mouse_.down && !prev_mouse_.down;
    mouse_.released = !mouse_.down && prev_mouse_.down;

    // Reset per-frame state
    hot_id_ = 0;
    last_widget_hovered_ = false;
    last_widget_clicked_ = false;
    any_window_hovered_  = false;
    has_tooltip_         = false;
    modal_active_        = false;
    window_stack_.clear();
}

void VkUI::end() {
    // Draw modal dim overlay if any modal window was active
    // (The modal windows themselves are drawn inline, but the dim should
    //  logically be behind them. In an immediate-mode system the draw order
    //  is determined by call order, so the user should call modal windows last.)

    // Draw deferred tooltip on top of everything
    if (has_tooltip_ && !tooltip_text_.empty()) {
        const UIStyle& s = style();
        float pad = s.padding;
        float tw = text_width(tooltip_text_) + pad * 2;
        float th = text_height() + pad * 2;

        float tx = mouse_.x + 12;
        float ty = mouse_.y + 12;

        // Keep on screen
        if (tx + tw > screen_w_) tx = screen_w_ - tw;
        if (ty + th > screen_h_) ty = screen_h_ - th;
        if (tx < 0) tx = 0;
        if (ty < 0) ty = 0;

        Rect2D tip_rect = { tx, ty, tw, th };

        if (has_skin_ && skin_.tooltip.valid()) {
            draw_nine_slice(skin_.tooltip, tip_rect);
        } else {
            draw_flat_rect(tip_rect, s.tooltip_bg);
            draw_flat_border(tip_rect, s.window_border, s.border_width);
        }
        draw_ui_text(tx + pad, ty + pad, tooltip_text_, s.tooltip_fg);
    }

    // Clear active if mouse released
    if (mouse_.released) {
        active_id_ = 0;
    }

    renderer_ = nullptr;
}

// =============================================================================
// Theming
// =============================================================================

void VkUI::push_style(const UIStyle& s) {
    style_stack_.push_back(s);
}

void VkUI::pop_style() {
    if (!style_stack_.empty()) style_stack_.pop_back();
}

void VkUI::set_skin(const UISkin& skin) {
    skin_     = skin;
    has_skin_ = true;
}

void VkUI::clear_skin() {
    skin_     = UISkin{};
    has_skin_ = false;
}

void VkUI::set_custom_draw(const std::string& name, CustomRectDraw callback) {
    if (callback) {
        custom_draws_[name] = callback;
    } else {
        custom_draws_.erase(name);
    }
}

// =============================================================================
// Layout helpers
// =============================================================================

Rect2D VkUI::layout_next(float width, float height) {
    if (window_stack_.empty()) {
        // No window context — this shouldn't normally happen for auto-layout
        // widgets, but return a default
        return { 0, 0, width, height };
    }

    WindowState& win = window_stack_.back();
    const UIStyle& s = style();

    Rect2D r;

    if (win.same_line_pending) {
        // Place next to previous widget
        r.x = win.same_line_x;
        r.y = win.cursor_y;
        r.w = width;
        r.h = height;
        win.same_line_pending = false;
        win.same_line_x = r.x + r.w + s.item_spacing;
        // Track max row height
        if (height > win.row_height) win.row_height = height;
    } else {
        // Advance cursor past previous row
        if (win.row_height > 0) {
            win.cursor_y += win.row_height + s.item_spacing;
        }
        r.x = win.bounds.x + s.padding;
        r.y = win.cursor_y;
        r.w = width;
        r.h = height;
        win.row_height = height;
        win.same_line_x = r.x + r.w + s.item_spacing;
    }

    return r;
}

// =============================================================================
// Windows
// =============================================================================

bool VkUI::begin_window(const std::string& title, Rect2D bounds,
                         uint32_t flags) {
    const UIStyle& s = style();

    // Center on screen if requested
    if (flags & WindowFlags::Centered) {
        bounds.x = (screen_w_ - bounds.w) * 0.5f;
        bounds.y = (screen_h_ - bounds.h) * 0.5f;
    }

    // Modal: draw dim overlay
    if (flags & WindowFlags::Modal) {
        modal_active_ = true;
        draw_flat_rect({ 0, 0, screen_w_, screen_h_ }, s.modal_dim);
    }

    // Track hovering
    if (point_in_rect(mouse_.x, mouse_.y, bounds)) {
        any_window_hovered_ = true;
    }

    // Draw window background
    auto custom_it = custom_draws_.find("window_bg");
    if (custom_it != custom_draws_.end()) {
        custom_it->second(*renderer_, to_world(bounds), WidgetState::Normal);
    } else if (has_skin_ && skin_.window_frame.valid()) {
        draw_nine_slice(skin_.window_frame, bounds);
    } else {
        draw_flat_rect(bounds, s.window_bg);
        draw_flat_border(bounds, s.window_border, s.border_width);
    }

    // Title bar
    float content_y = bounds.y + s.padding;
    bool closed = false;

    if (!(flags & WindowFlags::NoTitleBar)) {
        Rect2D title_rect = { bounds.x, bounds.y, bounds.w, s.title_bar_height };

        if (has_skin_ && skin_.window_title.valid()) {
            draw_nine_slice(skin_.window_title, title_rect);
        } else {
            draw_flat_rect(title_rect, s.window_title_bg);
        }

        // Title text (centered in title bar)
        float title_tw = text_width(title);
        float title_tx = bounds.x + (bounds.w - title_tw) * 0.5f;
        float title_ty = bounds.y + (s.title_bar_height - text_height()) * 0.5f;
        draw_ui_text(title_tx, title_ty, title, s.window_title_fg);

        // Close button
        if (!(flags & WindowFlags::NoClose)) {
            float cbs = s.title_bar_height - 4;
            Rect2D close_rect = {
                bounds.x + bounds.w - cbs - 2,
                bounds.y + 2,
                cbs, cbs
            };

            bool close_hovered = point_in_rect(mouse_.x, mouse_.y, close_rect);
            Color4 close_color = close_hovered ? s.close_btn_hover : s.close_btn_fg;

            if (has_skin_ && skin_.close_button.valid()) {
                Color4 tint = close_hovered 
                    ? Color4{ 1.2f, 0.8f, 0.8f, 1.0f } 
                    : Color4::white();
                renderer_->draw_sprite(skin_.close_button.texture, close_rect,
                                       skin_.close_button.uv, tint);
            } else {
                // Draw an "X" as text
                float cx = close_rect.x + (cbs - text_width("X")) * 0.5f;
                float cy = close_rect.y + (cbs - text_height()) * 0.5f;
                draw_ui_text(cx, cy, "X", close_color);
            }

            if (close_hovered && mouse_.pressed) {
                closed = true;
            }
        }

        content_y = bounds.y + s.title_bar_height + s.padding;
    }

    // Push window onto stack for layout
    WindowState ws;
    ws.title           = title;
    ws.bounds          = bounds;
    ws.flags           = flags;
    ws.cursor_x        = bounds.x + s.padding;
    ws.cursor_y        = content_y;
    ws.content_start_y = content_y;
    ws.row_height      = 0;
    ws.same_line_pending = false;
    ws.same_line_x     = ws.cursor_x;
    window_stack_.push_back(ws);

    return !closed;
}

void VkUI::end_window() {
    if (!window_stack_.empty()) {
        window_stack_.pop_back();
    }
}

// =============================================================================
// Label
// =============================================================================

void VkUI::label(Rect2D bounds, const std::string& text, const Color4& color) {
    Color4 c = (color.a == 0) ? style().text_color : color;
    draw_ui_text(bounds.x, bounds.y, text, c);
    last_widget_hovered_ = point_in_rect(mouse_.x, mouse_.y, bounds);
    last_widget_clicked_ = last_widget_hovered_ && mouse_.pressed;
}

void VkUI::label(float x, float y, const std::string& text, const Color4& color) {
    float w = text_width(text);
    float h = text_height();
    label({ x, y, w, h }, text, color);
}

void VkUI::label(const std::string& text, const Color4& color) {
    if (window_stack_.empty()) {
        label(0, 0, text, color);
        return;
    }
    const UIStyle& s = style();
    float w = window_stack_.back().bounds.w - 2 * s.padding;
    float h = text_height();
    Rect2D r = layout_next(w, h);
    label(r, text, color);
}

// =============================================================================
// Button
// =============================================================================

bool VkUI::button(const std::string& text, Rect2D bounds) {
    const UIStyle& s  = style();
    WidgetID id       = make_id(text);

    bool hovered = point_in_rect(mouse_.x, mouse_.y, bounds);
    bool held    = (active_id_ == id);
    bool pressed = false;

    if (hovered) {
        hot_id_ = id;
        if (mouse_.pressed) {
            active_id_ = id;
        }
    }

    if (held && mouse_.released && hovered) {
        pressed = true;
    }

    WidgetState ws = WidgetState::Normal;
    if (held && hovered)  ws = WidgetState::Active;
    else if (hovered)     ws = WidgetState::Hovered;

    // Draw
    auto custom_it = custom_draws_.find("button");
    if (custom_it != custom_draws_.end()) {
        custom_it->second(*renderer_, to_world(bounds), ws);
    } else if (has_skin_) {
        const NineSlice* ns = &skin_.button_normal;
        if (ws == WidgetState::Active  && skin_.button_pressed.valid()) ns = &skin_.button_pressed;
        else if (ws == WidgetState::Hovered && skin_.button_hover.valid()) ns = &skin_.button_hover;

        if (ns->valid()) {
            draw_nine_slice(*ns, bounds);
        } else {
            // Fallback to flat
            Color4 bg = (ws == WidgetState::Active)  ? s.button_active :
                        (ws == WidgetState::Hovered) ? s.button_hover : s.button_bg;
            draw_flat_rect(bounds, bg);
            draw_flat_border(bounds, s.window_border, s.border_width);
        }
    } else {
        Color4 bg = (ws == WidgetState::Active)  ? s.button_active :
                    (ws == WidgetState::Hovered) ? s.button_hover : s.button_bg;
        draw_flat_rect(bounds, bg);
        draw_flat_border(bounds, s.window_border, s.border_width);
    }

    // Center text
    float tw = text_width(text);
    float th = text_height();
    float tx = bounds.x + (bounds.w - tw) * 0.5f;
    float ty = bounds.y + (bounds.h - th) * 0.5f;
    draw_ui_text(tx, ty, text, s.button_fg);

    last_widget_hovered_ = hovered;
    last_widget_clicked_ = pressed;
    return pressed;
}

bool VkUI::button(const std::string& text) {
    if (window_stack_.empty()) {
        return button(text, { 0, 0, 100, style().button_height });
    }
    const UIStyle& s = style();
    float w = text_width(text) + s.padding * 2;
    float h = s.button_height;
    Rect2D r = layout_next(w, h);
    return button(text, r);
}

// =============================================================================
// Selectable
// =============================================================================

bool VkUI::selectable(const std::string& text, bool selected) {
    const UIStyle& s = style();

    // Full width of the current window, or a default
    float w = 200;
    float h = text_height() + s.item_spacing;

    if (!window_stack_.empty()) {
        w = window_stack_.back().bounds.w - 2 * s.padding;
    }

    Rect2D bounds = layout_next(w, h);
    WidgetID id = make_id(text);

    bool hovered = point_in_rect(mouse_.x, mouse_.y, bounds);
    bool held    = (active_id_ == id);
    bool clicked = false;

    if (hovered) {
        hot_id_ = id;
        if (mouse_.pressed) active_id_ = id;
    }

    if (held && mouse_.released && hovered) {
        clicked = true;
    }

    WidgetState ws = WidgetState::Normal;
    if (selected)          ws = WidgetState::Active;
    else if (hovered)      ws = WidgetState::Hovered;

    // Draw background
    auto custom_it = custom_draws_.find("selectable");
    if (custom_it != custom_draws_.end()) {
        custom_it->second(*renderer_, to_world(bounds), ws);
    } else if (has_skin_) {
        if (selected && skin_.selectable_selected.valid()) {
            draw_nine_slice(skin_.selectable_selected, bounds);
        } else if (hovered && skin_.selectable_hover.valid()) {
            draw_nine_slice(skin_.selectable_hover, bounds);
        } else {
            Color4 bg = selected ? s.selectable_selected :
                        hovered  ? s.selectable_hover : s.selectable_bg;
            draw_flat_rect(bounds, bg);
        }
    } else {
        Color4 bg = selected ? s.selectable_selected :
                    hovered  ? s.selectable_hover : s.selectable_bg;
        draw_flat_rect(bounds, bg);
    }

    // Text
    float tx = bounds.x + s.padding;
    float ty = bounds.y + (h - text_height()) * 0.5f;
    draw_ui_text(tx, ty, text, s.selectable_fg);

    last_widget_hovered_ = hovered;
    last_widget_clicked_ = clicked;
    return clicked;
}

// =============================================================================
// Progress Bar
// =============================================================================

void VkUI::progress_bar(Rect2D bounds, float fraction, const Color4& fill_color) {
    const UIStyle& s = style();
    fraction = std::max(0.0f, std::min(1.0f, fraction));

    Color4 fill = (fill_color.a == 0) ? s.progress_fill : fill_color;

    auto custom_it = custom_draws_.find("progress");
    if (custom_it != custom_draws_.end()) {
        custom_it->second(*renderer_, to_world(bounds), WidgetState::Normal);
    } else if (has_skin_ && skin_.progress_bg.valid()) {
        draw_nine_slice(skin_.progress_bg, bounds);
        if (fraction > 0 && skin_.progress_fill.valid()) {
            Rect2D fill_rect = { bounds.x, bounds.y, bounds.w * fraction, bounds.h };
            draw_nine_slice(skin_.progress_fill, fill_rect);
        }
    } else {
        draw_flat_rect(bounds, s.progress_bg);
        if (fraction > 0) {
            Rect2D fill_rect = { bounds.x, bounds.y, bounds.w * fraction, bounds.h };
            draw_flat_rect(fill_rect, fill);
        }
    }

    last_widget_hovered_ = point_in_rect(mouse_.x, mouse_.y, bounds);
    last_widget_clicked_ = last_widget_hovered_ && mouse_.pressed;
}

void VkUI::progress_bar(float fraction, const Color4& fill_color) {
    if (window_stack_.empty()) {
        progress_bar({ 0, 0, 200, 14 }, fraction, fill_color);
        return;
    }
    const UIStyle& s = style();
    float w = window_stack_.back().bounds.w - 2 * s.padding;
    float h = 14;
    Rect2D r = layout_next(w, h);
    progress_bar(r, fraction, fill_color);
}

// =============================================================================
// Separator
// =============================================================================

void VkUI::separator() {
    const UIStyle& s = style();
    float w = 200;
    if (!window_stack_.empty()) {
        w = window_stack_.back().bounds.w - 2 * s.padding;
    }

    Rect2D r = layout_next(w, s.border_width);
    draw_flat_rect(r, s.separator_color);
}

// =============================================================================
// same_line
// =============================================================================

void VkUI::same_line(float spacing) {
    if (window_stack_.empty()) return;

    WindowState& win = window_stack_.back();
    const UIStyle& s = style();

    win.same_line_pending = true;
    if (spacing >= 0) {
        // Override the default spacing
        win.same_line_x = win.same_line_x - s.item_spacing + spacing;
    }
}

// =============================================================================
// Tooltip
// =============================================================================

void VkUI::tooltip(const std::string& text) {
    has_tooltip_  = true;
    tooltip_text_ = text;
}

// =============================================================================
// Message Log
// =============================================================================

void VkUI::message_log(Rect2D bounds, const std::vector<std::string>& messages,
                        int max_visible) {
    const UIStyle& s = style();

    // Draw background panel
    if (has_skin_ && skin_.window_frame.valid()) {
        draw_nine_slice(skin_.window_frame, bounds);
    } else {
        draw_flat_rect(bounds, s.window_bg);
        draw_flat_border(bounds, s.window_border, s.border_width);
    }

    float line_h = text_height();
    float pad    = s.padding;
    float inner_h = bounds.h - 2 * pad;
    int visible = (max_visible > 0) ? max_visible
                                     : static_cast<int>(inner_h / (line_h + 2));

    int total = static_cast<int>(messages.size());
    int start = std::max(0, total - visible);

    float y = bounds.y + pad;
    for (int i = start; i < total && (y + line_h) <= bounds.y + bounds.h - pad; i++) {
        draw_ui_text(bounds.x + pad, y, messages[i], s.text_color);
        y += line_h + 2;
    }

    last_widget_hovered_ = point_in_rect(mouse_.x, mouse_.y, bounds);
    last_widget_clicked_ = false;
}
