#include "vk_context.hpp"
#include "vk_window.hpp"

// =============================================================================
// Debug messenger callback
// =============================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT        /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user*/)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "[Vulkan Validation] %s\n", data->pMessage);
    }
    return VK_FALSE;
}

static VkResult create_debug_messenger(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* info,
    VkDebugUtilsMessengerEXT* messenger)
{
    auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    return fn ? fn(instance, info, nullptr, messenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
    auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (fn) fn(instance, messenger, nullptr);
}

// =============================================================================
// Public API
// =============================================================================

void VkContext::init(VkWindow& window, const char* app_name, bool enable_validation) {
    validation_enabled_ = enable_validation;

    create_instance(window, app_name, enable_validation);
    if (enable_validation) setup_debug_messenger();
    window.init_surface(instance);
    pick_physical_device(window.surface());
    create_logical_device(window.surface());
    create_allocator();
    create_swapchain(window);
    create_swapchain_views();
    create_render_pass();
    create_framebuffers();
    create_sync_objects();
}

void VkContext::recreate_swapchain(VkWindow& window) {
    // Handle minimisation: wait until window has non-zero extent
    int w = 0, h = 0;
    SDL_GetWindowSize(window.handle(), &w, &h);
    while (w == 0 || h == 0) {
        SDL_WaitEvent(nullptr);
        SDL_GetWindowSize(window.handle(), &w, &h);
    }

    wait_idle();
    cleanup_swapchain();

    create_swapchain(window);
    create_swapchain_views();
    create_framebuffers();

    // Recreate per-swapchain-image semaphores and image fence tracking
    uint32_t img_count = static_cast<uint32_t>(swapchain_images.size());
    render_finished_semaphores.resize(img_count);
    for (uint32_t i = 0; i < img_count; ++i) {
        VkSemaphoreCreateInfo sem_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(device, &sem_info, nullptr, &render_finished_semaphores[i]));
    }
    images_in_flight.resize(img_count, VK_NULL_HANDLE);

    window.reset_resized();
}

bool VkContext::begin_frame() {
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
        device, swapchain, UINT64_MAX,
        image_available_semaphores[current_frame],
        VK_NULL_HANDLE,
        &current_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) return false;
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "[VkContext] Failed to acquire swapchain image\n");
        return false;
    }

    // If another in-flight frame is still rendering to this image, wait for it
    if (images_in_flight[current_image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &images_in_flight[current_image_index], VK_TRUE, UINT64_MAX);
    }
    images_in_flight[current_image_index] = in_flight_fences[current_frame];

    vkResetFences(device, 1, &in_flight_fences[current_frame]);
    vkResetCommandPool(device, command_pools[current_frame], 0);

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(command_buffers[current_frame], &begin_info));

    return true;
}

void VkContext::begin_render_pass(float cr, float cg, float cb, float ca) {
    VkClearValue clear{};
    clear.color = {{ cr, cg, cb, ca }};

    VkRenderPassBeginInfo rp_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rp_info.renderPass        = render_pass;
    rp_info.framebuffer       = framebuffers[current_image_index];
    rp_info.renderArea.offset = { 0, 0 };
    rp_info.renderArea.extent = swapchain_extent;
    rp_info.clearValueCount   = 1;
    rp_info.pClearValues      = &clear;

    vkCmdBeginRenderPass(current_cmd(), &rp_info, VK_SUBPASS_CONTENTS_INLINE);

    // Set dynamic viewport and scissor
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapchain_extent.width);
    viewport.height   = static_cast<float>(swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(current_cmd(), 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain_extent;
    vkCmdSetScissor(current_cmd(), 0, 1, &scissor);
}

void VkContext::end_render_pass() {
    vkCmdEndRenderPass(current_cmd());
}

bool VkContext::end_frame() {
    VK_CHECK(vkEndCommandBuffer(current_cmd()));

    VkSemaphore          wait_sems[]   = { image_available_semaphores[current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore          signal_sems[] = { render_finished_semaphores[current_image_index] };

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = wait_sems;
    submit.pWaitDstStageMask    = wait_stages;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &command_buffers[current_frame];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = signal_sems;

    VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit, in_flight_fences[current_frame]));

    VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = signal_sems;
    present.swapchainCount     = 1;
    present.pSwapchains        = &swapchain;
    present.pImageIndices      = &current_image_index;

    VkResult result = vkQueuePresentKHR(present_queue, &present);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        return false;
    }
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[VkContext] Failed to present\n");
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void VkContext::wait_idle() {
    if (device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(device);
}

// =============================================================================
// Immediate command helpers
// =============================================================================

VkCommandBuffer VkContext::begin_single_time_commands() {
    VkCommandBufferAllocateInfo alloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandPool        = command_pools[0];
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(device, &alloc, &cmd));

    VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin));

    return cmd;
}

