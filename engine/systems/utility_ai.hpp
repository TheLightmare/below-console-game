#pragma once
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
#include <string>
#include <unordered_map>

class ComponentManager;
struct UtilityContext;
struct UtilityAIComponent;

enum class CurveType {
    LINEAR,          // y = mx + b
    QUADRATIC,       // y = (mx + b)^k
    LOGISTIC,        // y = k / (1 + e^(-m*(x-c)))
    LOGIT,           // y = log(x / (1-x)) normalized
    EXPONENTIAL,     // y = e^(kx) normalized
    STEP,            // y = 0 if x < threshold, 1 otherwise
    INVERSE,         // y = 1 - x
    SINE,            // y = sin(x * pi/2)
    PARABOLIC,       // y = 4x(1-x) - peaks at 0.5
    CUSTOM           // User-defined function
};

struct ResponseCurve {
    CurveType type = CurveType::LINEAR;
    float slope = 1.0f;      // m: affects steepness
    float exponent = 1.0f;   // k: power/exponent
    float x_shift = 0.0f;    // c: horizontal shift
    float y_shift = 0.0f;    // b: vertical shift
    float threshold = 0.5f;  // For step function
    bool clamp = true;       // Clamp output to 0-1
    
    ResponseCurve() = default;
    ResponseCurve(CurveType t, float m = 1.0f, float k = 1.0f, float c = 0.0f, float b = 0.0f)
        : type(t), slope(m), exponent(k), x_shift(c), y_shift(b) {}
    
    // Evaluate the curve at point x (typically 0-1 input)
    float evaluate(float x) const {
        float result = 0.0f;
        
        switch (type) {
            case CurveType::LINEAR:
                result = slope * x + y_shift;
                break;
                
            case CurveType::QUADRATIC:
                result = std::pow(slope * (x - x_shift) + y_shift, exponent);
                break;
                
            case CurveType::LOGISTIC:
                result = exponent / (1.0f + std::exp(-slope * (x - x_shift)));
                break;
                
            case CurveType::LOGIT:
                // Avoid division by zero
                x = std::clamp(x, 0.01f, 0.99f);
                result = (std::log(x / (1.0f - x)) + 4.6f) / 9.2f; // Normalize to ~0-1
                break;
                
            case CurveType::EXPONENTIAL:
                result = (std::exp(exponent * x) - 1.0f) / (std::exp(exponent) - 1.0f);
                break;
                
            case CurveType::STEP:
                result = (x >= threshold) ? 1.0f : 0.0f;
                break;
                
            case CurveType::INVERSE:
                result = 1.0f - (slope * x + y_shift);
                break;
                
            case CurveType::SINE:
                result = std::sin(x * 3.14159f * 0.5f);
                break;
                
            case CurveType::PARABOLIC:
                result = 4.0f * x * (1.0f - x);
                break;
                
            default:
                result = x;
                break;
        }
        
        if (clamp) {
            result = std::clamp(result, 0.0f, 1.0f);
        }
        
        return result;
    }
    
    // Preset curves for common use cases
    static ResponseCurve linear() { return ResponseCurve(CurveType::LINEAR); }
    static ResponseCurve inverse() { return ResponseCurve(CurveType::INVERSE); }
    static ResponseCurve quadratic() { return ResponseCurve(CurveType::QUADRATIC, 1.0f, 2.0f); }
    static ResponseCurve exponential() { return ResponseCurve(CurveType::EXPONENTIAL, 1.0f, 3.0f); }
    static ResponseCurve steep_dropoff() { return ResponseCurve(CurveType::QUADRATIC, 1.0f, 0.5f); }
    static ResponseCurve bell_curve() { return ResponseCurve(CurveType::PARABOLIC); }
    static ResponseCurve step(float threshold = 0.5f) { 
        ResponseCurve c(CurveType::STEP); 
        c.threshold = threshold; 
        return c; 
    }
};

enum class InputType {
    // Self-related inputs
    HEALTH_PERCENT,          // current_hp / max_hp
    HEALTH_ABSOLUTE,         // current_hp (normalized by max expected)
    LEVEL,                   // Entity level (normalized)
    ATTACK_STAT,             // Attack stat (normalized)
    DEFENSE_STAT,            // Defense stat (normalized)
    
    // Target-related inputs (requires target in context)
    DISTANCE_TO_TARGET,      // Distance to current target (normalized by aggro range)
    TARGET_HEALTH_PERCENT,   // Target's hp percentage
    TARGET_THREAT_LEVEL,     // Computed threat based on stats
    IS_TARGET_HOSTILE,       // 1.0 if target is hostile, 0.0 otherwise
    IS_TARGET_ALLY,          // 1.0 if target is ally, 0.0 otherwise
    
