// ui.cpp
#include "ui.h"

// Constructor - store screen reference
UI::UI(VGADisplay* screen)
{    
    this->screen = screen;
}

UI::~UI()
{
}

// Draw top and bottom status bars
void UI::drawBars() 
{
    int screenWidth = screen->getWidth();
    int screenHeight = screen->getHeight();
    
    // Top bar with title
    screen->fillRect(0, 0, screenWidth, 24, BAR_COLOR);
    screen->fillRect(0, 22, screenWidth, 2, UI_COLOR_CYAN);
    
    // Emulator title
    screen->setTextColor(BAR_TEXT_COLOR, BAR_COLOR);
    screen->setCursor(8, 8);
    screen->print("ANEMOIA NES EMULATOR FOR TTGO VGA");
    
    // Bottom bar with control information
    screen->fillRect(0, screenHeight - 28, screenWidth, 28, BAR_COLOR);
    screen->fillRect(0, screenHeight - 30, screenWidth, 2, UI_COLOR_CYAN);
    
    int y = screenHeight - 20;
    screen->setTextColor(UI_COLOR_YELLOW, BAR_COLOR);
    screen->setCursor(20, y);
    screen->print("A=Z B=X SELECT=SPACE START=ENTER");
    
    // Directional arrows
    screen->setTextColor(UI_COLOR_CYAN, BAR_COLOR);
    screen->setCursor(260, y);
    screen->print("\x18 \x19 \x1A \x1B");
}

void UI::drawControlsInfo()
{
    // Already integrated into drawBars()
}

// Game selection menu - returns selected cartridge
Cartridge* UI::selectGame()
{
    unsigned int last_input_time = 0;
    constexpr unsigned int delay = 120;
    
    int screenWidth = screen->getWidth();
    int screenHeight = screen->getHeight();
    
    max_items = (screenHeight - 80) / ITEM_HEIGHT;

    // Draw simplified interface
    drawBars();
    
    // Single box for game list
    int list_x = 8;
    int list_y = 32;
    int list_w = screenWidth - 16;
    int list_h = screenHeight - 68;
    
    screen->drawRect(list_x, list_y, list_w, list_h, BOX_BORDER_COLOR);
    
    // Load and display game list
    getNesFiles();
    drawFileList();

    const int size = files.size();
    while (true)
    {
        unsigned int now = millis();

        if (now - last_input_time > delay)
        {
            // Navigate up
            if (isDownPressed(CONTROLLER::Up)) 
            {
                selected--;
                if (selected < 0)
                {
                    selected = (size - 1);
                    scroll_offset = selected - max_items + 1;
                }
                else if (selected < scroll_offset) scroll_offset = selected; 
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                drawFileList();
                last_input_time = now;
            }

            // Navigate down
            if (isDownPressed(CONTROLLER::Down)) 
            {
                selected++; 
                if (selected > (size - 1))
                {
                    selected = 0;
                    scroll_offset = selected;
                }
                else if (selected >= scroll_offset + max_items) scroll_offset = selected - max_items + 1;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                drawFileList();
                last_input_time = now;
            }
            
            // Select game with Start button
            if (isDownPressed(CONTROLLER::Start) && (selected >= 0 && selected < size))
            {
                Cartridge* cart;
                std::string fullPath = "/" + files[selected];
                const char* game = fullPath.c_str();
                std::vector<std::string>().swap(files);
                
                // Simple loading message
                screen->fillRect(0, screenHeight/2 - 15, screenWidth, 30, UI_COLOR_BLACK);
                screen->drawRect(screenWidth/2 - 80, screenHeight/2 - 12, 160, 24, UI_COLOR_CYAN);
                screen->setTextColor(UI_COLOR_YELLOW, UI_COLOR_BLACK);
                screen->setCursor((screenWidth - screen->textWidth("Loading...")) / 2, screenHeight/2 - 4);
                screen->print("Loading...");
                screen->updateDisplay();
                
                cart = new Cartridge(game);
                return cart;
            }
        }
    }
}

// Scan SD card for NES ROM files
void UI::getNesFiles()
{
    files.clear();
    File root = SD.open("/");
    while (true)
    {
        File file = root.openNextFile();
        if (!file) break;
        if (!file.isDirectory())
        {
            std::string filename = file.name();
            if (filename.rfind(".nes") == filename.size() - 4 ||
                filename.rfind(".NES") == filename.size() - 4)
                files.push_back(filename);
        }
        file.close();
    }
    root.close();
    
    // Sort files alphabetically
    std::sort(files.begin(), files.end());
}

