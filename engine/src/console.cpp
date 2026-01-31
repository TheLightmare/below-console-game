#include "../render/console.hpp"

// RGBColor static method
RGBColor RGBColor::from_color(Color c) {
    switch (c) {
        case Color::BLACK: return RGBColor(0, 0, 0);
        case Color::DARK_BLUE: return RGBColor(0, 0, 128);
        case Color::DARK_GREEN: return RGBColor(0, 128, 0);
        case Color::DARK_CYAN: return RGBColor(0, 128, 128);
        case Color::DARK_RED: return RGBColor(128, 0, 0);
        case Color::DARK_MAGENTA: return RGBColor(128, 0, 128);
        case Color::DARK_YELLOW: return RGBColor(128, 128, 0);
        case Color::GRAY: return RGBColor(192, 192, 192);
        case Color::DARK_GRAY: return RGBColor(128, 128, 128);
        case Color::BLUE: return RGBColor(0, 0, 255);
        case Color::GREEN: return RGBColor(0, 255, 0);
        case Color::CYAN: return RGBColor(0, 255, 255);
        case Color::RED: return RGBColor(255, 0, 0);
        case Color::MAGENTA: return RGBColor(255, 0, 255);
        case Color::YELLOW: return RGBColor(255, 255, 0);
        case Color::WHITE: return RGBColor(255, 255, 255);
        default: return RGBColor(255, 255, 255);
    }
}

// ANSI color code helper
const char* get_ansi_color(Color fg, Color bg) {
    static char buffer[32];
    int fg_code = 30;
    int bg_code = 40;
    
    // Foreground
    switch (fg) {
        case Color::BLACK: fg_code = 30; break;
        case Color::DARK_RED: fg_code = 31; break;
        case Color::DARK_GREEN: fg_code = 32; break;
        case Color::DARK_YELLOW: fg_code = 33; break;
        case Color::DARK_BLUE: fg_code = 34; break;
        case Color::DARK_MAGENTA: fg_code = 35; break;
        case Color::DARK_CYAN: fg_code = 36; break;
        case Color::GRAY: fg_code = 37; break;
        case Color::DARK_GRAY: fg_code = 90; break;
        case Color::RED: fg_code = 91; break;
        case Color::GREEN: fg_code = 92; break;
        case Color::YELLOW: fg_code = 93; break;
        case Color::BLUE: fg_code = 94; break;
        case Color::MAGENTA: fg_code = 95; break;
        case Color::CYAN: fg_code = 96; break;
        case Color::WHITE: fg_code = 97; break;
    }
    
    // Background
    switch (bg) {
        case Color::BLACK: bg_code = 40; break;
        case Color::DARK_RED: bg_code = 41; break;
        case Color::DARK_GREEN: bg_code = 42; break;
        case Color::DARK_YELLOW: bg_code = 43; break;
        case Color::DARK_BLUE: bg_code = 44; break;
        case Color::DARK_MAGENTA: bg_code = 45; break;
        case Color::DARK_CYAN: bg_code = 46; break;
        case Color::GRAY: bg_code = 47; break;
        case Color::DARK_GRAY: bg_code = 100; break;
        case Color::RED: bg_code = 101; break;
        case Color::GREEN: bg_code = 102; break;
        case Color::YELLOW: bg_code = 103; break;
        case Color::BLUE: bg_code = 104; break;
        case Color::MAGENTA: bg_code = 105; break;
        case Color::CYAN: bg_code = 106; break;
        case Color::WHITE: bg_code = 107; break;
    }
    
    snprintf(buffer, sizeof(buffer), "\033[%dm\033[%dm", fg_code, bg_code);
    return buffer;
}

// ANSI RGB color code helper
const char* get_ansi_rgb_color(const RGBColor& fg, const RGBColor& bg) {
    static char buffer[64];
    snprintf(buffer, sizeof(buffer), "\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm", 
             fg.r, fg.g, fg.b, bg.r, bg.g, bg.b);
    return buffer;
}

