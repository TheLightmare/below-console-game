#pragma once

// =============================================================================
// Vulkan 2D Engine - Vulkan Context
// =============================================================================
// Owns the core Vulkan objects: instance, device, swapchain, render pass,
// command buffers, and synchronisation primitives.
//
// Usage:
//   VkContext ctx;
//   ctx.init(window, "My Game", true);  // true = enable validation layers
//   // ... render loop ...
//   ctx.destroy();
// =============================================================================

#include "vk_types.hpp"

class VkWindow;

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   present_modes;
};

class VkContext {
public:
    // ---- Core objects ----
    VkInstance               instance        = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    VkPhysicalDevice         physical_device = VK_NULL_HANDLE;
    VkDevice                 device          = VK_NULL_HANDLE;
    VmaAllocator             allocator       = VK_NULL_HANDLE;

    // ---- Queues ----
    VkQueue  graphics_queue        = VK_NULL_HANDLE;
    VkQueue  present_queue         = VK_NULL_HANDLE;
    uint32_t graphics_queue_family = 0;
    uint32_t present_queue_family  = 0;

    // ---- Swapchain ----
    VkSwapchainKHR             swapchain        = VK_NULL_HANDLE;
    VkFormat                   swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D                 swapchain_extent = { 0, 0 };
    std::vector<VkImage>       swapchain_images;
    std::vector<VkImageView>   swapchain_views;

    // ---- Render pass ----
    VkRenderPass render_pass = VK_NULL_HANDLE;

    // ---- Framebuffers (one per swapchain image) ----
    std::vector<VkFramebuffer> framebuffers;

    // ---- Per-frame resources (double-buffered) ----
    std::vector<VkCommandPool>   command_pools;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore>     image_available_semaphores;
    std::vector<VkSemaphore>     render_finished_semaphores;
    std::vector<VkFence>         in_flight_fences;
    std::vector<VkFence>         images_in_flight;   // per swapchain image

    uint32_t current_frame       = 0;
    uint32_t current_image_index = 0;

    // ---- Physical device properties (useful for limits/info) ----
    VkPhysicalDeviceProperties gpu_properties{};

public:
    VkContext() = default;
    ~VkContext() = default;

    // No copy
    VkContext(const VkContext&) = delete;
    VkContext& operator=(const VkContext&) = delete;

    /// Full initialisation. Call once after VkWindow::init().
    void init(VkWindow& window, const char* app_name, bool enable_validation = true);

    /// Recreate swapchain (e.g. after window resize).
    void recreate_swapchain(VkWindow& window);

    /// Begin a frame: acquires swapchain image, resets command buffer.
    /// Returns false if swapchain is out-of-date (caller should recreate).
    bool begin_frame();

    /// Get the command buffer for the current frame.
    VkCommandBuffer current_cmd() const { return command_buffers[current_frame]; }

    /// Begin the main render pass for the current frame.
    void begin_render_pass(float clear_r = 0, float clear_g = 0, float clear_b = 0, float clear_a = 1);

    /// End render pass and submit.
    void end_render_pass();

    /// Present the current frame. Returns false if swapchain needs recreation.
    bool end_frame();

    /// Wait for device idle (e.g. before destruction or resize).
    void wait_idle();

    /// Clean up everything.
    void destroy();

    // ---- Immediate command helpers ----
    VkCommandBuffer begin_single_time_commands();
    void            end_single_time_commands(VkCommandBuffer cmd);

private:
    bool validation_enabled_ = false;

    void create_instance(VkWindow& window, const char* app_name, bool enable_validation);
    void setup_debug_messenger();
    void pick_physical_device(VkSurfaceKHR surface);
    void create_logical_device(VkSurfaceKHR surface);
    void create_allocator();
    void create_swapchain(VkWindow& window);
    void create_swapchain_views();
    void create_render_pass();
    void create_framebuffers();
    void create_sync_objects();
    void cleanup_swapchain();

    SwapchainSupport query_swapchain_support(VkPhysicalDevice dev, VkSurfaceKHR surface);
    bool             find_queue_families(VkPhysicalDevice dev, VkSurfaceKHR surface,
                                         uint32_t& graphics, uint32_t& present);
};
