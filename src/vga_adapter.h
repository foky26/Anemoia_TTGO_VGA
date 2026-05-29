// vga_adapter.h
#ifndef VGA_ADAPTER_H
#define VGA_ADAPTER_H

#include <Arduino.h>
#include <stdint.h>
#include "vga_6bit.h"
#include "../config.h"

// RGB565 color definitions
#define VGA_COLOR_BLACK_RGB565   0x0000
#define VGA_COLOR_WHITE_RGB565   0xFFFF
#define VGA_COLOR_RED_RGB565     0xF800
#define VGA_COLOR_GREEN_RGB565   0x07E0
#define VGA_COLOR_BLUE_RGB565    0x001F
#define VGA_COLOR_YELLOW_RGB565  0xFFE0

// UI Colors in RGB565
#define UI_COLOR_BLACK      0x0000
#define UI_COLOR_WHITE      0xFFFF
#define UI_COLOR_RED        0xF800
#define UI_COLOR_GREEN      0x07E0
#define UI_COLOR_BLUE       0x001F
#define UI_COLOR_YELLOW     0xFFE0
#define UI_COLOR_CYAN       0x07FF
#define UI_COLOR_MAGENTA    0xF81F
#define UI_COLOR_ORANGE     0xFC00
#define UI_COLOR_PURPLE     0x801F
#define UI_COLOR_GRAY       0x8410

// UI color scheme
#define BG_COLOR           UI_COLOR_BLACK
#define BAR_COLOR          UI_COLOR_BLUE
#define BAR_TEXT_COLOR     UI_COLOR_WHITE
#define TEXT_COLOR         UI_COLOR_WHITE
#define TEXT2_COLOR        UI_COLOR_CYAN
#define SELECTED_TEXT_COLOR UI_COLOR_BLACK
#define SELECTED_BG_COLOR  UI_COLOR_YELLOW
#define BOX_BORDER_COLOR   UI_COLOR_CYAN
#define TITLE_BG_COLOR     UI_COLOR_PURPLE
#define TITLE_TEXT_COLOR   UI_COLOR_WHITE

class VGADisplay {
public:
    // Initialization and basic operations
    static void init();
    static void fillScreen(uint16_t color);
    static void fillRect(int x, int y, int w, int h, uint16_t color);
    static void drawRect(int x, int y, int w, int h, uint16_t color);
    
    // Text rendering
    static void drawString(const char* text, int x, int y, int font = 0);
    static void drawChar(char c, int x, int y, uint8_t color, uint8_t backcolor);
    static void setTextColor(uint16_t fg, uint16_t bg = 0);
    static void setCursor(int x, int y);
    static void print(const char* text);
    static void print(char c);
    static int textWidth(const char* text);
    
    // Display properties
    static int getWidth();
    static int getHeight();
    
    // Emulation mode control
    static void beginEmulation();
    static void endEmulation();
    static void updateDisplay();
    static uint16_t* getFrameBuffer();
    
    // Frameskip control
    static void setFrameskip(uint8_t skip);
    static uint8_t getFrameskip();
    
    // Low-level drawing
    static void fastDrawPixel(int x, int y, uint8_t nescolor);
    static void setScreenOffset(int offset);
    static int getScreenOffset();
    static uint8_t* getRawFramebuffer();
    static uint8_t getSyncBits();
    static uint8_t* getColorTable();
    static void drawBlock(int startY, uint16_t* data, int width, int height);
    
    // State flags
    static bool bufferNeedsUpdate;
    static bool emulationMode;
    
private:
    static uint8_t rgb565_to_vga6(uint16_t rgb565);
    static void initColorTable();
    
    static uint8_t** framebuffer_ptr;
    static uint8_t* framebuffer_linear;
    static uint8_t sync_bits_val;
    static int cursorX, cursorY;
    static uint8_t textColor, bgColor;
    static bool initialized;
    static int screen_x_offset;
    static uint8_t color_table[128];
    static uint8_t frameskip;
    static uint8_t frame_counter;
};

#endif