void VkContext::end_single_time_commands(VkCommandBuffer cmd) {
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &cmd;

    VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE));
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pools[0], 1, &cmd);
}

// =============================================================================
// Instance creation
// =============================================================================

void VkContext::create_instance(VkWindow& window, const char* app_name, bool enable_validation) {
    VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName   = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "ConsoleEngine-Vulkan";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_2;

    // Gather extensions: SDL2 required + optional debug
    auto extensions = window.required_extensions();
    if (enable_validation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    const char* validation_layer = "VK_LAYER_KHRONOS_validation";

    VkInstanceCreateInfo create_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if (enable_validation) {
        create_info.enabledLayerCount   = 1;
        create_info.ppEnabledLayerNames = &validation_layer;
    }

    VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));
}

// =============================================================================
// Debug messenger
// =============================================================================

void VkContext::setup_debug_messenger() {
    VkDebugUtilsMessengerCreateInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debug_callback;

    create_debug_messenger(instance, &info, &debug_messenger);
}

// =============================================================================
// Physical device selection
// =============================================================================

void VkContext::pick_physical_device(VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    assert(count > 0 && "No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    // Prefer discrete GPU, fall back to integrated
    VkPhysicalDevice fallback = VK_NULL_HANDLE;

    for (auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        uint32_t gfx, pres;
        if (!find_queue_families(dev, surface, gfx, pres)) continue;

        // Check swapchain support
        auto support = query_swapchain_support(dev, surface);
        if (support.formats.empty() || support.present_modes.empty()) continue;

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physical_device = dev;
            gpu_properties  = props;
            fprintf(stderr, "[VkContext] Using discrete GPU: %s\n", props.deviceName);
            return;
        }

        if (fallback == VK_NULL_HANDLE) {
            fallback       = dev;
            gpu_properties = props;
        }
    }

    assert(fallback != VK_NULL_HANDLE && "No suitable GPU found");
    physical_device = fallback;
    fprintf(stderr, "[VkContext] Using GPU: %s\n", gpu_properties.deviceName);
}

// =============================================================================
// Logical device + queues
// =============================================================================

void VkContext::create_logical_device(VkSurfaceKHR surface) {
    find_queue_families(physical_device, surface, graphics_queue_family, present_queue_family);

    // Unique queue families
    std::vector<uint32_t> unique_families = { graphics_queue_family };
    if (present_queue_family != graphics_queue_family) {
        unique_families.push_back(present_queue_family);
    }

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    for (uint32_t family : unique_families) {
        VkDeviceQueueCreateInfo qi{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        qi.queueFamilyIndex = family;
        qi.queueCount       = 1;
        qi.pQueuePriorities = &priority;
        queue_infos.push_back(qi);
    }

    // Features we need (minimal for 2D)
    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo create_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_infos.size());
    create_info.pQueueCreateInfos       = queue_infos.data();
    create_info.enabledExtensionCount   = 1;
    create_info.ppEnabledExtensionNames = device_extensions;
    create_info.pEnabledFeatures        = &features;

    VK_CHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device));

    vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_queue_family,  0, &present_queue);
}

