#include "vk_texture.hpp"
#include "vk_context.hpp"
#include "vk_buffer.hpp"

#include <stb_image.h>

// =============================================================================
// VkTexture
// =============================================================================

void VkTexture::create_from_pixels(VkContext& ctx, const uint8_t* pixels,
                                    uint32_t width, uint32_t height,
                                    bool enable_filtering) {
    VkDeviceSize image_size = static_cast<VkDeviceSize>(width) * height * 4;

    // Create staging buffer and copy pixels into it
    AllocatedBuffer staging = vkbuf::create_staging_buffer(ctx.allocator, image_size);
    void* mapped = nullptr;
    VK_CHECK(vmaMapMemory(ctx.allocator, staging.allocation, &mapped));
    memcpy(mapped, pixels, static_cast<size_t>(image_size));
    vmaUnmapMemory(ctx.allocator, staging.allocation);

    // Create image
    VkImageCreateInfo img_ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    img_ci.imageType     = VK_IMAGE_TYPE_2D;
    img_ci.format        = VK_FORMAT_R8G8B8A8_UNORM;
    img_ci.extent        = { width, height, 1 };
    img_ci.mipLevels     = 1;
    img_ci.arrayLayers   = 1;
    img_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    img_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    img_ci.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_ci{};
    alloc_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(ctx.allocator, &img_ci, &alloc_ci,
                            &image.image, &image.allocation, nullptr));
    image.width  = width;
    image.height = height;
    image.format = VK_FORMAT_R8G8B8A8_UNORM;

    // Transition to TRANSFER_DST, copy, then transition to SHADER_READ_ONLY
    transition_layout(ctx, image.image,
                      VK_IMAGE_LAYOUT_UNDEFINED,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkCommandBuffer cmd = ctx.begin_single_time_commands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent                 = { width, height, 1 };
    vkCmdCopyBufferToImage(cmd, staging.buffer, image.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    ctx.end_single_time_commands(cmd);

    transition_layout(ctx, image.image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkbuf::destroy_buffer(ctx.allocator, staging);

    // Image view
    VkImageViewCreateInfo view_ci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_ci.image    = image.image;
    view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format   = image.format;
    view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_ci.subresourceRange.levelCount = 1;
    view_ci.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(ctx.device, &view_ci, nullptr, &image.view));

    // Sampler
    create_sampler(ctx.device, enable_filtering);
}

bool VkTexture::load_from_file(VkContext& ctx, const char* filepath,
                                bool enable_filtering) {
    int w, h, channels;
    stbi_uc* pixels = stbi_load(filepath, &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels) {
        fprintf(stderr, "[VkTexture] Failed to load: %s\n", filepath);
        return false;
    }

    create_from_pixels(ctx, pixels, static_cast<uint32_t>(w), static_cast<uint32_t>(h),
                       enable_filtering);
    stbi_image_free(pixels);
    return true;
}

void VkTexture::create_solid_color(VkContext& ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t pixel[4] = { r, g, b, a };
    create_from_pixels(ctx, pixel, 1, 1, false);
}

void VkTexture::create_sampler(VkDevice device, bool enable_filtering) {
    VkSamplerCreateInfo ci{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    ci.magFilter    = enable_filtering ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    ci.minFilter    = enable_filtering ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.maxLod       = 1.0f;

    VK_CHECK(vkCreateSampler(device, &ci, nullptr, &sampler));
}

void VkTexture::transition_layout(VkContext& ctx, VkImage img,
                                   VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer cmd = ctx.begin_single_time_commands();

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout           = old_layout;
    barrier.newLayout           = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = img;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage, dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        fprintf(stderr, "[VkTexture] Unsupported layout transition\n");
        ctx.end_single_time_commands(cmd);
        return;
    }

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    ctx.end_single_time_commands(cmd);
}

void VkTexture::destroy(VkContext& ctx) {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(ctx.device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (image.view != VK_NULL_HANDLE) {
        vkDestroyImageView(ctx.device, image.view, nullptr);
        image.view = VK_NULL_HANDLE;
    }
    if (image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(ctx.allocator, image.image, image.allocation);
        image.image      = VK_NULL_HANDLE;
        image.allocation = VK_NULL_HANDLE;
    }
}

// =============================================================================
// Texture Atlas
// =============================================================================

void TextureAtlas::add_region(const std::string& name, uint32_t w, uint32_t h,
                               const uint8_t* pixels) {
    assert(!built_ && "Cannot add regions after build()");
    pending_.push_back({ name, w, h, pixels });
}

void TextureAtlas::build(VkContext& ctx, bool enable_filtering) {
    if (pending_.empty()) return;

    // Sort by height (descending) for simple row packing
    std::sort(pending_.begin(), pending_.end(),
              [](const PendingRegion& a, const PendingRegion& b) {
                  return a.h > b.h;
              });

    // Calculate atlas dimensions (simple row-based packing)
    // Use rows of sprites, advancing Y by the tallest sprite in each row.
    const uint32_t max_width = 4096;
    uint32_t cur_x = 0, cur_y = 0, row_height = 0;
    atlas_w_ = 0;
    atlas_h_ = 0;

    // First pass: compute positions
    struct Placement { uint32_t x, y; };
    std::vector<Placement> placements(pending_.size());

    for (size_t i = 0; i < pending_.size(); ++i) {
        auto& p = pending_[i];
        if (cur_x + p.w > max_width) {
            cur_y += row_height;
            cur_x = 0;
            row_height = 0;
        }
        placements[i] = { cur_x, cur_y };
        cur_x += p.w;
        if (p.h > row_height) row_height = p.h;
        if (cur_x > atlas_w_) atlas_w_ = cur_x;
    }
    atlas_h_ = cur_y + row_height;

    // Round up to power of 2 (not strictly required but GPUs like it)
    auto next_pow2 = [](uint32_t v) -> uint32_t {
        v--;
        v |= v >> 1; v |= v >> 2; v |= v >> 4;
        v |= v >> 8; v |= v >> 16;
        return v + 1;
    };
    atlas_w_ = next_pow2(atlas_w_);
    atlas_h_ = next_pow2(atlas_h_);

    // Build CPU-side RGBA atlas
    std::vector<uint8_t> atlas_pixels(atlas_w_ * atlas_h_ * 4, 0);

    for (size_t i = 0; i < pending_.size(); ++i) {
        auto& p   = pending_[i];
        auto& pos = placements[i];

        // Copy rows
        for (uint32_t row = 0; row < p.h; ++row) {
            uint32_t dst_offset = ((pos.y + row) * atlas_w_ + pos.x) * 4;
            uint32_t src_offset = row * p.w * 4;
            memcpy(&atlas_pixels[dst_offset], &p.pixels[src_offset], p.w * 4);
        }

        // Store region info
        AtlasRegion region;
        region.name = p.name;
        region.x    = pos.x;
        region.y    = pos.y;
        region.w    = p.w;
        region.h    = p.h;
        region.uv.u0 = static_cast<float>(pos.x) / atlas_w_;
        region.uv.v0 = static_cast<float>(pos.y) / atlas_h_;
        region.uv.u1 = static_cast<float>(pos.x + p.w) / atlas_w_;
        region.uv.v1 = static_cast<float>(pos.y + p.h) / atlas_h_;
        regions_[p.name] = region;
    }

    // Upload to GPU
    texture_.create_from_pixels(ctx, atlas_pixels.data(), atlas_w_, atlas_h_,
                                enable_filtering);

    pending_.clear();
    built_ = true;
}

const AtlasRegion* TextureAtlas::get_region(const std::string& name) const {
    auto it = regions_.find(name);
    return it != regions_.end() ? &it->second : nullptr;
}

UVRect TextureAtlas::get_uv(const std::string& name) const {
    auto it = regions_.find(name);
    return it != regions_.end() ? it->second.uv : UVRect::full();
}

void TextureAtlas::destroy(VkContext& ctx) {
    texture_.destroy(ctx);
    regions_.clear();
    pending_.clear();
    built_ = false;
}
