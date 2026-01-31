#pragma once
#include "console.hpp"
#include "../ecs/component_manager.hpp"
#include "../ecs/component.hpp"
#include "../input/input.hpp"
#include <string>
#include <vector>
#include <functional>

// Forward declarations
struct ShopItem;

// Result from interaction window
struct InteractionResult {
    InteractionType type = InteractionType::EXAMINE;
    bool cancelled = false;
    EntityId target_id = 0;
    
    InteractionResult() = default;
    InteractionResult(InteractionType t, EntityId target, bool cancel = false)
        : type(t), cancelled(cancel), target_id(target) {}
};

class UI {
private:
    Console* console;
    ComponentManager* manager;
    
public:
    UI(Console* con, ComponentManager* mgr) : console(con), manager(mgr) {}
    
    // Trade result for shop interactions
    struct TradeResult {
        enum class Action { BOUGHT, SOLD, CANCELLED };
        Action action;
        size_t item_index;
        int gold_change;
    };
    
    // Draw a panel/window
    void draw_panel(int x, int y, int width, int height, const std::string& title = "", 
                   Color fg = Color::WHITE, Color bg = Color::BLACK);
    
    // Draw text with word wrap
    void draw_text(int x, int y, int max_width, const std::string& text, 
                  Color fg = Color::WHITE, Color bg = Color::BLACK);
    
    // Draw a health bar
    void draw_health_bar(int x, int y, int width, int current, int max, 
                        Color full_color = Color::GREEN, Color empty_color = Color::RED);
    
    // Draw stats for an entity
    void draw_stats(int x, int y, EntityId entity_id);
    
    // Draw inventory
    void draw_inventory(int x, int y, int width, int height, EntityId entity_id);
    
    // Draw equipment panel showing dynamic limb-based slots
    void draw_equipment(int x, int y, int width, int height, EntityId entity_id);
    
    // Draw message log (scroll_offset = 0 shows latest messages, positive values scroll up into history)
    void draw_message_log(int x, int y, int width, int height, const std::vector<std::string>& messages, int scroll_offset = 0, const KeyBindings* bindings = nullptr);
    
    // Draw a menu
    int draw_menu(int x, int y, const std::string& title, const std::vector<std::string>& options, 
                  int selected_index);
    
    // Draw HUD (Heads-Up Display) - Side panel on the right
    void draw_hud(EntityId player_id, int screen_width, int screen_height);
    
    // Draw game over screen
    void draw_game_over(bool victory, const std::string& death_reason = "");
    
    // Show interaction window for an entity
    InteractionResult show_interaction_window(EntityId target_id, std::function<void()> render_background = nullptr);
    
    // Show target selection when multiple entities are nearby
    EntityId show_target_selection(const std::vector<EntityId>& nearby_entities, 
                                   std::function<void()> render_background = nullptr);
    
    // Show trade interface
    TradeResult show_trade_interface(EntityId player_id, EntityId merchant_id, 
                                     std::function<void()> render_background = nullptr,
                                     std::function<EntityId(const ShopItem&, int, int)> create_item_callback = nullptr);
    
    // Simple confirmation dialog
    bool confirm_action(const std::string& message);
    
private:
    enum class TradeMode { BUY, SELL };
};
