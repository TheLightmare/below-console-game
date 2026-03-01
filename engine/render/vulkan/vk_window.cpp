#include "vk_window.hpp"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool VkWindow::init(const char* title, int width, int height, bool fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "[VkWindow] SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    window_ = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        flags
    );

    if (!window_) {
        fprintf(stderr, "[VkWindow] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    width_  = width;
    height_ = height;
    return true;
}

bool VkWindow::init_surface(VkInstance instance) {
    if (!SDL_Vulkan_CreateSurface(window_, instance, &surface_)) {
        fprintf(stderr, "[VkWindow] SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void VkWindow::handle_resize() {
    SDL_GetWindowSize(window_, &width_, &height_);
    resized_ = true;
}

void VkWindow::destroy(VkInstance instance) {
    if (surface_ != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

VkWindow::~VkWindow() {
    // Note: destroy() must be called explicitly with the VkInstance.
    // Destructor handles SDL_Quit if window was never properly destroyed.
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        SDL_Quit();
    }
}

// ---------------------------------------------------------------------------
// Extensions
// ---------------------------------------------------------------------------

std::vector<const char*> VkWindow::required_extensions() const {
    unsigned int count = 0;
    SDL_Vulkan_GetInstanceExtensions(window_, &count, nullptr);

    std::vector<const char*> extensions(count);
    SDL_Vulkan_GetInstanceExtensions(window_, &count, extensions.data());

    return extensions;
}
