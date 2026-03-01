#include "vk_sprite_batch.hpp"
#include "vk_context.hpp"
#include "vk_buffer.hpp"

// =============================================================================
// Init / Destroy
// =============================================================================

void SpriteBatch::init(VkContext& ctx) {
    ctx_ = &ctx;

    // Create per-frame mapped vertex buffers
    VkDeviceSize vb_size = sizeof(Vertex2D) * MAX_VERTICES;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vertex_buffers_[i] = vkbuf::create_mapped_buffer(
            ctx.allocator, vb_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            &vertex_mapped_[i]);
    }

    // Create shared index buffer (immutable)
    index_buffer_ = vkbuf::create_quad_index_buffer(ctx, MAX_QUADS_PER_BATCH);
}

void SpriteBatch::destroy(VkContext& ctx) {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkbuf::destroy_buffer(ctx.allocator, vertex_buffers_[i]);
        vertex_mapped_[i] = nullptr;
    }
    vkbuf::destroy_buffer(ctx.allocator, index_buffer_);
    ctx_ = nullptr;
}

// =============================================================================
// Begin / Draw / End
// =============================================================================

void SpriteBatch::begin() {
    quads_.clear();
    current_frame_ = ctx_->current_frame;
}

void SpriteBatch::draw(VkDescriptorSet texture_set,
                        const Rect2D& dest,
                        const UVRect& uv,
                        const Color4& tint,
                        float layer) {
    if (quads_.size() >= MAX_QUADS_PER_BATCH) return; // Silently drop if full

    (void)layer; // Reserved for future z-sorting

    QuadEntry entry{};
    entry.texture_set = texture_set;

    float x0 = dest.x;
    float y0 = dest.y;
    float x1 = dest.x + dest.w;
    float y1 = dest.y + dest.h;

    // Top-left, top-right, bottom-right, bottom-left
    entry.vertices[0] = { x0, y0, uv.u0, uv.v0, tint.r, tint.g, tint.b, tint.a };
    entry.vertices[1] = { x1, y0, uv.u1, uv.v0, tint.r, tint.g, tint.b, tint.a };
    entry.vertices[2] = { x1, y1, uv.u1, uv.v1, tint.r, tint.g, tint.b, tint.a };
    entry.vertices[3] = { x0, y1, uv.u0, uv.v1, tint.r, tint.g, tint.b, tint.a };

    quads_.push_back(entry);
}

void SpriteBatch::draw_rect(VkDescriptorSet white_texture_set,
                              const Rect2D& dest,
                              const Color4& color,
                              float layer) {
    draw(white_texture_set, dest, UVRect::full(), color, layer);
}

void SpriteBatch::end(VkCommandBuffer cmd,
                       VkDescriptorSet camera_set,
                       VkPipeline      pipeline,
                       VkPipelineLayout layout) {
    last_quad_count_  = static_cast<uint32_t>(quads_.size());
    last_draw_calls_ = 0;

    if (quads_.empty()) return;

    // NOTE: We intentionally do NOT sort by texture descriptor set.
    // A global sort would break painter's order (later draws must appear on
    // top).  Instead we batch consecutive quads that share the same texture
    // and flush whenever the texture changes.  This may produce a few extra
    // draw calls when textures alternate, but it preserves correct layering.

    // Upload vertices to the mapped buffer for this frame
    Vertex2D* dst = static_cast<Vertex2D*>(vertex_mapped_[current_frame_]);
    for (size_t i = 0; i < quads_.size(); ++i) {
        memcpy(&dst[i * 4], quads_[i].vertices, sizeof(Vertex2D) * 4);
    }

    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Bind camera UBO (set 0)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                            0, 1, &camera_set, 0, nullptr);

    // Bind vertex + index buffers
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffers_[current_frame_].buffer, &offset);
    vkCmdBindIndexBuffer(cmd, index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT16);

    // Issue batched draw calls (one per texture change)
    VkDescriptorSet current_tex = VK_NULL_HANDLE;
    uint32_t batch_start = 0;

    auto flush_batch = [&](uint32_t batch_end) {
        if (batch_end <= batch_start) return;

        uint32_t quad_count  = batch_end - batch_start;
        uint32_t index_count = quad_count * 6;
        uint32_t first_index = batch_start * 6;
        // vertex_off = 0 because the index buffer already stores absolute
        // vertex indices (quad N → indices N*4..N*4+3).
        vkCmdDrawIndexed(cmd, index_count, 1, first_index, 0, 0);
        last_draw_calls_++;
    };

    for (uint32_t i = 0; i < static_cast<uint32_t>(quads_.size()); ++i) {
        if (quads_[i].texture_set != current_tex) {
            // Flush previous batch
            flush_batch(i);
            batch_start = i;

            // Bind new texture (set 1)
            current_tex = quads_[i].texture_set;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                                    1, 1, &current_tex, 0, nullptr);
        }
    }

    // Flush final batch
    flush_batch(static_cast<uint32_t>(quads_.size()));
}
