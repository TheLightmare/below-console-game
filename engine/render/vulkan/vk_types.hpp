#pragma once

// =============================================================================
// Vulkan 2D Engine - Common Types & Includes
// =============================================================================
// Central header for Vulkan types, macros, and shared structures.
// All vulkan/*.hpp files should include this rather than <vulkan/vulkan.h> directly.
// =============================================================================

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <cassert>

// ---------------------------------------------------------------------------
// Error checking
// ---------------------------------------------------------------------------

#define VK_CHECK(result)                                                        \
    do {                                                                        \
        VkResult _r = (result);                                                 \
        if (_r != VK_SUCCESS) {                                                 \
            fprintf(stderr, "[Vulkan Error] %s:%d  VkResult = %d\n",            \
                    __FILE__, __LINE__, static_cast<int>(_r));                   \
            abort();                                                            \
        }                                                                       \
    } while (0)

// ---------------------------------------------------------------------------
// Vertex format for 2D sprites / quads
// ---------------------------------------------------------------------------

struct Vertex2D {
    float x, y;       // World-space position
    float u, v;       // Texture coordinates
    float r, g, b, a; // Color tint (multiplied with texture in frag shader)

    static VkVertexInputBindingDescription binding_description() {
        return { 0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX };
    }

    static std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions() {
        return {{
            { 0, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex2D, x) },  // position
            { 1, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex2D, u) },  // texcoord
            { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex2D, r) },  // color
        }};
    }
};

// ---------------------------------------------------------------------------
// GPU-side camera uniform (orthographic projection * view)
// ---------------------------------------------------------------------------

struct CameraUBO {
    float mvp[16]; // Column-major 4x4 matrix
};

// ---------------------------------------------------------------------------
// Allocated buffer (VMA-backed)
// ---------------------------------------------------------------------------

struct AllocatedBuffer {
    VkBuffer       buffer     = VK_NULL_HANDLE;
    VmaAllocation  allocation = VK_NULL_HANDLE;
    VkDeviceSize   size       = 0;
};

// ---------------------------------------------------------------------------
// Allocated image (VMA-backed)
// ---------------------------------------------------------------------------

struct AllocatedImage {
    VkImage        image      = VK_NULL_HANDLE;
    VmaAllocation  allocation = VK_NULL_HANDLE;
    VkImageView    view       = VK_NULL_HANDLE;
    uint32_t       width      = 0;
    uint32_t       height     = 0;
    VkFormat       format     = VK_FORMAT_R8G8B8A8_UNORM;
};

// ---------------------------------------------------------------------------
// Rectangle helpers
// ---------------------------------------------------------------------------

struct Rect2D {
    float x, y, w, h;
};

struct UVRect {
    float u0, v0, u1, v1;

    static UVRect full() { return { 0.0f, 0.0f, 1.0f, 1.0f }; }
};

struct Color4 {
    float r, g, b, a;

    static Color4 white()  { return { 1, 1, 1, 1 }; }
    static Color4 black()  { return { 0, 0, 0, 1 }; }
    static Color4 red()    { return { 1, 0, 0, 1 }; }
    static Color4 green()  { return { 0, 1, 0, 1 }; }
    static Color4 blue()   { return { 0, 0, 1, 1 }; }
    static Color4 yellow() { return { 1, 1, 0, 1 }; }
    static Color4 cyan()   { return { 0, 1, 1, 1 }; }

    static Color4 from_rgb(unsigned char rv, unsigned char gv, unsigned char bv, unsigned char av = 255) {
        return { rv / 255.0f, gv / 255.0f, bv / 255.0f, av / 255.0f };
    }

    static Color4 from_hex(uint32_t hex) {
        return from_rgb(
            (hex >> 16) & 0xFF,
            (hex >> 8)  & 0xFF,
            (hex)       & 0xFF
        );
    }
};

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t MAX_QUADS_PER_BATCH  = 10000;
constexpr uint32_t MAX_VERTICES         = MAX_QUADS_PER_BATCH * 4;
constexpr uint32_t MAX_INDICES          = MAX_QUADS_PER_BATCH * 6;
