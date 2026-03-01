#pragma once

// =============================================================================
// Vulkan 2D Engine - Buffer Helpers (VMA-backed)
// =============================================================================
// Convenience functions for creating vertex, index, uniform, and staging
// buffers via VMA. Also handles buffer uploads through staging.
// =============================================================================

#include "vk_types.hpp"

class VkContext;

namespace vkbuf {

// ---------------------------------------------------------------------------
// Buffer creation
// ---------------------------------------------------------------------------

/// Create a GPU-local buffer (not host-visible). Use staging to upload data.
AllocatedBuffer create_gpu_buffer(VmaAllocator allocator, VkDeviceSize size,
                                  VkBufferUsageFlags usage);

/// Create a host-visible, host-coherent buffer (for dynamic data like UBOs).
/// Returns the buffer already mapped; store the mapped pointer yourself.
AllocatedBuffer create_mapped_buffer(VmaAllocator allocator, VkDeviceSize size,
                                     VkBufferUsageFlags usage, void** mapped);

/// Create a staging buffer (host-visible, transfer-src).
AllocatedBuffer create_staging_buffer(VmaAllocator allocator, VkDeviceSize size);

// ---------------------------------------------------------------------------
// Upload
// ---------------------------------------------------------------------------

/// Upload data to a GPU-local buffer via a staging transfer.
void upload_buffer(VkContext& ctx, AllocatedBuffer& dst, const void* data, VkDeviceSize size);

// ---------------------------------------------------------------------------
// Destruction
// ---------------------------------------------------------------------------

void destroy_buffer(VmaAllocator allocator, AllocatedBuffer& buf);

// ---------------------------------------------------------------------------
// Index buffer helpers
// ---------------------------------------------------------------------------

/// Generate standard quad indices (0,1,2, 2,3,0, 4,5,6, 6,7,4, ...)
/// for `quad_count` quads. Returns a vector of uint16_t.
std::vector<uint16_t> generate_quad_indices(uint32_t quad_count);

/// Create and upload a pre-built quad index buffer for `quad_count` quads.
AllocatedBuffer create_quad_index_buffer(VkContext& ctx, uint32_t quad_count);

} // namespace vkbuf
