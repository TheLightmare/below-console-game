#pragma once

// =============================================================================
// Vulkan 2D Engine - Bitmap Font Text Renderer
// =============================================================================
// Renders text using a monospaced bitmap font texture atlas.
// Each glyph is a fixed-size cell within the atlas (16x16 grid of chars
// covering ASCII 0-255 is the common layout).
//
// The font texture should be a grid of glyph_w x glyph_h cells,
// arranged in 16 columns x 16 rows (covering ASCII 0..255).
//
// Text is rendered by queuing quads into a SpriteBatch.
// =============================================================================

#include "vk_types.hpp"
#include "vk_texture.hpp"

class VkContext;
class SpriteBatch;

class BitmapFont {
public:
    BitmapFont() = default;

    /// Load a bitmap font from an image file.
    /// `glyph_w` and `glyph_h` are the pixel dimensions of each character cell.
    /// The image must be a 16x16 grid (256 glyphs).
    bool load(VkContext& ctx, const char* filepath,
              uint32_t glyph_w, uint32_t glyph_h);

    /// Create a built-in minimal 8x8 monospace font (embedded bitmap).
    /// Useful for immediate debugging without requiring an external font file.
    void create_builtin(VkContext& ctx);

    /// Queue text to be drawn by a SpriteBatch.
    /// `scale` multiplies the glyph size.
    void draw_text(SpriteBatch& batch,
                   VkDescriptorSet font_texture_set,
                   float x, float y,
                   const std::string& text,
                   const Color4& color = Color4::white(),
                   float scale = 1.0f);

    /// Measure the pixel width of a string at the given scale.
    float measure_width(const std::string& text, float scale = 1.0f) const;

    /// Measure the pixel height of a single line at the given scale.
    float line_height(float scale = 1.0f) const;

    /// Access the underlying texture for descriptor set creation.
    VkTexture& texture() { return texture_; }

    uint32_t glyph_width()  const { return glyph_w_; }
    uint32_t glyph_height() const { return glyph_h_; }

    void destroy(VkContext& ctx);

private:
    VkTexture texture_;
    uint32_t  glyph_w_ = 0;
    uint32_t  glyph_h_ = 0;
    uint32_t  tex_w_   = 0; // Full texture width in pixels
    uint32_t  tex_h_   = 0; // Full texture height in pixels

    UVRect glyph_uv(char ch) const;
};
