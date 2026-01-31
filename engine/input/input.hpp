#pragma once

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/select.h>
    #include <fcntl.h>
#endif

#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>

enum class Key {
    NONE,
    UP, DOWN, LEFT, RIGHT,
    ESCAPE, ENTER, SPACE,
    W, A, S, D, I, E, Q, H, U, R, C, F, X, Y, N, M,
    TAB, GREATER, LESS, PLUS, MINUS, EQUALS,
    NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,
    B, G, J, K, L, O, P, T, V, Z,
    BRACKET_LEFT, BRACKET_RIGHT,  // [ and ] for scrolling
    PAGE_UP, PAGE_DOWN,           // Page up/down for scrolling
    UNKNOWN
};

// Game actions that can be rebound
enum class GameAction {
    // Movement
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    
    // Gameplay
    INVENTORY,
    CHARACTER,
    INTERACT,       // Pickup items (E key)
    TALK,           // Open interaction menu (F key)
    USE_ITEM,
    DROP_ITEM,
    THROW_ITEM,     // Throw item at nearby entity (T key)
    MAP,
    HELP,
    EXAMINE,
    RANGED_ATTACK,  // Enter ranged targeting mode (R key)
    
    // Menu Navigation
    MENU_UP,
    MENU_DOWN,
    MENU_LEFT,
    MENU_RIGHT,
    CONFIRM,
    CANCEL,
    PAUSE,
    
    // Stairs/Levels
    ASCEND,         // Go up stairs (<)
    DESCEND,        // Go down stairs (>)
    
    // Quick actions
    ZOOM_IN,
    ZOOM_OUT,
    SCROLL_MESSAGES_UP,
    SCROLL_MESSAGES_DOWN,
    
    ACTION_COUNT    // Keep last - used for iteration
};

// Categories for key binding conflict detection
// Keys can be shared between different categories since they're used in different contexts
enum class ActionCategory {
    GAMEPLAY,       // World movement and interactions (WASD, E, F, etc)
    MENU,           // Menu navigation (arrows, enter, escape)
    CAMERA,         // Camera controls (zoom)
    SYSTEM          // System actions (pause, help)
};

// Get the category for an action
inline ActionCategory get_action_category(GameAction action) {
    switch (action) {
        // Gameplay - world movement and interactions
        case GameAction::MOVE_UP:
        case GameAction::MOVE_DOWN:
        case GameAction::MOVE_LEFT:
        case GameAction::MOVE_RIGHT:
        case GameAction::INTERACT:
        case GameAction::TALK:
        case GameAction::EXAMINE:
        case GameAction::RANGED_ATTACK:
        case GameAction::THROW_ITEM:
        case GameAction::ASCEND:
        case GameAction::DESCEND:
            return ActionCategory::GAMEPLAY;
        
        // Menu navigation
        case GameAction::MENU_UP:
        case GameAction::MENU_DOWN:
        case GameAction::MENU_LEFT:
        case GameAction::MENU_RIGHT:
        case GameAction::CONFIRM:
        case GameAction::CANCEL:
        case GameAction::INVENTORY:
        case GameAction::CHARACTER:
        case GameAction::USE_ITEM:
        case GameAction::DROP_ITEM:
        case GameAction::MAP:
            return ActionCategory::MENU;
        
        // Camera
        case GameAction::ZOOM_IN:
        case GameAction::ZOOM_OUT:
        case GameAction::SCROLL_MESSAGES_UP:
        case GameAction::SCROLL_MESSAGES_DOWN:
            return ActionCategory::CAMERA;
        
        // System
        case GameAction::PAUSE:
        case GameAction::HELP:
        default:
            return ActionCategory::SYSTEM;
    }
}

// Get category name for display
inline std::string category_to_string(ActionCategory cat) {
    switch (cat) {
        case ActionCategory::GAMEPLAY: return "GAMEPLAY";
        case ActionCategory::MENU: return "MENU / UI";
        case ActionCategory::CAMERA: return "CAMERA";
        case ActionCategory::SYSTEM: return "SYSTEM";
        default: return "OTHER";
    }
}

