#pragma once
#include <string>

// =============================================================================
// Scene Identifiers
// =============================================================================
// Define scene names as constants to ensure type safety and avoid typos.
// Use these constants instead of raw strings when calling change_scene().
// =============================================================================

namespace SceneId {
    // Core scenes
    constexpr const char* MENU = "menu";
    constexpr const char* GAME_OVER = "gameover";
    
    // Gameplay scenes
    constexpr const char* EXPLORATION = "exploration";
    constexpr const char* DUNGEON = "game";  // Legacy name for dungeon scene
    
    // Debug scenes (development only)
    constexpr const char* STRUCTURE_DEBUG = "structure_debug";
}
