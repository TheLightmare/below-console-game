#pragma once
#include "component_base.hpp"
#include <vector>
#include <string>

// Dialogue option
struct DialogueOption {
    std::string text;           // What the player sees as an option
    int next_node_id = -1;      // Which dialogue node this leads to (-1 = end dialogue)
    std::string action;         // Optional action to trigger (e.g., "trade", "give_quest", "give_item")
    std::string condition;      // Optional condition to show this option
    bool ends_dialogue = false; // Whether this option ends the conversation
    
    DialogueOption() = default;
    DialogueOption(const std::string& text, int next_node = -1, bool ends = false,
                  const std::string& action = "", const std::string& condition = "")
        : text(text), next_node_id(next_node), action(action), condition(condition), ends_dialogue(ends) {}
};

// Dialogue node (a single "page" of dialogue)
struct DialogueNode {
    int id = 0;
    std::string speaker_name;    // Who is speaking (usually NPC name)
    std::string text;            // The dialogue text
    std::vector<DialogueOption> options;
    
    DialogueNode() = default;
    DialogueNode(int id, const std::string& speaker, const std::string& text)
        : id(id), speaker_name(speaker), text(text) {}
    
    void add_option(const std::string& text, int next_node = -1, bool ends = false,
                   const std::string& action = "") {
        options.emplace_back(text, next_node, ends, action);
    }
};

// Dialogue component - holds all dialogue data for an NPC
struct DialogueComponent : Component {
    std::vector<DialogueNode> nodes;
    int start_node_id = 0;       // Which node to start from
    int current_node_id = 0;     // Current position in dialogue
    bool has_talked = false;     // Whether player has talked to this NPC before
    
    DialogueComponent() = default;
    
    void add_node(const DialogueNode& node) {
        nodes.push_back(node);
    }
    
    DialogueNode* get_node(int id) {
        for (auto& node : nodes) {
            if (node.id == id) {
                return &node;
            }
        }
        return nullptr;
    }
    
    DialogueNode* get_start_node() {
        return get_node(start_node_id);
    }
    
    DialogueNode* get_current_node() {
        return get_node(current_node_id);
    }
    
    void reset() {
        current_node_id = start_node_id;
    }
    IMPLEMENT_CLONE(DialogueComponent)
};