// Convert Key to string for display
inline std::string key_to_string(Key key) {
    switch (key) {
        case Key::NONE: return "None";
        case Key::UP: return "Up Arrow";
        case Key::DOWN: return "Down Arrow";
        case Key::LEFT: return "Left Arrow";
        case Key::RIGHT: return "Right Arrow";
        case Key::ESCAPE: return "Escape";
        case Key::ENTER: return "Enter";
        case Key::SPACE: return "Space";
        case Key::TAB: return "Tab";
        case Key::W: return "W";
        case Key::A: return "A";
        case Key::S: return "S";
        case Key::D: return "D";
        case Key::I: return "I";
        case Key::E: return "E";
        case Key::Q: return "Q";
        case Key::H: return "H";
        case Key::U: return "U";
        case Key::R: return "R";
        case Key::C: return "C";
        case Key::F: return "F";
        case Key::X: return "X";
        case Key::Y: return "Y";
        case Key::N: return "N";
        case Key::M: return "M";
        case Key::B: return "B";
        case Key::G: return "G";
        case Key::J: return "J";
        case Key::K: return "K";
        case Key::L: return "L";
        case Key::O: return "O";
        case Key::P: return "P";
        case Key::T: return "T";
        case Key::V: return "V";
        case Key::Z: return "Z";
        case Key::GREATER: return ">";
        case Key::LESS: return "<";
        case Key::PLUS: return "+";
        case Key::MINUS: return "-";
        case Key::EQUALS: return "=";
        case Key::NUM_1: return "1";
        case Key::NUM_2: return "2";
        case Key::NUM_3: return "3";
        case Key::NUM_4: return "4";
        case Key::NUM_5: return "5";
        case Key::NUM_6: return "6";
        case Key::NUM_7: return "7";
        case Key::NUM_8: return "8";
        case Key::NUM_9: return "9";
        case Key::BRACKET_LEFT: return "[";
        case Key::BRACKET_RIGHT: return "]";
        case Key::PAGE_UP: return "Page Up";
        case Key::PAGE_DOWN: return "Page Down";
        default: return "Unknown";
    }
}

// Convert Key to short string for compact UI display
inline std::string key_to_short_string(Key key) {
    switch (key) {
        case Key::NONE: return "";
        case Key::UP: return "Up";
        case Key::DOWN: return "Dn";
        case Key::LEFT: return "Lt";
        case Key::RIGHT: return "Rt";
        case Key::ESCAPE: return "Esc";
        case Key::ENTER: return "Ent";
        case Key::SPACE: return "Spc";
        case Key::TAB: return "Tab";
        case Key::GREATER: return ">";
        case Key::LESS: return "<";
        case Key::PLUS: return "+";
        case Key::MINUS: return "-";
        case Key::EQUALS: return "=";
        case Key::BRACKET_LEFT: return "[";
        case Key::BRACKET_RIGHT: return "]";
        case Key::PAGE_UP: return "PgUp";
        case Key::PAGE_DOWN: return "PgDn";
        default: return key_to_string(key);  // Single letters and numbers use full version
    }
}

// Convert GameAction to display name
inline std::string action_to_string(GameAction action) {
    switch (action) {
        // Movement
        case GameAction::MOVE_UP: return "Move Up";
        case GameAction::MOVE_DOWN: return "Move Down";
        case GameAction::MOVE_LEFT: return "Move Left";
        case GameAction::MOVE_RIGHT: return "Move Right";
        
        // Gameplay
        case GameAction::INVENTORY: return "Inventory";
        case GameAction::CHARACTER: return "Character Stats";
        case GameAction::INTERACT: return "Interact/Pickup";
        case GameAction::TALK: return "Talk to NPC";
        case GameAction::USE_ITEM: return "Use Item";
        case GameAction::DROP_ITEM: return "Drop Item";
        case GameAction::THROW_ITEM: return "Throw Item";
        case GameAction::MAP: return "World Map";
        case GameAction::HELP: return "Help";
        case GameAction::EXAMINE: return "Examine";
        case GameAction::RANGED_ATTACK: return "Ranged Attack";
        
        // Menu Navigation
        case GameAction::MENU_UP: return "Menu Up";
        case GameAction::MENU_DOWN: return "Menu Down";
        case GameAction::MENU_LEFT: return "Menu Left";
        case GameAction::MENU_RIGHT: return "Menu Right";
        case GameAction::CONFIRM: return "Confirm/Select";
        case GameAction::CANCEL: return "Cancel/Back";
        case GameAction::PAUSE: return "Pause Game";
        
        // Stairs
        case GameAction::ASCEND: return "Go Up Stairs";
        case GameAction::DESCEND: return "Go Down Stairs";
        
        // Quick actions
        case GameAction::ZOOM_IN: return "Zoom In";
        case GameAction::ZOOM_OUT: return "Zoom Out";
        case GameAction::SCROLL_MESSAGES_UP: return "Scroll Messages Up";
        case GameAction::SCROLL_MESSAGES_DOWN: return "Scroll Messages Down";
        
        default: return "Unknown";
    }
}

