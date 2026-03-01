#pragma once

// =============================================================================
// Vulkan 2D Engine - 2D Orthographic Camera (header-only)
// =============================================================================
// Maintains position, zoom, and viewport size. Produces the 4x4 column-major
// MVP matrix consumed by the vertex shader via CameraUBO.
//
// Coordinate system:
//   World origin (0,0) is top-left by default (matching your tile engine).
//   Camera position is the world-space centre of the view.
// =============================================================================

#include "vk_types.hpp"
#include <cmath>

class Camera2D {
public:
    float x    = 0.0f;   // World-space centre X
    float y    = 0.0f;   // World-space centre Y
    float zoom = 1.0f;   // 1.0 = no zoom, 2.0 = 2x zoom in, 0.5 = zoom out

    Camera2D() = default;
    Camera2D(float cx, float cy, float z = 1.0f) : x(cx), y(cy), zoom(z) {}

    /// Set the viewport size (should match swapchain extent).
    void set_viewport(float vw, float vh) {
        viewport_w_ = vw;
        viewport_h_ = vh;
    }

    float viewport_width()  const { return viewport_w_; }
    float viewport_height() const { return viewport_h_; }

    /// Centre the camera on a world position.
    void look_at(float wx, float wy) { x = wx; y = wy; }

    /// Smoothly interpolate towards a target position (call each frame).
    void lerp_to(float tx, float ty, float t) {
        x += (tx - x) * t;
        y += (ty - y) * t;
    }

    /// World-space bounds currently visible.
    float left()   const { return x - (viewport_w_ / (2.0f * zoom)); }
    float right()  const { return x + (viewport_w_ / (2.0f * zoom)); }
    float top()    const { return y - (viewport_h_ / (2.0f * zoom)); }
    float bottom() const { return y + (viewport_h_ / (2.0f * zoom)); }

    /// Convert screen pixel coordinates to world coordinates.
    void screen_to_world(float sx, float sy, float& wx, float& wy) const {
        wx = left()  + sx / zoom;
        wy = top()   + sy / zoom;
    }

    /// Convert world coordinates to screen pixel coordinates.
    void world_to_screen(float wx, float wy, float& sx, float& sy) const {
        sx = (wx - left())  * zoom;
        sy = (wy - top())   * zoom;
    }

    /// Build the column-major 4x4 orthographic projection * view matrix.
    /// This goes directly into CameraUBO.mvp.
    ///
    /// The projection maps:
    ///   X: [left, right]   → [-1, +1]
    ///   Y: [top,  bottom]  → [-1, +1]   (Y-down world → Vulkan clip space)
    ///
    CameraUBO build_ubo() const {
        float l = left();
        float r = right();
        float t = top();
        float b = bottom();

        // Orthographic projection (column-major, Vulkan clip: Y [-1..1], Z [0..1])
        CameraUBO ubo{};
        float* m = ubo.mvp;

        // Column 0
        m[0]  =  2.0f / (r - l);
        m[1]  =  0.0f;
        m[2]  =  0.0f;
        m[3]  =  0.0f;

        // Column 1
        m[4]  =  0.0f;
        m[5]  =  2.0f / (b - t);  // Y-down
        m[6]  =  0.0f;
        m[7]  =  0.0f;

        // Column 2
        m[8]  =  0.0f;
        m[9]  =  0.0f;
        m[10] =  1.0f;
        m[11] =  0.0f;

        // Column 3 (translation)
        m[12] = -(r + l) / (r - l);
        m[13] = -(b + t) / (b - t);
        m[14] =  0.0f;
        m[15] =  1.0f;

        return ubo;
    }

private:
    float viewport_w_ = 1280.0f;
    float viewport_h_ = 720.0f;
};
