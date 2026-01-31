#pragma once

// =============================================================================
// Component System Facade Header
// =============================================================================
// This file includes all component headers for backward compatibility.
// New code should prefer including specific component headers directly.
// =============================================================================

// Base component class and IMPLEMENT_CLONE macro
#include "components/component_base.hpp"

// Position and collision components
#include "components/position_component.hpp"

// Rendering component
#include "components/render_component.hpp"

// Stats and name components
#include "components/stats_component.hpp"

// Inventory, anatomy, and equipment components
#include "components/inventory_component.hpp"

// Item, ranged weapon, ammo, and effect components
#include "components/item_component.hpp"

// Movement and controller components
#include "components/movement_component.hpp"

// Interactable components (doors, chests)
#include "components/interactable_component.hpp"

// Dialogue system components
#include "components/dialogue_component.hpp"

// NPC components
#include "components/npc_component.hpp"

// Shop components
#include "components/shop_component.hpp"

// Faction system components
#include "components/faction_component.hpp"

// Status effects (poison, regen, buffs, debuffs)
#include "components/status_effects_component.hpp"

// Corpse system (entities leaving bodies behind)
#include "components/corpse_component.hpp"

// Legacy includes for files that expect everything from this header
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