// =============================================================================
// VMA allocator
// =============================================================================

void VkContext::create_allocator() {
    VmaAllocatorCreateInfo info{};
    info.physicalDevice = physical_device;
    info.device         = device;
    info.instance       = instance;
    // VMA will use vkGetInstanceProcAddr/vkGetDeviceProcAddr internally
    VK_CHECK(vmaCreateAllocator(&info, &allocator));
}

// =============================================================================
// Swapchain
// =============================================================================

void VkContext::create_swapchain(VkWindow& window) {
    auto support = query_swapchain_support(physical_device, window.surface());

    // Choose surface format: prefer BGRA8 SRGB
    VkSurfaceFormatKHR chosen_format = support.formats[0];
    for (const auto& f : support.formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen_format = f;
            break;
        }
    }

    // Choose present mode: prefer mailbox (triple-buffering), fall back to FIFO (vsync)
    VkPresentModeKHR chosen_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& m : support.present_modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosen_mode = m;
            break;
        }
    }

    // Choose extent
    VkExtent2D extent;
    if (support.capabilities.currentExtent.width != UINT32_MAX) {
        extent = support.capabilities.currentExtent;
    } else {
        extent.width  = std::clamp(static_cast<uint32_t>(window.width()),
                                   support.capabilities.minImageExtent.width,
                                   support.capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(window.height()),
                                   support.capabilities.minImageExtent.height,
                                   support.capabilities.maxImageExtent.height);
    }

    uint32_t image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 &&
        image_count > support.capabilities.maxImageCount) {
        image_count = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    ci.surface          = window.surface();
    ci.minImageCount    = image_count;
    ci.imageFormat      = chosen_format.format;
    ci.imageColorSpace  = chosen_format.colorSpace;
    ci.imageExtent      = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform     = support.capabilities.currentTransform;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode      = chosen_mode;
    ci.clipped          = VK_TRUE;
    ci.oldSwapchain     = VK_NULL_HANDLE;

    uint32_t families[] = { graphics_queue_family, present_queue_family };
    if (graphics_queue_family != present_queue_family) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = families;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain));

    // Retrieve images
    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &img_count, nullptr);
    swapchain_images.resize(img_count);
    vkGetSwapchainImagesKHR(device, swapchain, &img_count, swapchain_images.data());

    swapchain_format = chosen_format.format;
    swapchain_extent = extent;
}

void VkContext::create_swapchain_views() {
    swapchain_views.resize(swapchain_images.size());

    for (size_t i = 0; i < swapchain_images.size(); ++i) {
        VkImageViewCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        ci.image    = swapchain_images[i];
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.format   = swapchain_format;
        ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.subresourceRange.baseMipLevel   = 0;
        ci.subresourceRange.levelCount     = 1;
        ci.subresourceRange.baseArrayLayer = 0;
        ci.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(device, &ci, nullptr, &swapchain_views[i]));
    }
}

// =============================================================================
// Render pass (single color attachment, no depth for 2D)
// =============================================================================

void VkContext::create_render_pass() {
    VkAttachmentDescription color_attach{};
    color_attach.format         = swapchain_format;
    color_attach.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attach.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_ref{};
    color_ref.attachment = 0;
    color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_ref;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rp_info.attachmentCount = 1;
    rp_info.pAttachments    = &color_attach;
    rp_info.subpassCount    = 1;
    rp_info.pSubpasses      = &subpass;
    rp_info.dependencyCount = 1;
    rp_info.pDependencies   = &dep;

    VK_CHECK(vkCreateRenderPass(device, &rp_info, nullptr, &render_pass));
}

// =============================================================================
// Framebuffers
// =============================================================================