    // Environmental inputs
    NEARBY_ENEMIES_COUNT,    // Number of hostile entities in range (normalized)
    NEARBY_ALLIES_COUNT,     // Number of allied entities in range (normalized)
    ALLIES_UNDER_ATTACK,     // Any ally being attacked? (0 or 1)
    
    // Situational inputs
    IN_COMBAT,               // Currently engaged in combat? (0 or 1)
    TIME_SINCE_LAST_ATTACK,  // Turns since last attack (normalized)
    HAS_TARGET,              // Is there a valid target? (0 or 1)
    TARGET_IN_MELEE_RANGE,   // Is target within attack range? (0 or 1)
    
    // Inventory/equipment
    HAS_HEALING_ITEM,        // Has consumable healing item? (0 or 1)
    HAS_RANGED_WEAPON,       // Has ranged weapon equipped? (0 or 1)
    
    // Home-related inputs
    DISTANCE_TO_HOME,        // Distance to home position (normalized)
    IS_AT_HOME,              // Currently at home? (0 or 1)
    IS_NIGHT_TIME,           // Is it night time? (0 or 1) - placeholder for time system
    
    CUSTOM
};

struct Consideration {
    std::string name;              // For debugging
    InputType input_type = InputType::HEALTH_PERCENT;
    ResponseCurve curve;
    float weight = 1.0f;           // Importance multiplier
    float bonus = 0.0f;            // Flat bonus to output
    
    // Custom input function (for InputType::CUSTOM)
    std::function<float(const UtilityContext&)> custom_input;
    
    Consideration() = default;
    Consideration(const std::string& name, InputType input, ResponseCurve curve, float weight = 1.0f)
        : name(name), input_type(input), curve(curve), weight(weight) {}
    
    // Evaluate this consideration given the context (inline implementation below)
    inline float evaluate(const UtilityContext& context) const;
};

enum class UtilityActionType {
    IDLE,                    // Do nothing
    WANDER,                  // Random movement
    PATROL,                  // Move along patrol route
    
    // Combat actions
    ATTACK_MELEE,            // Melee attack target
    ATTACK_RANGED,           // Ranged attack target
    CHASE,                   // Move toward target
    FLEE,                    // Move away from threats
    RETREAT,                 // Move to safe position
    
    // Support actions
    DEFEND_ALLY,             // Protect nearby ally
    HEAL_SELF,               // Use healing item/ability
    BUFF_ALLY,               // Buff nearby ally
    
    // Special behaviors
    INVESTIGATE,             // Check out suspicious area
    GUARD_POSITION,          // Stay at current position
    CALL_FOR_HELP,           // Alert nearby allies
    RETURN_HOME,             // Return to home location
    SLEEP,                   // Sleep at home (night behavior)
    
    // Interaction
    LOOT,                    // Pick up nearby items
    USE_ENVIRONMENT,         // Use environmental object
    
    CUSTOM                   // Custom action with callback
};

struct UtilityAction {
    std::string name;
    UtilityActionType type = UtilityActionType::IDLE;
    std::vector<Consideration> considerations;
    
    // Score modifiers
    float base_score = 0.0f;       // Starting score before considerations
    float score_multiplier = 1.0f; // Applied after all considerations
    float momentum_bonus = 0.0f;   // Bonus if this was the previous action
    
    // Execution parameters
    EntityId target_id = 0;        // Target for this action (if applicable)
    int cooldown_turns = 0;        // Turns before this action can be used again
    int current_cooldown = 0;      // Current cooldown counter
    
    // Custom execution function
    std::function<void(EntityId, const UtilityContext&)> custom_execute;
    
    UtilityAction() = default;
    UtilityAction(const std::string& action_name, UtilityActionType action_type)
        : name(action_name), type(action_type) {}
    
    // Add a consideration to this action
    UtilityAction& add_consideration(const std::string& consideration_name, InputType input, 
                                      ResponseCurve curve, float weight = 1.0f) {
        considerations.emplace_back(consideration_name, input, curve, weight);
        return *this;
    }
    
    // Evaluate all considerations and return final utility score (inline implementation below)
    inline float evaluate(const UtilityContext& context) const;
    
    // Check if action is on cooldown
    bool is_on_cooldown() const { return current_cooldown > 0; }
    
    // Tick cooldown
    void tick_cooldown() { if (current_cooldown > 0) current_cooldown--; }
    
    // Trigger cooldown after use
    void trigger_cooldown() { current_cooldown = cooldown_turns; }
};

struct UtilityContext {
    // Core references
    ComponentManager* manager = nullptr;
    EntityId self_id = 0;
    EntityId player_id = 0;
    
    // Cached component pointers for self
    PositionComponent* self_pos = nullptr;
    StatsComponent* self_stats = nullptr;
    UtilityAIComponent* self_utility_ai = nullptr;
    FactionComponent* self_faction = nullptr;
    InventoryComponent* self_inventory = nullptr;
    EquipmentComponent* self_equipment = nullptr;
    
