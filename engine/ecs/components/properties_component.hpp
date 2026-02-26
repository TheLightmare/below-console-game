#pragma once
#include "component_base.hpp"
#include <vector>
#include <string>
#include <algorithm>

// PropertiesComponent - arbitrary descriptive properties/traits on an entity.
// Examples: "has swapped souls with a goblin", "cursed by the witch", "blessed by the temple"
struct PropertiesComponent : Component {
    std::vector<std::string> properties;
    
    PropertiesComponent() = default;
    
    // Add a property (no duplicates)
    void add_property(const std::string& prop) {
        if (!has_property(prop)) {
            properties.push_back(prop);
        }
    }
    
    // Remove a property by exact match
    bool remove_property(const std::string& prop) {
        auto it = std::find(properties.begin(), properties.end(), prop);
        if (it != properties.end()) {
            properties.erase(it);
            return true;
        }
        return false;
    }
    
    // Check if entity has an exact property
    bool has_property(const std::string& prop) const {
        return std::find(properties.begin(), properties.end(), prop) != properties.end();
    }
    
    // Check if entity has a property containing a substring
    bool has_property_containing(const std::string& substr) const {
        for (const auto& p : properties) {
            if (p.find(substr) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    // Get all properties matching a substring
    std::vector<std::string> find_properties(const std::string& substr) const {
        std::vector<std::string> result;
        for (const auto& p : properties) {
            if (p.find(substr) != std::string::npos) {
                result.push_back(p);
            }
        }
        return result;
    }
    
    // Clear all properties
    void clear() {
        properties.clear();
    }
    
    IMPLEMENT_CLONE(PropertiesComponent)
};