// Console constructor
Console::Console(int w, int h) : width(w), height(h) {
    buffer.resize(width * height);
    
    // Initialize with pitch black background
    for (auto& cell : buffer) {
        cell.character = " ";
        cell.rgb_fg = RGBColor(255, 255, 255);
        cell.rgb_bg = RGBColor(0, 0, 0);
        cell.use_rgb = true;
    }
    
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Enable UTF-8 support
    SetConsoleOutputCP(CP_UTF8);
    
    // Enable virtual terminal processing for ANSI escape codes
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);

    hBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );
    
    SetConsoleActiveScreenBuffer(hBuffer);
    
    // Enable UTF-8 and VT processing for the buffer too
    SetConsoleOutputCP(CP_UTF8);
    GetConsoleMode(hBuffer, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hBuffer, dwMode);
    
    // Set window size first (must be smaller than or equal to buffer)
    SMALL_RECT minWindow = {0, 0, 1, 1};
    SetConsoleWindowInfo(hBuffer, TRUE, &minWindow);
    
    // Set buffer size
    COORD size = {static_cast<SHORT>(width), static_cast<SHORT>(height)};
    SetConsoleScreenBufferSize(hBuffer, size);
    
    // Source - https://stackoverflow.com/a
    // Posted by 許恩嘉
    // Retrieved 2026-01-20, License - CC BY-SA 4.0
    HWND hwnd = GetConsoleWindow();
    Sleep(10);//If you execute these code immediately after the program starts, you must wait here for a short period of time, otherwise GetWindow will fail. I speculate that it may be because the console has not been fully initialized.
    HWND owner = GetWindow(hwnd, GW_OWNER);
    if (owner == NULL) {
        // Windows 10
        SetWindowPos(hwnd, nullptr, 0, 0, 1920, 1080, SWP_NOZORDER|SWP_NOMOVE);
    }
    else {
        // Windows 11
        SetWindowPos(owner, nullptr, 0, 0, 1920, 1080, SWP_NOZORDER|SWP_NOMOVE);
        
    }

    // Set console font to a TrueType font that supports UTF-8
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    GetCurrentConsoleFontEx(hBuffer, FALSE, &cfi);
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas"); // Changed to "Consolas" for better UTF-8 support
    cfi.dwFontSize.X = 0; // Let system choose width
    cfi.dwFontSize.Y = 16; // Set height
    SetCurrentConsoleFontEx(hBuffer, FALSE, &cfi);

    

    // Hide cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hBuffer, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hBuffer, &cursorInfo);
    
    bufferSize = {static_cast<SHORT>(width), static_cast<SHORT>(height)};
    bufferCoord = {0, 0};
    writeRegion = {0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1)};
#else
    // Linux: Hide cursor and clear screen
    printf("\033[?25l"); // Hide cursor
    printf("\033[2J");   // Clear screen
    fflush(stdout);
#endif
}

Console::~Console() {
#ifdef _WIN32
    SetConsoleActiveScreenBuffer(hConsole);
    CloseHandle(hBuffer);
#else
    printf("\033[?25h"); // Show cursor
    printf("\033[0m");   // Reset colors
    printf("\033[2J");   // Clear screen
    printf("\033[H");    // Move cursor to home
    fflush(stdout);
#endif
}

void Console::clear(Color bg) {
    (void)bg;  // Unused parameter
    for (auto& cell : buffer) {
        cell.character = " ";
        cell.rgb_fg = RGBColor(255, 255, 255);
        cell.rgb_bg = RGBColor(0, 0, 0);  // Pitch black
        cell.use_rgb = true;
    }
}

void Console::set_cell(int x, int y, char ch, Color fg, Color bg) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    
    int index = y * width + x;
    buffer[index].character = std::string(1, ch);
    buffer[index].foreground = fg;
    buffer[index].background = bg;
    buffer[index].use_rgb = false;
}

