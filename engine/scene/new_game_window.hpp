#pragma once
#include "menu_window.hpp"
#include <functional>

// New game mode selection window

namespace NewGameWindow {

using namespace MenuWindow;

// Result from new game window
struct NewGameResult {
    enum class Mode {
        NONE,           // Window closed without selection
        EXPLORATION,    // Exploration mode selected
        STRUCTURE_DEBUG, // Structure debug mode selected
        CANCELLED       // User cancelled
    };
    
    Mode mode = Mode::NONE;
};

// Show new game mode selection window
inline NewGameResult show(
    Console* console,
    std::function<void()> render_background = nullptr
) {
    std::vector<std::string> options = {"Exploration Mode", "Structure Debug", "Back"};
    
    auto result = show_selection_menu(console, "SELECT MODE", options, render_background, 0, 50);
    
    if (result.cancelled || result.selected_index == 2) {
        return NewGameResult{NewGameResult::Mode::CANCELLED};
    }
    
    switch (result.selected_index) {
        case 0: return NewGameResult{NewGameResult::Mode::EXPLORATION};
        case 1: return NewGameResult{NewGameResult::Mode::STRUCTURE_DEBUG};
        default: return NewGameResult{NewGameResult::Mode::CANCELLED};
    }
}

} // namespace NewGameWindow
