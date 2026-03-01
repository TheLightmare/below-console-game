#pragma once

// =============================================================================
// Vulkan 2D Engine - Pipeline Management
// =============================================================================
// Handles shader loading (SPIR-V), graphics pipeline creation, descriptor
// set layouts, descriptor pools, and descriptor set allocation.
// =============================================================================

#include "vk_types.hpp"

class VkContext;

// ---------------------------------------------------------------------------
// Shader loading
// ---------------------------------------------------------------------------

/// Load a SPIR-V binary from disk and create a VkShaderModule.
VkShaderModule load_shader_module(VkDevice device, const char* filepath);

// ---------------------------------------------------------------------------
// Pipeline builder (fluent API)
// ---------------------------------------------------------------------------

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo>  shader_stages;
    VkPipelineVertexInputStateCreateInfo           vertex_input{};
    VkPipelineInputAssemblyStateCreateInfo         input_assembly{};
    VkPipelineRasterizationStateCreateInfo         rasteriser{};
    VkPipelineMultisampleStateCreateInfo           multisampling{};
    VkPipelineColorBlendAttachmentState            color_blend_attachment{};
    VkPipelineDepthStencilStateCreateInfo          depth_stencil{};
    VkPipelineLayout                               pipeline_layout = VK_NULL_HANDLE;

    PipelineBuilder();

    /// Set vertex + fragment shader modules.
    PipelineBuilder& set_shaders(VkShaderModule vert, VkShaderModule frag);

    /// Configure vertex input for Vertex2D.
    PipelineBuilder& set_vertex_input_2d();

    /// Set topology (default: triangle list).
    PipelineBuilder& set_topology(VkPrimitiveTopology topo);

    /// Enable standard alpha blending (src_alpha, one_minus_src_alpha).
    PipelineBuilder& enable_alpha_blending();

    /// Set pipeline layout.
    PipelineBuilder& set_layout(VkPipelineLayout layout);

    /// Build the final graphics pipeline.
    VkPipeline build(VkDevice device, VkRenderPass render_pass);
};

// ---------------------------------------------------------------------------
// Descriptor helpers
// ---------------------------------------------------------------------------

/// Create a descriptor set layout with a single UBO binding (set 0, binding 0).
VkDescriptorSetLayout create_camera_descriptor_layout(VkDevice device);

/// Create a descriptor set layout with a single combined image sampler (set 1, binding 0).
VkDescriptorSetLayout create_texture_descriptor_layout(VkDevice device);

/// Create a descriptor pool that can allocate `max_sets` sets with the given pool sizes.
VkDescriptorPool create_descriptor_pool(VkDevice device, uint32_t max_sets,
                                        const std::vector<VkDescriptorPoolSize>& sizes);

/// Allocate one descriptor set from a pool with the given layout.
VkDescriptorSet allocate_descriptor_set(VkDevice device, VkDescriptorPool pool,
                                        VkDescriptorSetLayout layout);

/// Write a UBO to a descriptor set (binding 0).
void write_ubo_descriptor(VkDevice device, VkDescriptorSet set,
                          VkBuffer buffer, VkDeviceSize size);

/// Write a combined image sampler to a descriptor set (binding 0).
void write_texture_descriptor(VkDevice device, VkDescriptorSet set,
                              VkImageView view, VkSampler sampler);