// Draw the list of available games
void UI::drawFileList()
{
    int screenWidth = screen->getWidth();
    int screenHeight = screen->getHeight();
    
    // Clear list area
    screen->fillRect(10, 34, screenWidth - 20, screenHeight - 72, BG_COLOR);

    const int size = files.size();
    for (int i = 0; i < max_items; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;

        std::string file = files[item];
        int maxWidth = screenWidth - 48;
        
        // Truncate long filenames
        while (screen->textWidth(file.c_str()) > maxWidth)
        {
            file.pop_back();
        }
        if (file.size() < files[item].size())
        {
            file.replace(file.size()-3, 3, "...");
        }

        const char* filename = file.c_str();
        int y = i * ITEM_HEIGHT + 44;
        
        // Highlight selected item
        if (item == selected)
        {
            screen->fillRect(12, y - 2, screenWidth - 24, ITEM_HEIGHT, SELECTED_BG_COLOR);
            screen->setTextColor(SELECTED_TEXT_COLOR, SELECTED_BG_COLOR);
            screen->drawString(filename, 20, y, 1);
        }
        else
        {
            screen->setTextColor(TEXT_COLOR, BG_COLOR);
            screen->drawString(filename, 20, y, 1);
        }
    }
}

// Pause menu with game options (Resume, Reset, Save, Load, Exit)
void UI::pauseMenu(Bus* nes)
{
    int screenWidth = screen->getWidth();
    int screenHeight = screen->getHeight();
    
    paused = true;
    int select = 0;

    screen->fillRect(0, 0, screenWidth, screenHeight, BG_COLOR);
    
    int panel_w = 200;
    int panel_h = 180;
    int panel_x = (screenWidth - panel_w) / 2;
    int panel_y = (screenHeight - panel_h) / 2;
    
    screen->fillRect(panel_x, panel_y, panel_w, panel_h, UI_COLOR_GRAY);
    screen->drawRect(panel_x, panel_y, panel_w, panel_h, UI_COLOR_CYAN);
    
    // Title
    screen->fillRect(panel_x, panel_y, panel_w, 24, UI_COLOR_PURPLE);
    screen->setTextColor(UI_COLOR_WHITE, UI_COLOR_PURPLE);
    screen->setCursor(panel_x + (panel_w - screen->textWidth("PAUSE")) / 2, panel_y + 7);
    screen->print("PAUSE");
    
    const char* items[] = {"Resume", "Reset", "Quick Save", "Quick Load", "Exit"};
    const int num_items = 5;
    
    for (int i = 0; i < num_items; i++) {
        int y = panel_y + 40 + (i * 25);
        if (i == select) {
            screen->fillRect(panel_x + 10, y - 3, panel_w - 20, 20, SELECTED_BG_COLOR);
            screen->setTextColor(SELECTED_TEXT_COLOR, SELECTED_BG_COLOR);
        } else {
            screen->setTextColor(TEXT_COLOR, UI_COLOR_GRAY);
        }
        screen->setCursor(panel_x + (panel_w - screen->textWidth(items[i])) / 2, y);
        screen->print(items[i]);
    }
    
    screen->updateDisplay();

    constexpr int delay = 200;
    int last_input_time = millis();
    
    while (paused)
    {
        int now = millis();
        if (now - last_input_time > delay)
        {
            // Navigate up
            if (isDownPressed(CONTROLLER::Up)) 
            {
                select--;
                if (select < 0) select = num_items - 1;
                for (int i = 0; i < num_items; i++) {
                    int y = panel_y + 40 + (i * 25);
                    if (i == select) {
                        screen->fillRect(panel_x + 10, y - 3, panel_w - 20, 20, SELECTED_BG_COLOR);
                        screen->setTextColor(SELECTED_TEXT_COLOR, SELECTED_BG_COLOR);
                    } else {
                        screen->fillRect(panel_x + 10, y - 3, panel_w - 20, 20, UI_COLOR_GRAY);
                        screen->setTextColor(TEXT_COLOR, UI_COLOR_GRAY);
                    }
                    screen->setCursor(panel_x + (panel_w - screen->textWidth(items[i])) / 2, y);
                    screen->print(items[i]);
                }
                screen->updateDisplay();
                last_input_time = now;
            }

            // Navigate down
            if (isDownPressed(CONTROLLER::Down)) 
            {
                select++;
                if (select >= num_items) select = 0;
                for (int i = 0; i < num_items; i++) {
                    int y = panel_y + 40 + (i * 25);
                    if (i == select) {
                        screen->fillRect(panel_x + 10, y - 3, panel_w - 20, 20, SELECTED_BG_COLOR);
                        screen->setTextColor(SELECTED_TEXT_COLOR, SELECTED_BG_COLOR);
                    } else {
                        screen->fillRect(panel_x + 10, y - 3, panel_w - 20, 20, UI_COLOR_GRAY);
                        screen->setTextColor(TEXT_COLOR, UI_COLOR_GRAY);
                    }
                    screen->setCursor(panel_x + (panel_w - screen->textWidth(items[i])) / 2, y);
                    screen->print(items[i]);
                }
                screen->updateDisplay();
                last_input_time = now;
            }
            
            // Select option with A or Start button
            if (isDownPressed(CONTROLLER::A) || isDownPressed(CONTROLLER::Start)) 
            {
                switch (select)
                {
                case 0:
                    paused = false;
                    screen->fillScreen(BG_COLOR);
                    return;
                case 1:
                    nes->reset();
                    paused = false;
                    screen->fillScreen(BG_COLOR);
                    return;
                case 2:
                    nes->saveState();
                    screen->fillScreen(BG_COLOR);
                    paused = false;
                    return;
                case 3:
                    nes->loadState();
                    screen->fillScreen(BG_COLOR);
                    paused = false;
                    return;
                case 4:
                    ESP.restart();
                    return;
                }
            }
        }
    }
}

