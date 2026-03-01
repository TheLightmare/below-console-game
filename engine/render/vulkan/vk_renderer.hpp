#pragma once

// =============================================================================
// Vulkan 2D Engine - Top-Level Renderer Facade
// =============================================================================
// VkRenderer is the user-facing API that orchestrates the Vulkan backend.
// It owns the window, context, pipelines, sprite batch, font, and camera.
//
// Minimal usage:
//   VkRenderer renderer;
//   renderer.init("My Game", 1280, 720);
//
//   while (renderer.is_running()) {
//       Key key = renderer.poll_input();
//       renderer.begin_frame();
//       renderer.draw_sprite(my_texture_set, {x,y,w,h});
//       renderer.draw_text(10, 10, "Hello!");
//       renderer.end_frame();
//   }
//
//   renderer.destroy();
// =============================================================================

#include "vk_types.hpp"
#include "vk_window.hpp"
#include "vk_context.hpp"
#include "vk_buffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_texture.hpp"
#include "vk_sprite_batch.hpp"
#include "vk_text.hpp"
#include "vk_camera.hpp"
#include "vk_input.hpp"
#include "vk_ui.hpp"

class VkRenderer {
public:
    VkRenderer() = default;
    ~VkRenderer() = default;

    // No copy
    VkRenderer(const VkRenderer&) = delete;
    VkRenderer& operator=(const VkRenderer&) = delete;

    // ---- Lifecycle ----

    /// Initialise everything: window, Vulkan, pipelines, default resources.
    void init(const char* title, int width = 1280, int height = 720,
              bool fullscreen = false, bool validation = true);

    /// Clean up all resources.
    void destroy();

    /// Returns false after the user closes the window.
    bool is_running() const { return running_; }

    // ---- Per-frame ----

    /// Poll SDL events and return the pressed Key (or Key::NONE).
    Key poll_input();

    /// Begin a new frame (acquire swapchain, clear, begin render pass).
    void begin_frame(float clear_r = 0, float clear_g = 0, float clear_b = 0);

    /// Queue a textured quad. Use load_texture() to get a texture_set.
    void draw_sprite(VkDescriptorSet texture_set,
                     const Rect2D& dest,
                     const UVRect& uv   = UVRect::full(),
                     const Color4& tint = Color4::white());

    /// Queue a solid colored rectangle (no texture).
    void draw_rect(const Rect2D& dest, const Color4& color);

    /// Queue text using the built-in bitmap font.
    void draw_text(float x, float y, const std::string& text,
                   const Color4& color = Color4::white(), float scale = 2.0f);

    /// End the frame: flush batches, end render pass, present.
    void end_frame();

    // ---- Resource management ----

    /// Load a texture from file and return its descriptor set (ready to draw).
    VkDescriptorSet load_texture(const char* filepath, bool filtering = true);

    /// Load a texture from RGBA pixels and return its descriptor set.
    VkDescriptorSet load_texture_from_pixels(const uint8_t* pixels,
                                              uint32_t w, uint32_t h,
                                              bool filtering = true);

    // ---- Camera ----

    Camera2D& camera() { return camera_; }
    const Camera2D& camera() const { return camera_; }

    // ---- Subsystem access (advanced) ----

    VkContext&     context()      { return ctx_; }
    SpriteBatch&   sprite_batch() { return batch_; }
    BitmapFont&    font()         { return font_; }
    VkWindow&      window()       { return window_; }

    /// Get the white-pixel descriptor set (useful for untextured quads).
    VkDescriptorSet white_texture_set() const { return white_tex_set_; }

    // ---- Stats ----
    uint32_t last_draw_calls() const { return batch_.last_draw_call_count(); }
    uint32_t last_quad_count() const { return batch_.last_quad_count(); }

private:
    VkWindow       window_;
    VkContext       ctx_;
    SpriteBatch    batch_;
    BitmapFont     font_;
    Camera2D       camera_;

    // Pipeline
    VkPipelineLayout  pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline        sprite_pipeline_ = VK_NULL_HANDLE;

    // Descriptor layouts
    VkDescriptorSetLayout camera_layout_  = VK_NULL_HANDLE;
    VkDescriptorSetLayout texture_layout_ = VK_NULL_HANDLE;

    // Descriptor pool
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

    // Per-frame camera UBOs
    AllocatedBuffer camera_ubos_[MAX_FRAMES_IN_FLIGHT]{};
    void*           camera_mapped_[MAX_FRAMES_IN_FLIGHT]{};
    VkDescriptorSet camera_sets_[MAX_FRAMES_IN_FLIGHT]{};

    // Default resources
    VkTexture       white_tex_;
    VkDescriptorSet white_tex_set_ = VK_NULL_HANDLE;

    VkDescriptorSet font_tex_set_ = VK_NULL_HANDLE;

    // Loaded textures (for cleanup)
    std::vector<VkTexture> loaded_textures_;

    bool running_ = false;

    void create_pipeline();
    void create_descriptors();
    void create_default_resources();
};