    // Home-related context
    int home_x = 0;
    int home_y = 0;
    bool has_home = false;
    float distance_to_home = 999999.0f;
    bool is_at_home = false;
    bool is_night_time = false;  // Placeholder for time system
    
    // Current target (if any)
    EntityId target_id = 0;
    PositionComponent* target_pos = nullptr;
    StatsComponent* target_stats = nullptr;
    FactionComponent* target_faction = nullptr;
    
    // Computed values (cached for efficiency)
    float distance_to_target = 999999.0f;
    int nearby_enemies = 0;
    int nearby_allies = 0;
    bool ally_under_attack = false;
    bool in_combat = false;
    int turns_since_attack = 0;
    
    // Action state
    UtilityActionType last_action = UtilityActionType::IDLE;
    
    // Helper methods
    float get_self_hp_percent() const {
        if (!self_stats || self_stats->max_hp <= 0) return 0.0f;
        return static_cast<float>(self_stats->current_hp) / static_cast<float>(self_stats->max_hp);
    }
    
    float get_target_hp_percent() const {
        if (!target_stats || target_stats->max_hp <= 0) return 0.0f;
        return static_cast<float>(target_stats->current_hp) / static_cast<float>(target_stats->max_hp);
    }
    
    bool has_target() const { return target_id != 0 && target_pos != nullptr; }
    
    bool target_in_melee_range() const { return distance_to_target <= 1.5f; }
};

struct UtilityAIComponent : Component {
    std::vector<UtilityAction> actions;
    
    // Decision-making parameters
    float score_threshold = 0.1f;  // Minimum score to consider an action
    float randomness = 0.0f;       // Random factor added to scores (0-1)
    bool use_momentum = true;      // Add bonus to previously selected action
    float momentum_value = 0.1f;   // How much bonus for momentum
    
    // State tracking
    UtilityActionType last_action = UtilityActionType::IDLE;
    int action_streak = 0;         // How many times same action was chosen
    
    // Evaluation config
    float aggro_range = 8.0f;      // Range to detect enemies
    float ally_range = 10.0f;      // Range to detect allies
    
    UtilityAIComponent() = default;
    
    // Add an action to this AI
    UtilityAction& add_action(const std::string& name, UtilityActionType type) {
        actions.emplace_back(name, type);
        return actions.back();
    }
    
    // Remove action by type
    void remove_action(UtilityActionType type) {
        actions.erase(
            std::remove_if(actions.begin(), actions.end(),
                [type](const UtilityAction& a) { return a.type == type; }),
            actions.end()
        );
    }
    
    // Get action by type
    UtilityAction* get_action(UtilityActionType type) {
        for (auto& action : actions) {
            if (action.type == type) return &action;
        }
        return nullptr;
    }
    
    // Tick all action cooldowns
    void tick_cooldowns() {
        for (auto& action : actions) {
            action.tick_cooldown();
        }
    }
    
    std::unique_ptr<Component> clone() const override {
        return std::make_unique<UtilityAIComponent>(*this);
    }
};