// Convert string to GameAction (for loading keybindings)
inline GameAction string_to_action(const std::string& str) {
    if (str == "MOVE_UP") return GameAction::MOVE_UP;
    if (str == "MOVE_DOWN") return GameAction::MOVE_DOWN;
    if (str == "MOVE_LEFT") return GameAction::MOVE_LEFT;
    if (str == "MOVE_RIGHT") return GameAction::MOVE_RIGHT;
    if (str == "INVENTORY") return GameAction::INVENTORY;
    if (str == "CHARACTER") return GameAction::CHARACTER;
    if (str == "INTERACT") return GameAction::INTERACT;
    if (str == "TALK") return GameAction::TALK;
    if (str == "USE_ITEM") return GameAction::USE_ITEM;
    if (str == "DROP_ITEM") return GameAction::DROP_ITEM;
    if (str == "THROW_ITEM") return GameAction::THROW_ITEM;
    if (str == "MAP") return GameAction::MAP;
    if (str == "HELP") return GameAction::HELP;
    if (str == "EXAMINE") return GameAction::EXAMINE;
    if (str == "RANGED_ATTACK") return GameAction::RANGED_ATTACK;
    if (str == "MENU_UP") return GameAction::MENU_UP;
    if (str == "MENU_DOWN") return GameAction::MENU_DOWN;
    if (str == "MENU_LEFT") return GameAction::MENU_LEFT;
    if (str == "MENU_RIGHT") return GameAction::MENU_RIGHT;
    if (str == "CONFIRM") return GameAction::CONFIRM;
    if (str == "CANCEL") return GameAction::CANCEL;
    if (str == "PAUSE") return GameAction::PAUSE;
    if (str == "ASCEND") return GameAction::ASCEND;
    if (str == "DESCEND") return GameAction::DESCEND;
    if (str == "ZOOM_IN") return GameAction::ZOOM_IN;
    if (str == "ZOOM_OUT") return GameAction::ZOOM_OUT;
    if (str == "SCROLL_MESSAGES_UP") return GameAction::SCROLL_MESSAGES_UP;
    if (str == "SCROLL_MESSAGES_DOWN") return GameAction::SCROLL_MESSAGES_DOWN;
    return GameAction::ACTION_COUNT;  // Invalid
}

// Convert GameAction to string for saving (stable across enum reordering)
inline std::string action_to_save_string(GameAction action) {
    switch (action) {
        case GameAction::MOVE_UP: return "MOVE_UP";
        case GameAction::MOVE_DOWN: return "MOVE_DOWN";
        case GameAction::MOVE_LEFT: return "MOVE_LEFT";
        case GameAction::MOVE_RIGHT: return "MOVE_RIGHT";
        case GameAction::INVENTORY: return "INVENTORY";
        case GameAction::CHARACTER: return "CHARACTER";
        case GameAction::INTERACT: return "INTERACT";
        case GameAction::TALK: return "TALK";
        case GameAction::USE_ITEM: return "USE_ITEM";
        case GameAction::DROP_ITEM: return "DROP_ITEM";
        case GameAction::THROW_ITEM: return "THROW_ITEM";
        case GameAction::MAP: return "MAP";
        case GameAction::HELP: return "HELP";
        case GameAction::EXAMINE: return "EXAMINE";
        case GameAction::RANGED_ATTACK: return "RANGED_ATTACK";
        case GameAction::MENU_UP: return "MENU_UP";
        case GameAction::MENU_DOWN: return "MENU_DOWN";
        case GameAction::MENU_LEFT: return "MENU_LEFT";
        case GameAction::MENU_RIGHT: return "MENU_RIGHT";
        case GameAction::CONFIRM: return "CONFIRM";
        case GameAction::CANCEL: return "CANCEL";
        case GameAction::PAUSE: return "PAUSE";
        case GameAction::ASCEND: return "ASCEND";
        case GameAction::DESCEND: return "DESCEND";
        case GameAction::ZOOM_IN: return "ZOOM_IN";
        case GameAction::ZOOM_OUT: return "ZOOM_OUT";
        case GameAction::SCROLL_MESSAGES_UP: return "SCROLL_MESSAGES_UP";
        case GameAction::SCROLL_MESSAGES_DOWN: return "SCROLL_MESSAGES_DOWN";
        default: return "UNKNOWN";
    }
}

