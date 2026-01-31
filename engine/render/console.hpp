#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>
#endif

// Console color codes
enum class Color {
    BLACK = 0,
    DARK_BLUE = 1,
    DARK_GREEN = 2,
    DARK_CYAN = 3,
    DARK_RED = 4,
    DARK_MAGENTA = 5,
    DARK_YELLOW = 6,
    GRAY = 7,
    DARK_GRAY = 8,
    BLUE = 9,
    GREEN = 10,
    CYAN = 11,
    RED = 12,
    MAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
};

// RGB color structure for true color support
struct RGBColor {
    unsigned char r, g, b;
    
    RGBColor(unsigned char red = 255, unsigned char green = 255, unsigned char blue = 255)
        : r(red), g(green), b(blue) {}
    
    // Create from hex value (e.g., 0xFF5733)
    static RGBColor from_hex(unsigned int hex) {
        return RGBColor((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
    }
    
    // Convert enum Color to RGB approximation
    static RGBColor from_color(Color c);
};

// Convert string color name to Color enum
inline Color string_to_color(const std::string& color_name) {
    if (color_name == "black") return Color::BLACK;
    if (color_name == "blue") return Color::BLUE;
    if (color_name == "green") return Color::GREEN;
    if (color_name == "cyan") return Color::CYAN;
    if (color_name == "red") return Color::RED;
    if (color_name == "magenta") return Color::MAGENTA;
    if (color_name == "yellow") return Color::YELLOW;
    if (color_name == "white") return Color::WHITE;
    if (color_name == "gray") return Color::GRAY;
    if (color_name == "dark_gray") return Color::DARK_GRAY;
    if (color_name == "brown") return Color::DARK_YELLOW;
    if (color_name == "dark_green") return Color::DARK_GREEN;
    if (color_name == "dark_blue") return Color::DARK_BLUE;
    if (color_name == "dark_cyan") return Color::DARK_CYAN;
    if (color_name == "dark_red") return Color::DARK_RED;
    if (color_name == "dark_yellow") return Color::DARK_YELLOW;
    if (color_name == "dark_magenta") return Color::DARK_MAGENTA;
    return Color::WHITE;
}

// Cell structure for console buffer
struct Cell {
    std::string character = " ";  // Changed to string to support UTF-8
    Color foreground = Color::WHITE;
    Color background = Color::BLACK;
    RGBColor rgb_fg = RGBColor(255, 255, 255);
    RGBColor rgb_bg = RGBColor(0, 0, 0);
    bool use_rgb = false; // Flag to use RGB instead of enum colors
    
    Cell() = default;
    Cell(char ch, Color fg = Color::WHITE, Color bg = Color::BLACK)
        : character(1, ch), foreground(fg), background(bg), use_rgb(false) {}
    
    Cell(char ch, RGBColor fg, RGBColor bg)
        : character(1, ch), rgb_fg(fg), rgb_bg(bg), use_rgb(true) {}
    
    Cell(const std::string& str, Color fg = Color::WHITE, Color bg = Color::BLACK)
        : character(str), foreground(fg), background(bg), use_rgb(false) {}
    
    Cell(const std::string& str, RGBColor fg, RGBColor bg)
        : character(str), rgb_fg(fg), rgb_bg(bg), use_rgb(true) {}
};

// ANSI color code helper
const char* get_ansi_color(Color fg, Color bg);

// ANSI RGB color code helper
const char* get_ansi_rgb_color(const RGBColor& fg, const RGBColor& bg);

// Cross-platform Console wrapper
class Console {
private:
#ifdef _WIN32
    HANDLE hConsole;
    HANDLE hBuffer;
    COORD bufferSize;
    COORD bufferCoord;
    SMALL_RECT writeRegion;
#endif
    int width;
    int height;
    std::vector<Cell> buffer;
    
public:
    Console(int w = 100, int h = 30);
    ~Console();
    
    void clear(Color bg = Color::BLACK);
    void set_cell(int x, int y, char ch, Color fg = Color::WHITE, Color bg = Color::BLACK);
    void set_cell_rgb(int x, int y, char ch, RGBColor fg, RGBColor bg);
    void set_cell(int x, int y, const Cell& cell);
    Cell get_cell(int x, int y) const;
    void draw_string(int x, int y, const std::string& str, Color fg = Color::WHITE, Color bg = Color::BLACK);
    void draw_string_rgb(int x, int y, const std::string& str, RGBColor fg, RGBColor bg = RGBColor(0, 0, 0));
    void draw_box(int x, int y, int w, int h, Color fg = Color::WHITE, Color bg = Color::BLACK,
                  char horizontal = '-', char vertical = '|',
                  char corner_tl = '+', char corner_tr = '+',
                  char corner_bl = '+', char corner_br = '+');
    void draw_box_rgb(int x, int y, int w, int h, RGBColor fg, RGBColor bg = RGBColor(0, 0, 0),
                      char horizontal = '-', char vertical = '|',
                      char corner_tl = '+', char corner_tr = '+',
                      char corner_bl = '+', char corner_br = '+');
    void fill_rect(int x, int y, int w, int h, char ch = ' ', Color fg = Color::WHITE, Color bg = Color::BLACK);
    void present();
    
    int get_width() const { return width; }
    int get_height() const { return height; }
};