namespace UtilityPresets {

// Aggressive melee fighter - charges in, attacks, low flee threshold
inline void setup_aggressive_melee(UtilityAIComponent& ai) {
    ai.actions.clear();
    
    // Chase action - high priority when target exists and not in melee range
    ai.add_action("Chase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f)
        .add_consideration("NotInMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::inverse(), 1.0f)
        .add_consideration("TargetDistance", InputType::DISTANCE_TO_TARGET, ResponseCurve::inverse(), 0.8f)
        .add_consideration("HealthOK", InputType::HEALTH_PERCENT, ResponseCurve::step(0.2f), 0.5f);
    
    // Attack action - highest priority when in melee range
    ai.add_action("Attack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.0f)
        .add_consideration("TargetHealth", InputType::TARGET_HEALTH_PERCENT, ResponseCurve::linear(), 0.5f);
    
    // Flee action - only when very low health
    auto& flee = ai.add_action("Flee", UtilityActionType::FLEE);
    flee.add_consideration("LowHealth", InputType::HEALTH_PERCENT, 
        ResponseCurve(CurveType::INVERSE, 4.0f), 2.0f);  // Strong when HP < 25%
    flee.add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f);
    
    // Wander action - fallback when no target
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("NoTarget", InputType::HAS_TARGET, ResponseCurve::inverse(), 1.0f);
    
    // Idle action - very low base priority
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.05f;
}

// Defensive guard - protects allies, holds position
inline void setup_defensive_guard(UtilityAIComponent& ai) {
    ai.actions.clear();
    
    // Guard position - default behavior
    ai.add_action("Guard", UtilityActionType::GUARD_POSITION)
        .add_consideration("NoThreat", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::inverse(), 1.0f)
        .add_consideration("HealthyAllies", InputType::ALLIES_UNDER_ATTACK, ResponseCurve::inverse(), 1.5f);
    ai.get_action(UtilityActionType::GUARD_POSITION)->base_score = 0.3f;
    
    // Defend ally - high priority when ally is attacked
    ai.add_action("DefendAlly", UtilityActionType::DEFEND_ALLY)
        .add_consideration("AllyAttacked", InputType::ALLIES_UNDER_ATTACK, ResponseCurve::step(0.5f), 2.0f)
        .add_consideration("HealthOK", InputType::HEALTH_PERCENT, ResponseCurve::step(0.3f), 1.0f);
    
    // Attack - when enemy is close
    ai.add_action("Attack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 1.5f)
        .add_consideration("TargetHostile", InputType::IS_TARGET_HOSTILE, ResponseCurve::step(0.5f), 1.0f);
    
    // Chase - but not too far
    ai.add_action("Chase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f)
        .add_consideration("CloseEnough", InputType::DISTANCE_TO_TARGET, 
            ResponseCurve(CurveType::PARABOLIC), 1.2f);  // Prefer mid-range
}

// Cowardly creature - flees easily, only attacks when cornered
inline void setup_cowardly(UtilityAIComponent& ai) {
    ai.actions.clear();
    
    // Flee - very high priority when threatened
    ai.add_action("Flee", UtilityActionType::FLEE)
        .add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 2.0f)
        .add_consideration("LowHealth", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 1.5f)
        .add_consideration("EnemyClose", InputType::DISTANCE_TO_TARGET, ResponseCurve::inverse(), 1.2f);
    
    // Attack - only when cornered (low HP and enemy very close)
    ai.add_action("CornerAttack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("VeryLowHP", InputType::HEALTH_PERCENT, 
            ResponseCurve(CurveType::INVERSE, 2.0f), 1.5f)  // High when HP very low
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.0f);
    
    // Wander - safe default
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("Safe", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::inverse(), 1.0f);
    ai.get_action(UtilityActionType::WANDER)->base_score = 0.2f;
}

// Pack hunter - works with allies, flanks targets
inline void setup_pack_hunter(UtilityAIComponent& ai) {
    ai.actions.clear();
    
    // Chase - higher priority when allies nearby
    ai.add_action("PackChase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f)
        .add_consideration("AlliesNearby", InputType::NEARBY_ALLIES_COUNT, ResponseCurve::linear(), 1.5f)
        .add_consideration("NotInRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::inverse(), 0.8f);
    
    // Attack - bonus when allies are also attacking
    ai.add_action("PackAttack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.0f)
        .add_consideration("AlliesNearby", InputType::NEARBY_ALLIES_COUNT, ResponseCurve::linear(), 0.8f);
    
    // Call for help - when alone and threatened
    ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP)
        .add_consideration("Alone", InputType::NEARBY_ALLIES_COUNT, ResponseCurve::inverse(), 1.5f)
        .add_consideration("InDanger", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::linear(), 1.2f)
        .add_consideration("LowHP", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 1.0f);
    
    // Flee - when alone and hurt
    ai.add_action("Flee", UtilityActionType::FLEE)
        .add_consideration("Alone", InputType::NEARBY_ALLIES_COUNT, ResponseCurve::inverse(), 1.5f)
        .add_consideration("LowHP", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 2.0f)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f);
    
    // Wander together
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("NoTarget", InputType::HAS_TARGET, ResponseCurve::inverse(), 1.0f);
}

// Berserker - more aggressive when hurt
inline void setup_berserker(UtilityAIComponent& ai) {
    ai.actions.clear();
    
    // Berserk attack - gets stronger as HP decreases
    ai.add_action("BerserkAttack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 1.5f)
        .add_consideration("Wounded", InputType::HEALTH_PERCENT, 
            ResponseCurve(CurveType::INVERSE, 0.5f), 2.0f);  // More aggressive when hurt
    
    // Berserk chase - charges recklessly when hurt
    ai.add_action("BerserkChase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f)
        .add_consideration("NotInRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::inverse(), 1.0f)
        .add_consideration("Wounded", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 1.5f);
    
    // Normal wander
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("NoTarget", InputType::HAS_TARGET, ResponseCurve::inverse(), 1.0f);
    
    // Never flees - no flee action!
}

inline void setup_defensive_villager(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 6.0f;
    ai.ally_range = 10.0f;  // Will help nearby allies
    ai.randomness = 0.2f;  // More variety in behavior
    ai.use_momentum = true;
    ai.momentum_value = 0.15f;  // Tend to keep doing what they're doing
    
    // Fight back when damaged (low health = was attacked) - they're not cowards!
    auto& attack = ai.add_action("FightBack", UtilityActionType::ATTACK_MELEE);
    // Inverse of health percent: when health is lower (damaged), this scores higher
    attack.add_consideration("Damaged", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 2.0f);
    attack.add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.5f);
    attack.base_score = 0.0f;  // Only activates when considerations trigger
    
    // Chase attackers when damaged
    auto& chase = ai.add_action("ChaseAttacker", UtilityActionType::CHASE);
    chase.add_consideration("Damaged", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 1.8f);
    chase.add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f);
    chase.base_score = 0.0f;
    
    // Call for help when threatened - alert guards and other villagers
    auto& call = ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP);
    call.add_consideration("Damaged", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 2.2f);
    call.cooldown_turns = 5;
    
    // Flee when health is very low (survival instinct) - inverse with steep response
    auto& flee = ai.add_action("FleeWhenHurt", UtilityActionType::FLEE);
    // Using quadratic inverse for steep response at low health
    ResponseCurve low_health_curve(CurveType::INVERSE, 1.5f, 1.0f, 0.0f, 0.0f);
    flee.add_consideration("LowHealth", InputType::HEALTH_PERCENT, low_health_curve, 2.0f);
    flee.base_score = 0.0f;
    
    // Wander around the area - daily routine (simple, always available)
    auto& wander = ai.add_action("Wander", UtilityActionType::WANDER);
    wander.base_score = 0.5f;  // Base score means it works without considerations
    
    // Idle - rest and stand around
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.3f;
}

// Peaceful villager - has daily routine, flees from threats (for truly passive NPCs)
inline void setup_peaceful_villager(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 6.0f;
    ai.randomness = 0.2f;  // More variety in behavior
    ai.use_momentum = true;
    ai.momentum_value = 0.15f;  // Tend to keep doing what they're doing
    
    // Flee from any threat - highest priority
    auto& flee = ai.add_action("Flee", UtilityActionType::FLEE);
    flee.add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 2.5f);
    flee.base_score = 0.0f;
    
    // Call for help when threatened
    auto& call = ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP);
    call.add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 2.0f);
    call.cooldown_turns = 5;
    
    // Wander around the area - daily routine (simple, always available)
    auto& wander = ai.add_action("Wander", UtilityActionType::WANDER);
    wander.base_score = 0.5f;  // Base score means it works without considerations
    
    // Idle - rest and stand around
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.3f;
}

// Villager with home - has a home they return to, wanders during day, returns home at night
inline void setup_villager_with_home(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 6.0f;
    ai.ally_range = 10.0f;
    ai.randomness = 0.15f;
    ai.use_momentum = true;
    ai.momentum_value = 0.2f;
    
    // Return home when it's night or when far from home
    // Uses distance_to_home consideration - higher score when far from home
    auto& go_home = ai.add_action("ReturnHome", UtilityActionType::RETURN_HOME);
    go_home.add_consideration("FarFromHome", InputType::DISTANCE_TO_HOME, ResponseCurve::linear(), 1.5f);
    go_home.add_consideration("NightTime", InputType::IS_NIGHT_TIME, ResponseCurve::step(0.5f), 2.0f);
    go_home.base_score = 0.2f;  // Slight preference to stay near home
    
    // Sleep when at home and it's night
    auto& sleep = ai.add_action("Sleep", UtilityActionType::SLEEP);
    sleep.add_consideration("AtHome", InputType::IS_AT_HOME, ResponseCurve::step(0.5f), 2.5f);
    sleep.add_consideration("NightTime", InputType::IS_NIGHT_TIME, ResponseCurve::step(0.5f), 2.0f);
    sleep.base_score = 0.0f;
    
    // Fight back when damaged
    auto& attack = ai.add_action("FightBack", UtilityActionType::ATTACK_MELEE);
    attack.add_consideration("Damaged", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 2.0f);
    attack.add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.5f);
    attack.base_score = 0.0f;
    