// Key Bindings Manager - singleton for managing key bindings
class KeyBindings {
private:
    // Primary and secondary bindings for each action
    std::unordered_map<GameAction, Key> primary_bindings;
    std::unordered_map<GameAction, Key> secondary_bindings;
    
    // Reverse lookup: key -> action
    std::unordered_map<Key, GameAction> key_to_action_primary;
    std::unordered_map<Key, GameAction> key_to_action_secondary;
    
    KeyBindings() {
        reset_to_defaults();
        load();  // Load saved keybindings (if any)
    }
    
public:
    static KeyBindings& instance() {
        static KeyBindings instance;
        return instance;
    }
    
    // Reset all bindings to defaults
    void reset_to_defaults() {
        primary_bindings.clear();
        secondary_bindings.clear();
        
        // Movement - primary: WASD, secondary: arrow keys
        primary_bindings[GameAction::MOVE_UP] = Key::W;
        primary_bindings[GameAction::MOVE_DOWN] = Key::S;
        primary_bindings[GameAction::MOVE_LEFT] = Key::A;
        primary_bindings[GameAction::MOVE_RIGHT] = Key::D;
        
        secondary_bindings[GameAction::MOVE_UP] = Key::UP;
        secondary_bindings[GameAction::MOVE_DOWN] = Key::DOWN;
        secondary_bindings[GameAction::MOVE_LEFT] = Key::LEFT;
        secondary_bindings[GameAction::MOVE_RIGHT] = Key::RIGHT;
        
        // Gameplay actions
        primary_bindings[GameAction::INVENTORY] = Key::I;
        primary_bindings[GameAction::CHARACTER] = Key::C;
        primary_bindings[GameAction::INTERACT] = Key::E;  // Pickup items
        primary_bindings[GameAction::TALK] = Key::F;      // Open interaction menu
        primary_bindings[GameAction::USE_ITEM] = Key::U;
        primary_bindings[GameAction::DROP_ITEM] = Key::G; // Drop item (changed from R)
        primary_bindings[GameAction::THROW_ITEM] = Key::T; // Throw item at target
        primary_bindings[GameAction::MAP] = Key::M;
        primary_bindings[GameAction::HELP] = Key::H;
        primary_bindings[GameAction::EXAMINE] = Key::X;
        primary_bindings[GameAction::RANGED_ATTACK] = Key::R; // Ranged attack targeting
        
        // Menu navigation - primary: WASD, secondary: arrow keys
        primary_bindings[GameAction::MENU_UP] = Key::W;
        primary_bindings[GameAction::MENU_DOWN] = Key::S;
        primary_bindings[GameAction::MENU_LEFT] = Key::A;
        primary_bindings[GameAction::MENU_RIGHT] = Key::D;
        
        secondary_bindings[GameAction::MENU_UP] = Key::UP;
        secondary_bindings[GameAction::MENU_DOWN] = Key::DOWN;
        secondary_bindings[GameAction::MENU_LEFT] = Key::LEFT;
        secondary_bindings[GameAction::MENU_RIGHT] = Key::RIGHT;
        
        // Confirm/Cancel
        primary_bindings[GameAction::CONFIRM] = Key::ENTER;
        primary_bindings[GameAction::CANCEL] = Key::ESCAPE;
        primary_bindings[GameAction::PAUSE] = Key::ESCAPE;
        
        secondary_bindings[GameAction::CONFIRM] = Key::SPACE;
        secondary_bindings[GameAction::CANCEL] = Key::N;
        
        // Stairs
        primary_bindings[GameAction::ASCEND] = Key::LESS;
        primary_bindings[GameAction::DESCEND] = Key::GREATER;
        
        // Zoom (for map)
        primary_bindings[GameAction::ZOOM_IN] = Key::PLUS;
        primary_bindings[GameAction::ZOOM_OUT] = Key::MINUS;
        
        secondary_bindings[GameAction::ZOOM_IN] = Key::EQUALS;
        
        // Message log scrolling
        primary_bindings[GameAction::SCROLL_MESSAGES_UP] = Key::BRACKET_LEFT;
        primary_bindings[GameAction::SCROLL_MESSAGES_DOWN] = Key::BRACKET_RIGHT;
        
        rebuild_reverse_lookup();
    }
    
