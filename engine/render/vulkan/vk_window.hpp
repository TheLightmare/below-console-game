#pragma once

// =============================================================================
// Vulkan 2D Engine - SDL2 Window Wrapper
// =============================================================================
// Handles window creation, Vulkan surface, and resize events.
// This is the ONLY place SDL2 touches the rendering pipeline.
// =============================================================================

#include "vk_types.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

class VkWindow {
private:
    SDL_Window*  window_  = nullptr;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    int          width_   = 0;
    int          height_  = 0;
    bool         resized_ = false;

public:
    VkWindow() = default;
    ~VkWindow();

    // No copy
    VkWindow(const VkWindow&) = delete;
    VkWindow& operator=(const VkWindow&) = delete;

    /// Create the SDL2 window. Call before init_surface().
    bool init(const char* title, int width, int height, bool fullscreen = false);

    /// Create a Vulkan surface for this window.
    bool init_surface(VkInstance instance);

    /// Call after SDL_WINDOWEVENT_RESIZED to update cached size.
    void handle_resize();

    /// Destroy surface and window.
    void destroy(VkInstance instance);

    // Accessors
    SDL_Window*  handle()        const { return window_; }
    VkSurfaceKHR surface()       const { return surface_; }
    int          width()         const { return width_; }
    int          height()        const { return height_; }
    bool         was_resized()   const { return resized_; }
    void         reset_resized()       { resized_ = false; }

    /// Get the Vulkan instance extensions required by SDL2.
    std::vector<const char*> required_extensions() const;
};