void Console::set_cell_rgb(int x, int y, char ch, RGBColor fg, RGBColor bg) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    
    int index = y * width + x;
    buffer[index].character = std::string(1, ch);
    buffer[index].rgb_fg = fg;
    buffer[index].rgb_bg = bg;
    buffer[index].use_rgb = true;
}

void Console::set_cell(int x, int y, const Cell& cell) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    buffer[y * width + x] = cell;
}

Cell Console::get_cell(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return Cell();
    }
    return buffer[y * width + x];
}

void Console::draw_string(int x, int y, const std::string& str, Color fg, Color bg) {
    int current_x = x;
    size_t i = 0;
    
    while (i < str.length() && current_x < width) {
        // Check if this is a UTF-8 multi-byte character
        unsigned char c = str[i];
        int char_len = 1;
        
        if ((c & 0x80) == 0) {
            // Single-byte ASCII
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }
        
        // Extract the full character
        std::string character = str.substr(i, char_len);
        
        // Set the cell with the complete UTF-8 character
        if (current_x >= 0 && current_x < width && y >= 0 && y < height) {
            int index = y * width + current_x;
            buffer[index].character = character;
            buffer[index].foreground = fg;
            buffer[index].background = bg;
            buffer[index].use_rgb = false;
        }
        
        i += char_len;
        current_x++;
    }
}

void Console::draw_string_rgb(int x, int y, const std::string& str, RGBColor fg, RGBColor bg) {
    int current_x = x;
    size_t i = 0;
    
    while (i < str.length() && current_x < width) {
        // Check if this is a UTF-8 multi-byte character
        unsigned char c = str[i];
        int char_len = 1;
        
        if ((c & 0x80) == 0) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }
        
        std::string character = str.substr(i, char_len);
        
        if (current_x >= 0 && current_x < width && y >= 0 && y < height) {
            int index = y * width + current_x;
            buffer[index].character = character;
            buffer[index].rgb_fg = fg;
            buffer[index].rgb_bg = bg;
            buffer[index].use_rgb = true;
        }
        
        i += char_len;
        current_x++;
    }
}

void Console::draw_box(int x, int y, int w, int h, Color fg, Color bg,
                       char horizontal, char vertical,
                       char corner_tl, char corner_tr,
                       char corner_bl, char corner_br) {
    // Draw corners
    set_cell(x, y, corner_tl, fg, bg);
    set_cell(x + w - 1, y, corner_tr, fg, bg);
    set_cell(x, y + h - 1, corner_bl, fg, bg);
    set_cell(x + w - 1, y + h - 1, corner_br, fg, bg);
    
    // Draw horizontal borders
    for (int i = 1; i < w - 1; ++i) {
        set_cell(x + i, y, horizontal, fg, bg);
        set_cell(x + i, y + h - 1, horizontal, fg, bg);
    }
    
    // Draw vertical borders
    for (int i = 1; i < h - 1; ++i) {
        set_cell(x, y + i, vertical, fg, bg);
        set_cell(x + w - 1, y + i, vertical, fg, bg);
    }
}

void Console::draw_box_rgb(int x, int y, int w, int h, RGBColor fg, RGBColor bg,
                           char horizontal, char vertical,
                           char corner_tl, char corner_tr,
                           char corner_bl, char corner_br) {
    // Draw corners
    set_cell_rgb(x, y, corner_tl, fg, bg);
    set_cell_rgb(x + w - 1, y, corner_tr, fg, bg);
    set_cell_rgb(x, y + h - 1, corner_bl, fg, bg);
    set_cell_rgb(x + w - 1, y + h - 1, corner_br, fg, bg);
    
    // Draw horizontal borders
    for (int i = 1; i < w - 1; ++i) {
        set_cell_rgb(x + i, y, horizontal, fg, bg);
        set_cell_rgb(x + i, y + h - 1, horizontal, fg, bg);
    }
    
    // Draw vertical borders
    for (int i = 1; i < h - 1; ++i) {
        set_cell_rgb(x, y + i, vertical, fg, bg);
        set_cell_rgb(x + w - 1, y + i, vertical, fg, bg);
    }
}