    // Chase attackers when damaged
    auto& chase = ai.add_action("ChaseAttacker", UtilityActionType::CHASE);
    chase.add_consideration("Damaged", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 1.8f);
    chase.add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f);
    chase.base_score = 0.0f;
    
    // Call for help when threatened
    auto& call = ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP);
    call.add_consideration("Damaged", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 2.2f);
    call.cooldown_turns = 5;
    
    // Flee when health is very low
    auto& flee = ai.add_action("FleeWhenHurt", UtilityActionType::FLEE);
    ResponseCurve low_health_curve(CurveType::INVERSE, 1.5f, 1.0f, 0.0f, 0.0f);
    flee.add_consideration("LowHealth", InputType::HEALTH_PERCENT, low_health_curve, 2.0f);
    flee.base_score = 0.0f;
    
    // Wander around during the day (lower priority when far from home)
    auto& wander = ai.add_action("Wander", UtilityActionType::WANDER);
    wander.add_consideration("NotNight", InputType::IS_NIGHT_TIME, ResponseCurve::inverse(), 0.8f);
    wander.add_consideration("NearHome", InputType::DISTANCE_TO_HOME, ResponseCurve::inverse(), 0.6f);
    wander.base_score = 0.45f;
    
    // Idle - rest and stand around at home
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.add_consideration("AtHome", InputType::IS_AT_HOME, ResponseCurve::linear(), 0.5f);
    idle.base_score = 0.25f;
}