// Settings menu (placeholder for future features)
void UI::settingsMenu(Bus* nes)
{
    (void)nes;
    int screenWidth = screen->getWidth();
    int screenHeight = screen->getHeight();
    
    screen->fillScreen(BG_COLOR);
    
    int panel_w = 280;
    int panel_h = 100;
    int panel_x = (screenWidth - panel_w) / 2;
    int panel_y = (screenHeight - panel_h) / 2;
    
    screen->fillRect(panel_x, panel_y, panel_w, panel_h, UI_COLOR_GRAY);
    screen->drawRect(panel_x, panel_y, panel_w, panel_h, UI_COLOR_CYAN);
    
    screen->setTextColor(UI_COLOR_YELLOW, UI_COLOR_GRAY);
    screen->setCursor(panel_x + 10, panel_y + 10);
    screen->print("Settings Menu");
    
    screen->setTextColor(UI_COLOR_WHITE, UI_COLOR_GRAY);
    screen->setCursor(panel_x + 10, panel_y + 35);
    screen->print("Coming Soon...");
    
    screen->updateDisplay();
    delay(2000);
    screen->fillScreen(BG_COLOR);
}

// Initialize settings from SD card or create default
void UI::initializeSettings(Bus* nes)
{
    if (!SD.exists("/settings.bin"))
    {
        Settings temp;
        saveSettings(&temp);
    }
    loadSettings(&settings);
    
    nes->ppu.setPalette(settings.palette);
    nes->cpu.apu.setVolume(settings.volume);
}

// Save settings to SD card
void UI::saveSettings(const Settings* s)
{
    File f = SD.open("/settings.bin", FILE_WRITE);
    if (!f) return;
    f.seek(0);
    f.write((uint8_t*)s, sizeof(*s));
    f.close();
}

// Load settings from SD card
void UI::loadSettings(Settings* s)
{
    File f = SD.open("/settings.bin", FILE_READ);
    if (!f) return;
    if (f.size() != sizeof(Settings)) 
    {
        f.close();
        Settings temp;
        saveSettings(&temp);
        *s = temp;
        return;
    }
    f.read((uint8_t*)s, sizeof(*s));
    f.close();
}

// Helper to draw text at specified coordinates
void UI::drawText(const char* text, const int x, const int y)
{
    screen->setTextColor(UI_COLOR_WHITE, BG_COLOR);
    screen->setCursor(x, y);
    screen->print(text);
}
