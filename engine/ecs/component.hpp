#pragma once

// =============================================================================
// Component System Facade Header
// =============================================================================
// This file includes all engine component headers for convenience.
// New code should prefer including specific component headers directly.
// =============================================================================

// Base component class and IMPLEMENT_CLONE macro
#include "components/component_base.hpp"

// Position and collision components
#include "components/position_component.hpp"

// Rendering component
#include "components/render_component.hpp"

// Movement and controller components
#include "components/movement_component.hpp"

// Arbitrary entity properties/traits
#include "components/properties_component.hpp"

// Core types
#include "types.hpp"
#include "../util/json.hpp"
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