// Town guard - patrols area, defends citizens, attacks hostiles
inline void setup_town_guard(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 12.0f;  // Guards are vigilant
    ai.ally_range = 15.0f;
    ai.randomness = 0.1f;
    ai.use_momentum = true;
    ai.momentum_value = 0.1f;
    
    // Defend allies under attack - highest priority
    auto& defend = ai.add_action("DefendAlly", UtilityActionType::DEFEND_ALLY);
    defend.add_consideration("AllyAttacked", InputType::ALLIES_UNDER_ATTACK, ResponseCurve::step(0.5f), 3.0f);
    defend.base_score = 0.0f;
    
    // Attack hostile targets in range
    auto& attack = ai.add_action("Attack", UtilityActionType::ATTACK_MELEE);
    attack.add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.5f);
    attack.base_score = 0.0f;
    
    // Chase hostiles
    auto& chase = ai.add_action("Chase", UtilityActionType::CHASE);
    chase.add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 2.0f);
    chase.base_score = 0.0f;
    
    // Patrol the area - primary duty when no threats (uses base score)
    auto& patrol = ai.add_action("Patrol", UtilityActionType::WANDER);
    patrol.base_score = 0.5f;
    
    // Guard position - stand watch at key points
    auto& guard = ai.add_action("Guard", UtilityActionType::GUARD_POSITION);
    guard.base_score = 0.4f;
    
    // Call for backup when in combat
    auto& call = ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP);
    call.add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 1.5f);
    call.cooldown_turns = 5;
}

// Merchant - stays at shop, flees and calls for guards when threatened
inline void setup_merchant(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 5.0f;
    ai.ally_range = 20.0f;  // Large range to find guards
    ai.randomness = 0.1f;
    ai.use_momentum = true;
    ai.momentum_value = 0.2f;  // Merchants like routine
    
    // Call for help immediately when threatened
    auto& call = ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP);
    call.add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 3.0f);
    call.cooldown_turns = 3;
    
    // Flee when threatened
    auto& flee = ai.add_action("Flee", UtilityActionType::FLEE);
    flee.add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 2.5f);
    flee.base_score = 0.0f;
    
    // Guard position (stay at shop) - primary behavior
    auto& guard = ai.add_action("Guard", UtilityActionType::GUARD_POSITION);
    guard.base_score = 0.55f;
    
    // Occasionally move around shop area
    auto& wander = ai.add_action("Wander", UtilityActionType::WANDER);
    wander.base_score = 0.3f;
    
    // Idle - waiting for customers
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.4f;
}

// Wandering NPC - explorer, moves around freely, curious
inline void setup_wanderer(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 6.0f;
    ai.randomness = 0.25f;  // Very unpredictable
    ai.use_momentum = false;  // Changes mind often
    
    // Flee from threats
    auto& flee = ai.add_action("Flee", UtilityActionType::FLEE);
    flee.add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 2.5f);
    flee.base_score = 0.0f;
    
    // Wander is primary behavior - exploring
    auto& wander = ai.add_action("Wander", UtilityActionType::WANDER);
    wander.base_score = 0.6f;
    
    // Idle - rest occasionally
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.2f;
}

inline void setup_wildlife(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 8.0f;
    ai.randomness = 0.1f;
    
    // Flee from any threat
    ai.add_action("Flee", UtilityActionType::FLEE)
        .add_consideration("SeesEnemy", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::step(0.1f), 2.0f)
        .add_consideration("EnemyClose", InputType::DISTANCE_TO_TARGET, ResponseCurve::inverse(), 1.5f);
    
    // Wander when safe
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("NoThreat", InputType::NEARBY_ENEMIES_COUNT, ResponseCurve::inverse(), 1.0f);
    ai.get_action(UtilityActionType::WANDER)->base_score = 0.4f;
    
    // Idle
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.3f;
}

