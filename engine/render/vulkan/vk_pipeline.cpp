#include "vk_pipeline.hpp"
#include "vk_context.hpp"
#include <fstream>

// =============================================================================
// Shader module loading
// =============================================================================

VkShaderModule load_shader_module(VkDevice device, const char* filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        fprintf(stderr, "[Pipeline] Failed to open shader file: %s\n", filepath);
        return VK_NULL_HANDLE;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> code(file_size / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(file_size));
    file.close();

    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = file_size;
    ci.pCode    = code.data();

    VkShaderModule module;
    VK_CHECK(vkCreateShaderModule(device, &ci, nullptr, &module));
    return module;
}

// =============================================================================
// Pipeline builder
// =============================================================================

PipelineBuilder::PipelineBuilder() {
    // Sensible defaults for 2D rendering
    vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    input_assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    rasteriser = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasteriser.polygonMode = VK_POLYGON_MODE_FILL;
    rasteriser.lineWidth   = 1.0f;
    rasteriser.cullMode    = VK_CULL_MODE_NONE; // No culling for 2D
    rasteriser.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Default: no blending (overwrite)
    color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    depth_stencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth_stencil.depthTestEnable  = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;
}

PipelineBuilder& PipelineBuilder::set_shaders(VkShaderModule vert, VkShaderModule frag) {
    shader_stages.clear();

    VkPipelineShaderStageCreateInfo vert_stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    vert_stage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage.module = vert;
    vert_stage.pName  = "main";
    shader_stages.push_back(vert_stage);

    VkPipelineShaderStageCreateInfo frag_stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    frag_stage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage.module = frag;
    frag_stage.pName  = "main";
    shader_stages.push_back(frag_stage);

    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_input_2d() {
    static auto binding = Vertex2D::binding_description();
    static auto attribs = Vertex2D::attribute_descriptions();

    vertex_input.vertexBindingDescriptionCount   = 1;
    vertex_input.pVertexBindingDescriptions       = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
    vertex_input.pVertexAttributeDescriptions     = attribs.data();

    return *this;
}

PipelineBuilder& PipelineBuilder::set_topology(VkPrimitiveTopology topo) {
    input_assembly.topology = topo;
    return *this;
}

PipelineBuilder& PipelineBuilder::enable_alpha_blending() {
    color_blend_attachment.blendEnable         = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_layout(VkPipelineLayout layout) {
    pipeline_layout = layout;
    return *this;
}

VkPipeline PipelineBuilder::build(VkDevice device, VkRenderPass render_pass) {
    // Viewport and scissor are dynamic
    VkPipelineViewportStateCreateInfo viewport_state{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount  = 1;

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    VkPipelineColorBlendStateCreateInfo color_blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend.attachmentCount = 1;
    color_blend.pAttachments    = &color_blend_attachment;

    VkGraphicsPipelineCreateInfo ci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    ci.stageCount          = static_cast<uint32_t>(shader_stages.size());
    ci.pStages             = shader_stages.data();
    ci.pVertexInputState   = &vertex_input;
    ci.pInputAssemblyState = &input_assembly;
    ci.pViewportState      = &viewport_state;
    ci.pRasterizationState = &rasteriser;
    ci.pMultisampleState   = &multisampling;
    ci.pDepthStencilState  = &depth_stencil;
    ci.pColorBlendState    = &color_blend;
    ci.pDynamicState       = &dynamic_state;
    ci.layout              = pipeline_layout;
    ci.renderPass          = render_pass;
    ci.subpass             = 0;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline));
    return pipeline;
}

// =============================================================================
// Descriptor helpers
// =============================================================================

VkDescriptorSetLayout create_camera_descriptor_layout(VkDevice device) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    ci.bindingCount = 1;
    ci.pBindings    = &binding;

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &ci, nullptr, &layout));
    return layout;
}

VkDescriptorSetLayout create_texture_descriptor_layout(VkDevice device) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    ci.bindingCount = 1;
    ci.pBindings    = &binding;

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &ci, nullptr, &layout));
    return layout;
}

VkDescriptorPool create_descriptor_pool(VkDevice device, uint32_t max_sets,
                                        const std::vector<VkDescriptorPoolSize>& sizes) {
    VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    ci.maxSets       = max_sets;
    ci.poolSizeCount = static_cast<uint32_t>(sizes.size());
    ci.pPoolSizes    = sizes.data();

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(device, &ci, nullptr, &pool));
    return pool;
}

VkDescriptorSet allocate_descriptor_set(VkDevice device, VkDescriptorPool pool,
                                        VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo alloc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    alloc.descriptorPool     = pool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts        = &layout;

    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(device, &alloc, &set));
    return set;
}

void write_ubo_descriptor(VkDevice device, VkDescriptorSet set,
                          VkBuffer buffer, VkDeviceSize size) {
    VkDescriptorBufferInfo buf_info{};
    buf_info.buffer = buffer;
    buf_info.offset = 0;
    buf_info.range  = size;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet          = set;
    write.dstBinding      = 0;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo     = &buf_info;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void write_texture_descriptor(VkDevice device, VkDescriptorSet set,
                              VkImageView view, VkSampler sampler) {
    VkDescriptorImageInfo img_info{};
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img_info.imageView   = view;
    img_info.sampler     = sampler;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet          = set;
    write.dstBinding      = 0;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo      = &img_info;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}
