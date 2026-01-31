#pragma once
#include "../types.hpp"
#include <memory>
#include <string>

// Forward declarations for custom effect callbacks
class ComponentManager;
struct Entity;

struct Component {
    virtual ~Component() = default;
    
    // Clone this component - returns a new heap-allocated copy
    virtual std::unique_ptr<Component> clone() const = 0;
    
    // Get component type name for serialization
    virtual std::string get_type_name() const { return "unknown"; }
};

// Helper macro to implement clone() for simple components
#define IMPLEMENT_CLONE(ComponentType) \
    std::unique_ptr<Component> clone() const override { \
        return std::make_unique<ComponentType>(*this); \
    }