void Console::fill_rect(int x, int y, int w, int h, char ch, Color fg, Color bg) {
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            set_cell(x + dx, y + dy, ch, fg, bg);
        }
    }
}

void Console::present() {
#ifdef _WIN32
    // Ensure buffer size is correct (in case window was resized)
    COORD size = {static_cast<SHORT>(width), static_cast<SHORT>(height)};
    SetConsoleScreenBufferSize(hBuffer, size);
    
    // Use ANSI escape codes on Windows too (requires VT processing enabled)
    // Move cursor to home position
    std::string output = "\033[H";
    
    Color last_fg = Color::WHITE;
    Color last_bg = Color::BLACK;
    RGBColor last_rgb_fg(255, 255, 255);
    RGBColor last_rgb_bg(0, 0, 0);
    bool last_use_rgb = false;
    bool first = true;
    
    for (int y = 0; y < height; ++y) {
        // Move cursor to start of this line
        char line_pos[32];
        snprintf(line_pos, sizeof(line_pos), "\033[%d;1H", y + 1);
        output += line_pos;
        
        for (int x = 0; x < width; ++x) {
            const Cell& cell = buffer[y * width + x];
            
            if (cell.use_rgb) {
                if (first || !last_use_rgb || 
                    cell.rgb_fg.r != last_rgb_fg.r || cell.rgb_fg.g != last_rgb_fg.g || cell.rgb_fg.b != last_rgb_fg.b ||
                    cell.rgb_bg.r != last_rgb_bg.r || cell.rgb_bg.g != last_rgb_bg.g || cell.rgb_bg.b != last_rgb_bg.b) {
                    output += get_ansi_rgb_color(cell.rgb_fg, cell.rgb_bg);
                    last_rgb_fg = cell.rgb_fg;
                    last_rgb_bg = cell.rgb_bg;
                    last_use_rgb = true;
                }
            } else {
                if (first || last_use_rgb || cell.foreground != last_fg || cell.background != last_bg) {
                    output += get_ansi_color(cell.foreground, cell.background);
                    last_fg = cell.foreground;
                    last_bg = cell.background;
                    last_use_rgb = false;
                }
            }
            
            output += cell.character;
            first = false;
        }
    }
    
    output += "\033[0m"; // Reset colors
    
    DWORD written;
    WriteConsoleA(hBuffer, output.c_str(), static_cast<DWORD>(output.length()), &written, NULL);
#else
    // Linux: Use ANSI escape codes
    printf("\033[H"); // Move cursor to home
    
    Color last_fg = Color::WHITE;
    Color last_bg = Color::BLACK;
    RGBColor last_rgb_fg(255, 255, 255);
    RGBColor last_rgb_bg(0, 0, 0);
    bool last_use_rgb = false;
    bool first = true;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Cell& cell = buffer[y * width + x];
            
            if (cell.use_rgb) {
                if (first || !last_use_rgb || 
                    cell.rgb_fg.r != last_rgb_fg.r || cell.rgb_fg.g != last_rgb_fg.g || cell.rgb_fg.b != last_rgb_fg.b ||
                    cell.rgb_bg.r != last_rgb_bg.r || cell.rgb_bg.g != last_rgb_bg.g || cell.rgb_bg.b != last_rgb_bg.b) {
                    printf("%s", get_ansi_rgb_color(cell.rgb_fg, cell.rgb_bg));
                    last_rgb_fg = cell.rgb_fg;
                    last_rgb_bg = cell.rgb_bg;
                    last_use_rgb = true;
                }
            } else {
                if (first || last_use_rgb || cell.foreground != last_fg || cell.background != last_bg) {
                    printf("%s", get_ansi_color(cell.foreground, cell.background));
                    last_fg = cell.foreground;
                    last_bg = cell.background;
                    last_use_rgb = false;
                }
            }
            
            first = false;
            printf("%s", cell.character.c_str());
        }
        if (y < height - 1) putchar('\n');
    }
    
    printf("\033[0m"); // Reset color
    fflush(stdout);
#endif
}