    void rebuild_reverse_lookup() {
        key_to_action_primary.clear();
        key_to_action_secondary.clear();
        
        for (const auto& pair : primary_bindings) {
            if (pair.second != Key::NONE) {
                key_to_action_primary[pair.second] = pair.first;
            }
        }
        for (const auto& pair : secondary_bindings) {
            if (pair.second != Key::NONE) {
                key_to_action_secondary[pair.second] = pair.first;
            }
        }
    }
    
    // Set a binding (only clears conflicts within the same category)
    void set_primary(GameAction action, Key key) {
        ActionCategory category = get_action_category(action);
        // Remove old binding for this key if it exists IN THE SAME CATEGORY
        for (auto& pair : primary_bindings) {
            if (pair.second == key && pair.first != action && 
                get_action_category(pair.first) == category) {
                pair.second = Key::NONE;
            }
        }
        primary_bindings[action] = key;
        rebuild_reverse_lookup();
    }
    
    void set_secondary(GameAction action, Key key) {
        ActionCategory category = get_action_category(action);
        for (auto& pair : secondary_bindings) {
            if (pair.second == key && pair.first != action &&
                get_action_category(pair.first) == category) {
                pair.second = Key::NONE;
            }
        }
        secondary_bindings[action] = key;
        rebuild_reverse_lookup();
    }
    
    // Get bindings
    Key get_primary(GameAction action) const {
        auto it = primary_bindings.find(action);
        return (it != primary_bindings.end()) ? it->second : Key::NONE;
    }
    
    Key get_secondary(GameAction action) const {
        auto it = secondary_bindings.find(action);
        return (it != secondary_bindings.end()) ? it->second : Key::NONE;
    }
    
    // Check if a key is bound to an action
    // Note: Multiple actions can share the same key (e.g., CANCEL and PAUSE both use ESC)
    bool is_action(Key key, GameAction action) const {
        if (key == Key::NONE) return false;
        
        // Check primary binding for this action
        auto it1 = primary_bindings.find(action);
        if (it1 != primary_bindings.end() && it1->second == key) return true;
        
        // Check secondary binding for this action
        auto it2 = secondary_bindings.find(action);
        if (it2 != secondary_bindings.end() && it2->second == key) return true;
        
        return false;
    }
    
    // Get action for a key (returns first match)
    GameAction get_action(Key key) const {
        auto it1 = key_to_action_primary.find(key);
        if (it1 != key_to_action_primary.end()) return it1->second;
        
        auto it2 = key_to_action_secondary.find(key);
        if (it2 != key_to_action_secondary.end()) return it2->second;
        
        return GameAction::ACTION_COUNT;  // Invalid action
    }
    
    // Get actions organized by category for display
    struct CategoryGroup {
        ActionCategory category;
        std::vector<GameAction> actions;
    };
    
    std::vector<CategoryGroup> get_actions_by_category() const {
        return {
            { ActionCategory::GAMEPLAY, {
                GameAction::MOVE_UP,
                GameAction::MOVE_DOWN,
                GameAction::MOVE_LEFT,
                GameAction::MOVE_RIGHT,
                GameAction::INTERACT,
                GameAction::TALK,
                GameAction::EXAMINE,
                GameAction::RANGED_ATTACK,
                GameAction::THROW_ITEM,
                GameAction::ASCEND,
                GameAction::DESCEND
            }},
            { ActionCategory::MENU, {
                GameAction::MENU_UP,
                GameAction::MENU_DOWN,
                GameAction::MENU_LEFT,
                GameAction::MENU_RIGHT,
                GameAction::CONFIRM,
                GameAction::CANCEL,
                GameAction::INVENTORY,
                GameAction::CHARACTER,
                GameAction::USE_ITEM,
                GameAction::DROP_ITEM,
                GameAction::MAP
            }},
            { ActionCategory::CAMERA, {
                GameAction::ZOOM_IN,
                GameAction::ZOOM_OUT,
                GameAction::SCROLL_MESSAGES_UP,
                GameAction::SCROLL_MESSAGES_DOWN
            }},
            { ActionCategory::SYSTEM, {
                GameAction::PAUSE,
                GameAction::HELP
            }}
        };
    }
    
