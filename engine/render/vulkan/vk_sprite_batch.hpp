#pragma once

// =============================================================================
// Vulkan 2D Engine - Sprite Batch
// =============================================================================
// High-performance batched quad renderer. Collects draw calls and flushes
// them in minimal GPU draw calls, sorted by texture.
//
// Usage per frame:
//   batch.begin();
//   batch.draw(texture_set, {x, y, w, h}, uv, color);
//   batch.draw(...);
//   batch.end(cmd, camera_set, pipeline, layout);
// =============================================================================

#include "vk_types.hpp"

class VkContext;

class SpriteBatch {
public:
    SpriteBatch() = default;

    /// Initialise GPU resources (vertex buffer, index buffer).
    void init(VkContext& ctx);

    /// Start collecting quads for this frame.
    void begin();

    /// Queue a textured, tinted quad into the batch.
    /// `texture_set` is the VkDescriptorSet bound to the texture sampler.
    /// The batch will sort draws by texture_set to minimise binds.
    void draw(VkDescriptorSet texture_set,
              const Rect2D& dest,
              const UVRect& uv   = UVRect::full(),
              const Color4& tint = Color4::white(),
              float layer        = 0.0f);

    /// Draw a single quad without texture (uses the bound white-pixel texture).
    void draw_rect(VkDescriptorSet white_texture_set,
                   const Rect2D& dest,
                   const Color4& color,
                   float layer = 0.0f);

    /// Flush all queued quads to the command buffer.
    /// Binds pipeline, camera descriptor (set 0), and issues draw calls.
    void end(VkCommandBuffer cmd,
             VkDescriptorSet camera_set,
             VkPipeline      pipeline,
             VkPipelineLayout layout);

    /// Number of draw calls issued in the last end() call (for debugging).
    uint32_t last_draw_call_count() const { return last_draw_calls_; }

    /// Number of quads submitted in the last begin/end cycle.
    uint32_t last_quad_count() const { return last_quad_count_; }

    void destroy(VkContext& ctx);

private:
    struct QuadEntry {
        VkDescriptorSet texture_set;
        Vertex2D        vertices[4];
    };

    std::vector<QuadEntry> quads_;

    // Per-frame double-buffered vertex buffers (mapped)
    AllocatedBuffer vertex_buffers_[MAX_FRAMES_IN_FLIGHT]{};
    void*           vertex_mapped_[MAX_FRAMES_IN_FLIGHT]{};

    // Shared index buffer (pre-computed for MAX_QUADS_PER_BATCH)
    AllocatedBuffer index_buffer_{};

    uint32_t current_frame_  = 0;
    uint32_t last_draw_calls_ = 0;
    uint32_t last_quad_count_  = 0;

    VkContext* ctx_ = nullptr; // Stored for frame index access
};