void VkContext::create_framebuffers() {
    framebuffers.resize(swapchain_views.size());

    for (size_t i = 0; i < swapchain_views.size(); ++i) {
        VkFramebufferCreateInfo ci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        ci.renderPass      = render_pass;
        ci.attachmentCount = 1;
        ci.pAttachments    = &swapchain_views[i];
        ci.width           = swapchain_extent.width;
        ci.height          = swapchain_extent.height;
        ci.layers          = 1;

        VK_CHECK(vkCreateFramebuffer(device, &ci, nullptr, &framebuffers[i]));
    }
}

// =============================================================================
// Command pools & buffers + sync objects
// =============================================================================

void VkContext::create_sync_objects() {
    command_pools.resize(MAX_FRAMES_IN_FLIGHT);
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Command pool (resettable)
        VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = graphics_queue_family;
        VK_CHECK(vkCreateCommandPool(device, &pool_info, nullptr, &command_pools[i]));

        // Command buffer
        VkCommandBufferAllocateInfo alloc{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc.commandPool        = command_pools[i];
        alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(device, &alloc, &command_buffers[i]));

        // Image-available semaphore (per frame-in-flight)
        VkSemaphoreCreateInfo sem_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(device, &sem_info, nullptr, &image_available_semaphores[i]));

        // Fence (starts signalled so first wait doesn't deadlock)
        VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]));
    }

    // Render-finished semaphores: one per swapchain image so the presentation
    // engine can hold each semaphore until its image is re-acquired.
    uint32_t img_count = static_cast<uint32_t>(swapchain_images.size());
    render_finished_semaphores.resize(img_count);
    for (uint32_t i = 0; i < img_count; ++i) {
        VkSemaphoreCreateInfo sem_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(device, &sem_info, nullptr, &render_finished_semaphores[i]));
    }

    // Per-image fence tracking (initially null = no frame is using this image)
    images_in_flight.resize(img_count, VK_NULL_HANDLE);
}

// =============================================================================
// Swapchain support query
// =============================================================================

SwapchainSupport VkContext::query_swapchain_support(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    SwapchainSupport support;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &support.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, nullptr);
    support.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, support.formats.data());

    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &mode_count, nullptr);
    support.present_modes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &mode_count, support.present_modes.data());

    return support;
}

bool VkContext::find_queue_families(VkPhysicalDevice dev, VkSurfaceKHR surface,
                                    uint32_t& graphics, uint32_t& present) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

    bool found_graphics = false, found_present = false;

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics = i;
            found_graphics = true;
        }
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present_support);
        if (present_support) {
            present = i;
            found_present = true;
        }
        if (found_graphics && found_present) return true;
    }

    return found_graphics && found_present;
}

// =============================================================================
// Cleanup
// =============================================================================

void VkContext::cleanup_swapchain() {
    for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    framebuffers.clear();
    for (auto view : swapchain_views) vkDestroyImageView(device, view, nullptr);
    swapchain_views.clear();

    // Destroy per-swapchain-image semaphores
    for (auto sem : render_finished_semaphores) vkDestroySemaphore(device, sem, nullptr);
    render_finished_semaphores.clear();
    images_in_flight.clear();

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
}

void VkContext::destroy() {
    wait_idle();

    cleanup_swapchain();

    // Per-swapchain-image semaphores (already cleaned up by cleanup_swapchain)
    // Per-frame-in-flight resources
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
        vkDestroyCommandPool(device, command_pools[i], nullptr);
    }

    vkDestroyRenderPass(device, render_pass, nullptr);

    if (allocator) {
        vmaDestroyAllocator(allocator);
        allocator = VK_NULL_HANDLE;
    }

    vkDestroyDevice(device, nullptr);

    if (validation_enabled_ && debug_messenger != VK_NULL_HANDLE) {
        destroy_debug_messenger(instance, debug_messenger);
    }

    vkDestroyInstance(instance, nullptr);
}
