#pragma once

// =============================================================================
// Vulkan 2D Engine - SDL2 Input Mapper
// =============================================================================
// Translates SDL2 events into the engine's existing Key enum so that the
// rest of the engine (scenes, keybindings) works unchanged.
// =============================================================================

#include "input/input.hpp"
#include <SDL2/SDL.h>

namespace VkInput {

/// Poll all SDL events. Returns the last Key pressed this frame, or Key::NONE.
/// Also sets `quit` to true if the window close was requested.
/// Call once per frame.
Key poll_events(bool& quit, bool& window_resized);

/// Convert a single SDL_Keycode to the engine's Key enum.
Key sdl_keycode_to_key(SDL_Keycode code);

} // namespace VkInput
