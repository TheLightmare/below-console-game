#pragma once
#include "dungeon_state.hpp"

// GameState - Central state container for game data shared across scenes
// Replaces the DungeonState singleton pattern with explicit dependency injection.
// The Game class owns the GameState instance and passes it to scenes via SceneManager.
struct GameState {
    // Dungeon/world state for scene transitions and save/load
    DungeonState dungeon;
    
    // Reset all game state (new game)
    void reset() {
        dungeon.reset();
    }
    
    // Get dungeon state reference (convenience accessor)
    DungeonState& get_dungeon() { return dungeon; }
    const DungeonState& get_dungeon() const { return dungeon; }
};
