#pragma once
#include "component_base.hpp"
#include <vector>
#include <string>

// Interaction type enum for the interaction window
enum class InteractionType {
    NONE,
    TALK,
    TRADE,
    STEAL,
    ATTACK,
    EXAMINE,
    OPEN,
    PICKUP,
    USE,
    CUSTOM
};

// Represents a single interaction option in the interaction window
struct InteractionOption {
    InteractionType type;
    std::string label;
    std::string description;
    bool enabled = true;
    std::string disabled_reason;
    
    InteractionOption() = default;
    InteractionOption(InteractionType type, const std::string& label, const std::string& desc = "", bool enabled = true)
        : type(type), label(label), description(desc), enabled(enabled) {}
};

// NPC component - defines NPC-specific behaviors
struct NPCComponent : Component {
    enum class Disposition {
        FRIENDLY,
        NEUTRAL,
        HOSTILE,
        FEARFUL
    };
    
    enum class Role {
        VILLAGER,
        MERCHANT,
        GUARD,
        QUEST_GIVER,
        TRAINER,
        INNKEEPER,
        WANDERER
    };
    
    Disposition disposition = Disposition::NEUTRAL;
    Role role = Role::VILLAGER;
    bool can_trade = false;
    bool can_give_quests = false;
    std::string greeting;        // What they say when you first interact
    std::string farewell;        // What they say when dialogue ends
    int relationship = 0;        // Player's standing with this NPC (-100 to 100)
    
    // Available interaction types for this NPC
    std::vector<InteractionOption> available_interactions;
    
    NPCComponent() = default;
    NPCComponent(Disposition disp, Role role = Role::VILLAGER)
        : disposition(disp), role(role) {
        setup_default_interactions();
    }
    
    void setup_default_interactions() {
        available_interactions.clear();
        
        // All NPCs can be talked to (examine is now a separate keybind)
        available_interactions.emplace_back(InteractionType::TALK, "Talk", "Start a conversation");
        
        // Role-specific interactions
        switch (role) {
            case Role::MERCHANT:
                can_trade = true;
                available_interactions.emplace_back(InteractionType::TRADE, "Trade", "Buy or sell items");
                available_interactions.emplace_back(InteractionType::STEAL, "Steal", "Attempt to pickpocket", true);
                break;
            case Role::INNKEEPER:
                can_trade = true;
                available_interactions.emplace_back(InteractionType::TRADE, "Trade", "Rent a room or buy food");
                break;
            case Role::QUEST_GIVER:
                can_give_quests = true;
                break;
            case Role::GUARD:
                // Guards are harder to steal from
                available_interactions.emplace_back(InteractionType::STEAL, "Steal", "Risky - guards are vigilant", true);
                break;
            case Role::TRAINER:
                available_interactions.emplace_back(InteractionType::TRADE, "Train", "Learn new skills");
                break;
            default:
                // Regular villagers can be stolen from
                available_interactions.emplace_back(InteractionType::STEAL, "Steal", "Attempt to pickpocket", true);
                break;
        }
        
        // Hostile option (attacking) - usually last
        if (disposition != Disposition::HOSTILE) {
            available_interactions.emplace_back(InteractionType::ATTACK, "Attack", "Initiate combat");
        }
    }
    
    void add_interaction(InteractionType type, const std::string& label, const std::string& desc = "", bool enabled = true) {
        available_interactions.emplace_back(type, label, desc, enabled);
    }
    
    void set_interaction_enabled(InteractionType type, bool enabled, const std::string& reason = "") {
        for (auto& interaction : available_interactions) {
            if (interaction.type == type) {
                interaction.enabled = enabled;
                interaction.disabled_reason = reason;
            }
        }
    }
    IMPLEMENT_CLONE(NPCComponent)
};
