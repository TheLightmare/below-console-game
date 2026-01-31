#pragma once
#include "component_base.hpp"
#include <string>

struct RenderComponent : Component {
    char ch = '?';
    std::string color;
    std::string bg_color;
    int priority = 0;
    
    RenderComponent() = default;
    RenderComponent(char ch, const std::string& color, const std::string& bg_color, int priority)
        : ch(ch), color(color), bg_color(bg_color), priority(priority) {}
    IMPLEMENT_CLONE(RenderComponent)
};