    // Get a short key hint string for a single action (shows primary/secondary)
    std::string get_key_hint(GameAction action) const {
        Key pri = get_primary(action);
        Key sec = get_secondary(action);
        if (pri != Key::NONE && sec != Key::NONE) {
            return key_to_short_string(pri) + "/" + key_to_short_string(sec);
        } else if (pri != Key::NONE) {
            return key_to_short_string(pri);
        } else if (sec != Key::NONE) {
            return key_to_short_string(sec);
        }
        return "?";
    }
    
    // Get navigation hint string (MENU_UP/MENU_DOWN combined)
    std::string get_nav_keys() const {
        Key up_pri = get_primary(GameAction::MENU_UP);
        Key down_pri = get_primary(GameAction::MENU_DOWN);
        Key up_sec = get_secondary(GameAction::MENU_UP);
        Key down_sec = get_secondary(GameAction::MENU_DOWN);
        
        std::string result;
        if (up_pri != Key::NONE && down_pri != Key::NONE) {
            result = key_to_short_string(up_pri) + "/" + key_to_short_string(down_pri);
        }
        if (up_sec != Key::NONE && down_sec != Key::NONE) {
            if (!result.empty()) result += " or ";
            result += key_to_short_string(up_sec) + "/" + key_to_short_string(down_sec);
        }
        return result.empty() ? "?" : result;
    }
    
    // Get flat list of all rebindable actions (for backward compatibility)
    std::vector<GameAction> get_all_actions() const {
        std::vector<GameAction> all;
        for (const auto& group : get_actions_by_category()) {
            for (const auto& action : group.actions) {
                all.push_back(action);
            }
        }
        return all;
    }
    
    // Save bindings to file (using action names for stability)
    void save(const std::string& filename = "keybindings.cfg") {
        std::ofstream file(filename);
        if (!file) return;
        
        // Write version header so we can detect old format
        file << "version 2\n";
        
        for (const auto& pair : primary_bindings) {
            file << "primary " << action_to_save_string(pair.first) << " " << static_cast<int>(pair.second) << "\n";
        }
        for (const auto& pair : secondary_bindings) {
            file << "secondary " << action_to_save_string(pair.first) << " " << static_cast<int>(pair.second) << "\n";
        }
    }
    
    // Load bindings from file
    void load(const std::string& filename = "keybindings.cfg") {
        std::ifstream file(filename);
        if (!file) return;
        
        std::string first_word;
        file >> first_word;
        
        // Check for version header
        if (first_word == "version") {
            int version;
            file >> version;
            
            if (version >= 2) {
                // New format: action names as strings
                std::string type, action_str;
                int key_int;
                
                while (file >> type >> action_str >> key_int) {
                    GameAction action = string_to_action(action_str);
                    Key key = static_cast<Key>(key_int);
                    
                    if (action != GameAction::ACTION_COUNT) {
                        if (type == "primary") {
                            primary_bindings[action] = key;
                        } else if (type == "secondary") {
                            secondary_bindings[action] = key;
                        }
                    }
                }
            }
        } else {
            // Old format (integers) - discard and use defaults
            // Just close the file and keep defaults from reset_to_defaults()
        }
        
        rebuild_reverse_lookup();
    }
};

class Input {
private:
#ifndef _WIN32
    static struct termios orig_termios;
    static bool termios_saved;
    static bool raw_mode_enabled;
    
