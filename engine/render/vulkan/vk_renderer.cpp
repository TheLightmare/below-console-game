#include "vk_renderer.hpp"

// =============================================================================
// Init / Destroy
// =============================================================================

void VkRenderer::init(const char* title, int width, int height,
                       bool fullscreen, bool validation) {
    window_.init(title, width, height, fullscreen);
    ctx_.init(window_, title, validation);

    camera_.set_viewport(static_cast<float>(ctx_.swapchain_extent.width),
                         static_cast<float>(ctx_.swapchain_extent.height));

    create_descriptors();
    create_pipeline();
    create_default_resources();

    batch_.init(ctx_);
    running_ = true;
}

void VkRenderer::destroy() {
    ctx_.wait_idle();

    // Loaded textures
    for (auto& tex : loaded_textures_) tex.destroy(ctx_);
    loaded_textures_.clear();

    font_.destroy(ctx_);
    white_tex_.destroy(ctx_);
    batch_.destroy(ctx_);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkbuf::destroy_buffer(ctx_.allocator, camera_ubos_[i]);
    }

    vkDestroyPipeline(ctx_.device, sprite_pipeline_, nullptr);
    vkDestroyPipelineLayout(ctx_.device, pipeline_layout_, nullptr);
    vkDestroyDescriptorPool(ctx_.device, descriptor_pool_, nullptr);
    vkDestroyDescriptorSetLayout(ctx_.device, camera_layout_, nullptr);
    vkDestroyDescriptorSetLayout(ctx_.device, texture_layout_, nullptr);

    window_.destroy(ctx_.instance);
    ctx_.destroy();

    running_ = false;
}

// =============================================================================
// Per-frame
// =============================================================================

Key VkRenderer::poll_input() {
    bool quit = false;
    bool resized = false;
    Key key = VkInput::poll_events(quit, resized);

    if (quit) running_ = false;
    if (resized) window_.handle_resize();

    return key;
}

void VkRenderer::begin_frame(float cr, float cg, float cb) {
    // Handle resize
    if (window_.was_resized()) {
        ctx_.recreate_swapchain(window_);
        camera_.set_viewport(static_cast<float>(ctx_.swapchain_extent.width),
                             static_cast<float>(ctx_.swapchain_extent.height));
    }

    if (!ctx_.begin_frame()) {
        ctx_.recreate_swapchain(window_);
        camera_.set_viewport(static_cast<float>(ctx_.swapchain_extent.width),
                             static_cast<float>(ctx_.swapchain_extent.height));
        ctx_.begin_frame();
    }

    // Update camera UBO
    CameraUBO ubo = camera_.build_ubo();
    memcpy(camera_mapped_[ctx_.current_frame], &ubo, sizeof(CameraUBO));

    ctx_.begin_render_pass(cr, cg, cb);
    batch_.begin();
}

void VkRenderer::draw_sprite(VkDescriptorSet texture_set,
                               const Rect2D& dest,
                               const UVRect& uv,
                               const Color4& tint) {
    batch_.draw(texture_set, dest, uv, tint);
}

void VkRenderer::draw_rect(const Rect2D& dest, const Color4& color) {
    batch_.draw_rect(white_tex_set_, dest, color);
}

void VkRenderer::draw_text(float x, float y, const std::string& text,
                             const Color4& color, float scale) {
    font_.draw_text(batch_, font_tex_set_, x, y, text, color, scale);
}

void VkRenderer::end_frame() {
    batch_.end(ctx_.current_cmd(), camera_sets_[ctx_.current_frame],
               sprite_pipeline_, pipeline_layout_);

    ctx_.end_render_pass();

    if (!ctx_.end_frame()) {
        // Swapchain out of date -- will be recreated next begin_frame()
    }
}

// =============================================================================
// Resource loading
// =============================================================================