// Predator - hunts prey, moderate aggression
inline void setup_predator(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 10.0f;
    ai.randomness = 0.05f;
    
    // Attack when in melee range
    ai.add_action("Attack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.0f)
        .add_consideration("TargetWeak", InputType::TARGET_HEALTH_PERCENT, ResponseCurve::inverse(), 0.8f);
    
    // Chase prey
    ai.add_action("Chase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f)
        .add_consideration("NotInRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::inverse(), 1.0f)
        .add_consideration("HealthOK", InputType::HEALTH_PERCENT, ResponseCurve::step(0.3f), 0.8f);
    
    // Flee when badly hurt
    ai.add_action("Flee", UtilityActionType::FLEE)
        .add_consideration("LowHP", InputType::HEALTH_PERCENT, ResponseCurve(CurveType::INVERSE, 3.0f), 2.0f)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f);
    
    // Wander looking for prey
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("NoTarget", InputType::HAS_TARGET, ResponseCurve::inverse(), 1.0f);
    ai.get_action(UtilityActionType::WANDER)->base_score = 0.2f;
}

// Spider - ambush predator, patient, attacks when prey is close
inline void setup_ambush_predator(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 4.0f;  // Short aggro range
    ai.randomness = 0.02f;
    
    // Attack when in melee range - very high priority
    ai.add_action("Attack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 3.0f);
    
    // Only chase when target is very close
    ai.add_action("Chase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.0f)
        .add_consideration("VeryClose", InputType::DISTANCE_TO_TARGET, ResponseCurve(CurveType::INVERSE, 2.0f), 2.0f);
    
    // Guard position (wait for prey)
    ai.add_action("Guard", UtilityActionType::GUARD_POSITION);
    ai.get_action(UtilityActionType::GUARD_POSITION)->base_score = 0.6f;
    
    // Idle most of the time
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.4f;
}

// Boss creature - never flees, calls minions, relentless
inline void setup_boss(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 15.0f;
    ai.ally_range = 20.0f;
    
    // Attack when in range
    ai.add_action("Attack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.5f);
    
    // Chase relentlessly
    ai.add_action("Chase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.5f)
        .add_consideration("NotInRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::inverse(), 1.0f);
    
    // Call for help when hurt
    ai.add_action("CallHelp", UtilityActionType::CALL_FOR_HELP)
        .add_consideration("Wounded", InputType::HEALTH_PERCENT, ResponseCurve::inverse(), 1.5f)
        .add_consideration("AlliesExist", InputType::NEARBY_ALLIES_COUNT, ResponseCurve::step(0.1f), 0.5f);
    ai.get_action(UtilityActionType::CALL_FOR_HELP)->cooldown_turns = 5;
    
    // Idle fallback
    auto& idle = ai.add_action("Idle", UtilityActionType::IDLE);
    idle.base_score = 0.1f;
    
    // NEVER flees - no flee action
}

// Undead - mindless pursuit, no self-preservation
inline void setup_undead(UtilityAIComponent& ai) {
    ai.actions.clear();
    ai.aggro_range = 8.0f;
    ai.randomness = 0.02f;
    
    // Attack - primary goal
    ai.add_action("Attack", UtilityActionType::ATTACK_MELEE)
        .add_consideration("InMeleeRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::step(0.5f), 2.5f);
    
    // Chase - relentless
    ai.add_action("Chase", UtilityActionType::CHASE)
        .add_consideration("HasTarget", InputType::HAS_TARGET, ResponseCurve::step(0.5f), 1.5f)
        .add_consideration("NotInRange", InputType::TARGET_IN_MELEE_RANGE, ResponseCurve::inverse(), 1.0f);
    
    // Wander aimlessly
    ai.add_action("Wander", UtilityActionType::WANDER)
        .add_consideration("NoTarget", InputType::HAS_TARGET, ResponseCurve::inverse(), 1.0f);
    ai.get_action(UtilityActionType::WANDER)->base_score = 0.3f;
    
    // No flee - undead don't fear death
}

} // namespace UtilityPresets

inline float Consideration::evaluate(const UtilityContext& context) const {
    float input_value = 0.0f;
    
    switch (input_type) {
        // Self-related inputs
        case InputType::HEALTH_PERCENT:
            input_value = context.get_self_hp_percent();
            break;
            
        case InputType::HEALTH_ABSOLUTE:
            if (context.self_stats) {
                input_value = std::min(1.0f, context.self_stats->current_hp / 200.0f);
            }
            break;
            
        case InputType::LEVEL:
            if (context.self_stats) {
                input_value = std::min(1.0f, context.self_stats->level / 20.0f);
            }
            break;
            
        case InputType::ATTACK_STAT:
            if (context.self_stats) {
                input_value = std::min(1.0f, context.self_stats->attack / 50.0f);
            }
            break;
            
        case InputType::DEFENSE_STAT:
            if (context.self_stats) {
                input_value = std::min(1.0f, context.self_stats->defense / 50.0f);
            }
            break;
            
        // Target-related inputs
        case InputType::DISTANCE_TO_TARGET:
            if (context.has_target()) {
                // Use utility AI aggro range, fallback to default
                float aggro_range = 8.0f;
                if (context.self_utility_ai) {
                    aggro_range = context.self_utility_ai->aggro_range;
                }
                input_value = std::min(1.0f, context.distance_to_target / aggro_range);
            } else {
                input_value = 1.0f;
            }
            break;
            
        case InputType::TARGET_HEALTH_PERCENT:
            input_value = context.get_target_hp_percent();
            break;
            
        case InputType::TARGET_THREAT_LEVEL:
            if (context.target_stats && context.self_stats) {
                float threat = static_cast<float>(context.target_stats->attack) / 
                               static_cast<float>(std::max(1, context.self_stats->defense));
                input_value = std::min(1.0f, threat / 3.0f);
            }
            break;
            
        case InputType::IS_TARGET_HOSTILE:
            if (context.target_faction && context.self_faction) {
                FactionManager& fm = get_faction_manager();
                input_value = fm.is_hostile(context.self_faction->faction, 
                                            context.target_faction->faction) ? 1.0f : 0.0f;
            }
            break;
            
        case InputType::IS_TARGET_ALLY:
            if (context.target_faction && context.self_faction) {
                FactionManager& fm = get_faction_manager();
                input_value = fm.is_allied(context.self_faction->faction, 
                                           context.target_faction->faction) ? 1.0f : 0.0f;
            }
            break;
            
        // Environmental inputs
        case InputType::NEARBY_ENEMIES_COUNT:
            input_value = std::min(1.0f, context.nearby_enemies / 8.0f);
            break;
            
        case InputType::NEARBY_ALLIES_COUNT:
            input_value = std::min(1.0f, context.nearby_allies / 8.0f);
            break;
            
        case InputType::ALLIES_UNDER_ATTACK:
            input_value = context.ally_under_attack ? 1.0f : 0.0f;
            break;
            
        // Situational inputs
        case InputType::IN_COMBAT:
            input_value = context.in_combat ? 1.0f : 0.0f;
            break;
            
        case InputType::TIME_SINCE_LAST_ATTACK:
            input_value = std::min(1.0f, context.turns_since_attack / 10.0f);
            break;
            
        case InputType::HAS_TARGET:
            input_value = context.has_target() ? 1.0f : 0.0f;
            break;
            
        case InputType::TARGET_IN_MELEE_RANGE:
            input_value = context.target_in_melee_range() ? 1.0f : 0.0f;
            break;
            
        // Inventory/equipment
        case InputType::HAS_HEALING_ITEM:
            if (context.self_inventory) {
                input_value = !context.self_inventory->items.empty() ? 0.5f : 0.0f;
            }
            break;
            
        case InputType::HAS_RANGED_WEAPON:
            input_value = 0.0f;
            break;
            
        // Home-related inputs
        case InputType::DISTANCE_TO_HOME:
            if (context.has_home) {
                // Normalize distance to home (0 = at home, 1 = far away)
                input_value = std::min(1.0f, context.distance_to_home / 50.0f);
            } else {
                input_value = 0.0f;  // No home, so "at home" everywhere
            }
            break;
            
        case InputType::IS_AT_HOME:
            input_value = context.is_at_home ? 1.0f : 0.0f;
            break;
            
        case InputType::IS_NIGHT_TIME:
            input_value = context.is_night_time ? 1.0f : 0.0f;
            break;
            
        // Custom
        case InputType::CUSTOM:
            if (custom_input) {
                input_value = custom_input(context);
            }
            break;
    }
    
    // Apply response curve
    float curved_value = curve.evaluate(input_value);
    
    // Apply weight and bonus
    return (curved_value * weight) + bonus;
}

inline float UtilityAction::evaluate(const UtilityContext& context) const {
    // Start with base score
    float score = base_score;
    
    // If on cooldown, return 0
    if (is_on_cooldown()) {
        return 0.0f;
    }
    
    // Evaluate all considerations
    if (!considerations.empty()) {
        // Use multiplicative combination with proper handling of weighted values
        float product = 1.0f;
        float weight_sum = 0.0f;
        
        for (const auto& consideration : considerations) {
            float value = consideration.evaluate(context);
            
            // Clamp to avoid negative values (but allow > 1.0 for weighted considerations)
            value = std::max(0.0f, value);
            
            // For values <= 1.0, apply compensation to prevent single 0 from killing score
            // For values > 1.0, use them directly as score boosters
            if (value <= 1.0f) {
                // Small value compensation - prevents a single 0 from zeroing the whole score
                float modification = std::pow(1.0f - (1.0f / static_cast<float>(considerations.size())), 
                                              1.0f - value);
                value = std::max(0.001f, value + (1.0f - value) * modification);
            }
            
            product *= value;
            weight_sum += consideration.weight;
        }
        
        // Apply geometric mean based on number of considerations
        float geo_mean = std::pow(product, 1.0f / static_cast<float>(considerations.size()));
        score += geo_mean;
    }
    
    // Add momentum bonus if this was the previous action
    if (context.last_action == type) {
        score += momentum_bonus;
    }
    
    // Apply final multiplier
    score *= score_multiplier;
    
    return std::max(0.0f, score);
}
