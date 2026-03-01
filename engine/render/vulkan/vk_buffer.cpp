#include "vk_buffer.hpp"
#include "vk_context.hpp"

namespace vkbuf {

// ---------------------------------------------------------------------------
// GPU-local buffer
// ---------------------------------------------------------------------------

AllocatedBuffer create_gpu_buffer(VmaAllocator allocator, VkDeviceSize size,
                                  VkBufferUsageFlags usage) {
    VkBufferCreateInfo buf_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_info.size  = size;
    buf_info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    AllocatedBuffer result{};
    result.size = size;
    VK_CHECK(vmaCreateBuffer(allocator, &buf_info, &alloc_info,
                             &result.buffer, &result.allocation, nullptr));
    return result;
}

// ---------------------------------------------------------------------------
// Host-visible mapped buffer
// ---------------------------------------------------------------------------

AllocatedBuffer create_mapped_buffer(VmaAllocator allocator, VkDeviceSize size,
                                     VkBufferUsageFlags usage, void** mapped) {
    VkBufferCreateInfo buf_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_info.size  = size;
    buf_info.usage = usage;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo info{};
    AllocatedBuffer result{};
    result.size = size;
    VK_CHECK(vmaCreateBuffer(allocator, &buf_info, &alloc_info,
                             &result.buffer, &result.allocation, &info));

    if (mapped) *mapped = info.pMappedData;
    return result;
}

// ---------------------------------------------------------------------------
// Staging buffer
// ---------------------------------------------------------------------------

AllocatedBuffer create_staging_buffer(VmaAllocator allocator, VkDeviceSize size) {
    VkBufferCreateInfo buf_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_info.size  = size;
    buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    AllocatedBuffer result{};
    result.size = size;
    VK_CHECK(vmaCreateBuffer(allocator, &buf_info, &alloc_info,
                             &result.buffer, &result.allocation, nullptr));
    return result;
}

// ---------------------------------------------------------------------------
// Upload via staging
// ---------------------------------------------------------------------------

void upload_buffer(VkContext& ctx, AllocatedBuffer& dst, const void* data, VkDeviceSize size) {
    AllocatedBuffer staging = create_staging_buffer(ctx.allocator, size);

    // Map, copy, unmap
    void* mapped = nullptr;
    VK_CHECK(vmaMapMemory(ctx.allocator, staging.allocation, &mapped));
    memcpy(mapped, data, static_cast<size_t>(size));
    vmaUnmapMemory(ctx.allocator, staging.allocation);

    // Copy staging → dst
    VkCommandBuffer cmd = ctx.begin_single_time_commands();

    VkBufferCopy region{};
    region.size = size;
    vkCmdCopyBuffer(cmd, staging.buffer, dst.buffer, 1, &region);

    ctx.end_single_time_commands(cmd);

    // Destroy staging buffer
    destroy_buffer(ctx.allocator, staging);
}

// ---------------------------------------------------------------------------
// Destruction
// ---------------------------------------------------------------------------

void destroy_buffer(VmaAllocator allocator, AllocatedBuffer& buf) {
    if (buf.buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, buf.buffer, buf.allocation);
        buf.buffer     = VK_NULL_HANDLE;
        buf.allocation = VK_NULL_HANDLE;
        buf.size       = 0;
    }
}

// ---------------------------------------------------------------------------
// Quad index generation
// ---------------------------------------------------------------------------

std::vector<uint16_t> generate_quad_indices(uint32_t quad_count) {
    std::vector<uint16_t> indices(quad_count * 6);
    for (uint32_t i = 0; i < quad_count; ++i) {
        uint16_t base = static_cast<uint16_t>(i * 4);
        uint32_t idx  = i * 6;
        indices[idx + 0] = base + 0;
        indices[idx + 1] = base + 1;
        indices[idx + 2] = base + 2;
        indices[idx + 3] = base + 2;
        indices[idx + 4] = base + 3;
        indices[idx + 5] = base + 0;
    }
    return indices;
}

AllocatedBuffer create_quad_index_buffer(VkContext& ctx, uint32_t quad_count) {
    auto indices = generate_quad_indices(quad_count);
    VkDeviceSize size = indices.size() * sizeof(uint16_t);

    AllocatedBuffer ibo = create_gpu_buffer(ctx.allocator, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    upload_buffer(ctx, ibo, indices.data(), size);
    return ibo;
}

} // namespace vkbuf