VkDescriptorSet VkRenderer::load_texture(const char* filepath, bool filtering) {
    loaded_textures_.emplace_back();
    VkTexture& tex = loaded_textures_.back();

    if (!tex.load_from_file(ctx_, filepath, filtering)) {
        loaded_textures_.pop_back();
        return VK_NULL_HANDLE;
    }

    VkDescriptorSet set = allocate_descriptor_set(ctx_.device, descriptor_pool_, texture_layout_);
    write_texture_descriptor(ctx_.device, set, tex.image.view, tex.sampler);
    return set;
}

VkDescriptorSet VkRenderer::load_texture_from_pixels(const uint8_t* pixels,
                                                       uint32_t w, uint32_t h,
                                                       bool filtering) {
    loaded_textures_.emplace_back();
    VkTexture& tex = loaded_textures_.back();
    tex.create_from_pixels(ctx_, pixels, w, h, filtering);

    VkDescriptorSet set = allocate_descriptor_set(ctx_.device, descriptor_pool_, texture_layout_);
    write_texture_descriptor(ctx_.device, set, tex.image.view, tex.sampler);
    return set;
}

// =============================================================================
// Internal setup
// =============================================================================

void VkRenderer::create_descriptors() {
    camera_layout_  = create_camera_descriptor_layout(ctx_.device);
    texture_layout_ = create_texture_descriptor_layout(ctx_.device);

    // Pool: enough for camera sets + a generous number of texture sets
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         MAX_FRAMES_IN_FLIGHT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256 },
    };
    descriptor_pool_ = create_descriptor_pool(ctx_.device,
                                               MAX_FRAMES_IN_FLIGHT + 256,
                                               pool_sizes);

    // Per-frame camera UBO + descriptor set
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        camera_ubos_[i] = vkbuf::create_mapped_buffer(
            ctx_.allocator, sizeof(CameraUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            &camera_mapped_[i]);

        camera_sets_[i] = allocate_descriptor_set(ctx_.device, descriptor_pool_, camera_layout_);
        write_ubo_descriptor(ctx_.device, camera_sets_[i],
                             camera_ubos_[i].buffer, sizeof(CameraUBO));
    }
}

void VkRenderer::create_pipeline() {
    // Pipeline layout: set 0 = camera UBO, set 1 = texture sampler
    VkDescriptorSetLayout layouts[] = { camera_layout_, texture_layout_ };

    VkPipelineLayoutCreateInfo layout_ci{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_ci.setLayoutCount = 2;
    layout_ci.pSetLayouts    = layouts;
    VK_CHECK(vkCreatePipelineLayout(ctx_.device, &layout_ci, nullptr, &pipeline_layout_));

    // Load shaders
    VkShaderModule vert = load_shader_module(ctx_.device, "shaders/sprite.vert.spv");
    VkShaderModule frag = load_shader_module(ctx_.device, "shaders/sprite.frag.spv");

    assert(vert != VK_NULL_HANDLE && "Failed to load sprite vertex shader");
    assert(frag != VK_NULL_HANDLE && "Failed to load sprite fragment shader");

    sprite_pipeline_ = PipelineBuilder()
        .set_shaders(vert, frag)
        .set_vertex_input_2d()
        .enable_alpha_blending()
        .set_layout(pipeline_layout_)
        .build(ctx_.device, ctx_.render_pass);

    vkDestroyShaderModule(ctx_.device, vert, nullptr);
    vkDestroyShaderModule(ctx_.device, frag, nullptr);
}

void VkRenderer::create_default_resources() {
    // 1x1 white pixel texture (for untextured quads)
    white_tex_.create_solid_color(ctx_, 255, 255, 255, 255);
    white_tex_set_ = allocate_descriptor_set(ctx_.device, descriptor_pool_, texture_layout_);
    write_texture_descriptor(ctx_.device, white_tex_set_,
                             white_tex_.image.view, white_tex_.sampler);

    // Built-in bitmap font
    font_.create_builtin(ctx_);
    font_tex_set_ = allocate_descriptor_set(ctx_.device, descriptor_pool_, texture_layout_);
    write_texture_descriptor(ctx_.device, font_tex_set_,
                             font_.texture().image.view, font_.texture().sampler);
}
