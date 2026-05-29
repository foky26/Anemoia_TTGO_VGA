// ui.h
#ifndef UI_H
#define UI_H

#include <SD.h>
#include <vector>
#include <string>

#include "controller.h"
#include "core/bus.h"
#include "vga_adapter.h"

// User settings structure
typedef struct Settings
{
    uint8_t volume = 100;   // Audio volume (0-100)
    uint8_t palette = 0;    // Color palette selection
} Settings;

// Basic UI colors (RGB565 format)
#define UI_COLOR_BLACK      0x0000
#define UI_COLOR_WHITE      0xFFFF
#define UI_COLOR_RED        0xF800
#define UI_COLOR_GREEN      0x07E0
#define UI_COLOR_BLUE       0x001F
#define UI_COLOR_YELLOW     0xFFE0
#define UI_COLOR_CYAN       0x07FF
#define UI_COLOR_PURPLE     0x801F
#define UI_COLOR_GRAY       0x8410

// Simplified UI color scheme
#define BG_COLOR           UI_COLOR_BLACK
#define BAR_COLOR          UI_COLOR_BLUE
#define BAR_TEXT_COLOR     UI_COLOR_WHITE
#define TEXT_COLOR         UI_COLOR_WHITE
#define SELECTED_TEXT_COLOR UI_COLOR_BLACK
#define SELECTED_BG_COLOR  UI_COLOR_YELLOW
#define BOX_BORDER_COLOR   UI_COLOR_CYAN

class UI
{
public:
    UI(VGADisplay* screen);
    ~UI();
    
    Cartridge* selectGame();                    // Game selection menu
    void getNesFiles();                         // Scan for .nes files
    void drawFileList();                        // Render game list
    void drawBars();                            // Draw top/bottom bars
    void drawControlsInfo();                    // Display control info
    void pauseMenu(Bus* nes);                   // In-game pause menu
    void settingsMenu(Bus* nes);                // Settings menu (placeholder)
    void initializeSettings(Bus* nes);          // Load/setup settings
    void saveSettings(const Settings* s);       // Save to SD card
    void loadSettings(Settings* s);             // Load from SD card

    bool paused = false;                        // Pause state flag

private:
    void drawText(const char* text, const int x, const int y);  // Helper
    
    VGADisplay* screen = nullptr;               // Display reference
    int selected = 0;                           // Currently selected item
    int scroll_offset = 0;                      // Scroll position
    int max_items = 0;                          // Max visible items
    static constexpr int ITEM_HEIGHT = 14;      // Height of each list item
    std::vector<std::string> files;             // List of ROM files

    Settings settings;                          // Current settings
};

#endif