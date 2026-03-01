#pragma once

// =============================================================================
// Vulkan 2D Engine - Texture & Texture Atlas
// =============================================================================
// Loads images from disk (via stb_image), uploads to GPU, and provides a
// simple atlas system that packs sprites into a single large texture for
// efficient batched rendering.
// =============================================================================

#include "vk_types.hpp"
#include <unordered_map>

class VkContext;

// ---------------------------------------------------------------------------
// Single texture
// ---------------------------------------------------------------------------

class VkTexture {
public:
    AllocatedImage image{};
    VkSampler      sampler = VK_NULL_HANDLE;

    VkTexture() = default;

    /// Load from RGBA pixel data already in memory.
    void create_from_pixels(VkContext& ctx, const uint8_t* pixels,
                            uint32_t width, uint32_t height,
                            bool enable_filtering = true);

    /// Load from an image file (PNG, BMP, TGA, JPG via stb_image).
    bool load_from_file(VkContext& ctx, const char* filepath,
                        bool enable_filtering = true);

    /// Create a 1x1 single-color texture (useful as white pixel for untextured quads).
    void create_solid_color(VkContext& ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    void destroy(VkContext& ctx);

    uint32_t width()  const { return image.width; }
    uint32_t height() const { return image.height; }

private:
    void create_sampler(VkDevice device, bool enable_filtering);
    void transition_layout(VkContext& ctx, VkImage img,
                           VkImageLayout old_layout, VkImageLayout new_layout);
};

// ---------------------------------------------------------------------------
// Texture Atlas
// ---------------------------------------------------------------------------

/// A sub-region within an atlas, identified by name.
struct AtlasRegion {
    std::string name;
    uint32_t    x, y, w, h;  // Pixel coordinates within atlas
    UVRect      uv;           // Pre-computed normalised UVs
};

/// Simple row-based texture atlas packer.
/// 1. Call add_region() for each sprite before building.
/// 2. Call build() to upload. After that, use get_region() to look up UVs.
class TextureAtlas {
public:
    TextureAtlas() = default;

    /// Reserve a region of the given pixel size. Data will be copied in build().
    /// `pixels` must be RGBA and stay valid until build() is called.
    void add_region(const std::string& name, uint32_t w, uint32_t h, const uint8_t* pixels);

    /// Pack all added regions into a single GPU texture. After this, add_region
    /// may not be called again.
    void build(VkContext& ctx, bool enable_filtering = false);

    /// Look up a region by name.
    const AtlasRegion* get_region(const std::string& name) const;

    /// Look up UVs by name (convenience).
    UVRect get_uv(const std::string& name) const;

    /// Access the underlying GPU texture (for descriptor binding).
    const VkTexture& texture() const { return texture_; }

    void destroy(VkContext& ctx);

private:
    struct PendingRegion {
        std::string          name;
        uint32_t             w, h;
        const uint8_t*       pixels;
    };

    std::vector<PendingRegion>                    pending_;
    std::unordered_map<std::string, AtlasRegion>  regions_;
    VkTexture                                     texture_;
    uint32_t                                      atlas_w_ = 0;
    uint32_t                                      atlas_h_ = 0;
    bool                                          built_   = false;
};