    static void disable_raw_mode() {
        if (termios_saved && raw_mode_enabled) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            raw_mode_enabled = false;
        }
    }
    
    static void enable_raw_mode() {
        if (!termios_saved) {
            tcgetattr(STDIN_FILENO, &orig_termios);
            atexit(disable_raw_mode);
            termios_saved = true;
        }
        
        if (!raw_mode_enabled) {
            struct termios raw = orig_termios;
            raw.c_lflag &= ~(ECHO | ICANON);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            raw_mode_enabled = true;
        }
    }
    
    static int kbhit() {
        enable_raw_mode();
        struct timeval tv = { 0L, 0L };
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
    }
    
    static int getch_nonblock() {
        enable_raw_mode();
        unsigned char buf = 0;
        if (read(STDIN_FILENO, &buf, 1) < 0) {
            return -1;
        }
        return buf;
    }
#endif

public:
    static Key get_key() {
#ifdef _WIN32
        if (!_kbhit()) {
            return Key::NONE;
        }
        
        int ch = _getch();
        
        if (ch == 0 || ch == 224) {
            ch = _getch();
            switch (ch) {
                case 72: return Key::UP;
                case 80: return Key::DOWN;
                case 75: return Key::LEFT;
                case 77: return Key::RIGHT;
                default: return Key::UNKNOWN;
            }
        }
#else
        if (!kbhit()) {
            return Key::NONE;
        }
        
        int ch = getch_nonblock();
        if (ch < 0) return Key::NONE;
        
        if (ch == 27) { // ESC or arrow key
            // Check if there's more data (arrow keys send escape sequences)
            usleep(1000); // Small delay to let sequence complete
            if (kbhit()) {
                int ch2 = getch_nonblock();
                if (ch2 == '[') {
                    int ch3 = getch_nonblock();
                    switch (ch3) {
                        case 'A': return Key::UP;
                        case 'B': return Key::DOWN;
                        case 'C': return Key::RIGHT;
                        case 'D': return Key::LEFT;
                        default: return Key::UNKNOWN;
                    }
                }
            }
            return Key::ESCAPE;
        }
#endif
        
        switch (ch) {
            case 27: return Key::ESCAPE;
            case 13: case 10: return Key::ENTER;
            case ' ': return Key::SPACE;
            case 'w': case 'W': return Key::W;
            case 'a': case 'A': return Key::A;
            case 's': case 'S': return Key::S;
            case 'd': case 'D': return Key::D;
            case 'i': case 'I': return Key::I;
            case 'e': case 'E': return Key::E;
            case 'q': case 'Q': return Key::Q;
            case 'h': case 'H': return Key::H;
            case 'u': case 'U': return Key::U;
            case 'r': case 'R': return Key::R;
            case 'c': case 'C': return Key::C;
            case 'f': case 'F': return Key::F;
            case 'x': case 'X': return Key::X;
            case 'y': case 'Y': return Key::Y;
            case 'n': case 'N': return Key::N;
            case 'm': case 'M': return Key::M;
            case 'b': case 'B': return Key::B;
            case 'g': case 'G': return Key::G;
            case 'j': case 'J': return Key::J;
            case 'k': case 'K': return Key::K;
            case 'l': case 'L': return Key::L;
            case 'o': case 'O': return Key::O;
            case 'p': case 'P': return Key::P;
            case 't': case 'T': return Key::T;
            case 'v': case 'V': return Key::V;
            case 'z': case 'Z': return Key::Z;
            case '\t': return Key::TAB;
            case '>': return Key::GREATER;
            case '<': return Key::LESS;
            case '+': return Key::PLUS;
            case '-': return Key::MINUS;
            case '=': return Key::EQUALS;
            case '1': return Key::NUM_1;
            case '2': return Key::NUM_2;
            case '3': return Key::NUM_3;
            case '4': return Key::NUM_4;
            case '5': return Key::NUM_5;
            case '6': return Key::NUM_6;
            case '7': return Key::NUM_7;
            case '8': return Key::NUM_8;
            case '9': return Key::NUM_9;
            case '[': return Key::BRACKET_LEFT;
            case ']': return Key::BRACKET_RIGHT;
            default: return Key::UNKNOWN;
        }
    }
    
    static Key wait_for_key() {
        Key key;
        do {
            key = get_key();
            if (key != Key::NONE) break;
#ifdef _WIN32
            Sleep(10);
#else
            usleep(10000);
#endif
        } while (true);
        return key;
    }
};

#ifndef _WIN32
inline struct termios Input::orig_termios;
inline bool Input::termios_saved = false;
inline bool Input::raw_mode_enabled = false;
#endif
