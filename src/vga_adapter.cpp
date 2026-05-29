// vga_adapter.cpp
#include "vga_adapter.h"
#include "gb_sdl_font8x8.h"

// Static variable definitions
uint8_t** VGADisplay::framebuffer_ptr = nullptr;
uint8_t* VGADisplay::framebuffer_linear = nullptr;
uint8_t VGADisplay::sync_bits_val = 0;
int VGADisplay::cursorX = 0;
int VGADisplay::cursorY = 0;
uint8_t VGADisplay::textColor = VGA_COLOR_WHITE;
uint8_t VGADisplay::bgColor = VGA_COLOR_BLACK;
bool VGADisplay::initialized = false;
bool VGADisplay::emulationMode = false;
bool VGADisplay::bufferNeedsUpdate = true;
int VGADisplay::screen_x_offset = 0;
uint8_t VGADisplay::color_table[128];

// Frameskip variables
uint8_t VGADisplay::frameskip = 0;
uint8_t VGADisplay::frame_counter = 0;

// Convert RGB565 color to 6-bit VGA format (RRGGBB)
uint8_t VGADisplay::rgb565_to_vga6(uint16_t rgb565) {
    uint8_t r = (rgb565 >> 8) & 0xF8;
    uint8_t g = (rgb565 >> 3) & 0xFC;
    uint8_t b = (rgb565 << 3) & 0xF8;
    
    uint8_t r2 = r >> 6;
    uint8_t g2 = g >> 6;
    uint8_t b2 = b >> 6;
    
    return (b2 << 4) | (g2 << 2) | r2;
}

// Initialize color lookup table (similar to TinyNES PreparaColorVGA)
void VGADisplay::initColorTable() {
    for (int i = 0; i < 128; i++) {
        color_table[i] = sync_bits_val | (i & 0x3F);
    }
}

// Fast pixel drawing function (like jj_fast_drawpixel from TinyNES)
void VGADisplay::fastDrawPixel(int x, int y, uint8_t nescolor) {
    if (!framebuffer_ptr) return;
    // Boundary check
    if (x < 0 || x >= getWidth() || y < 0 || y >= getHeight()) {
        #ifdef DEBUG
            Serial.printf("fastDrawPixel out of bounds: x=%d, y=%d\n", x, y);
        #endif
        return;
    }
    int final_x = (x + screen_x_offset) ^ 2;
    if (final_x < 0 || final_x >= getWidth()) {
        #ifdef DEBUG
            Serial.printf("final_x out of bounds: %d\n", final_x);
        #endif
        return;
    }
    framebuffer_ptr[y][final_x] = color_table[nescolor & 0x7F];
}

// Set horizontal screen offset for scrolling effects
void VGADisplay::setScreenOffset(int offset) {
    screen_x_offset = offset;
}

// Get current screen offset
int VGADisplay::getScreenOffset() {
    return screen_x_offset;
}

// Get color lookup table pointer
uint8_t* VGADisplay::getColorTable() {
    return color_table;
}

// Initialize VGA display hardware
void VGADisplay::init() {
    if (initialized) return;
    
    #ifdef DEBUG
        Serial.println("VGADisplay: Initializing VGA (TinyNES mode)...");
    #endif
    
    static const unsigned char vga_pins[] = VGA_PINS;
    
    vga_init(vga_pins, VgaMode_vga_mode_320x240, false);
    
    framebuffer_ptr = vga_get_framebuffer();
    framebuffer_linear = vga_get_raw_framebuffer();
    sync_bits_val = vga_get_sync_bits();
    
    // Initialize color lookup table
    initColorTable();
    
    initialized = true;
    
    #ifdef DEBUG
        Serial.printf("VGADisplay: %dx%d initialized\n", getWidth(), getHeight());
        Serial.printf("Sync bits: 0x%02X\n", sync_bits_val);
    #endif
}

// Fill entire screen with a single color
void VGADisplay::fillScreen(uint16_t color) {
    uint8_t vga_color = rgb565_to_vga6(color);
    if (framebuffer_linear) {
        memset(framebuffer_linear, sync_bits_val | vga_color, getWidth() * getHeight());
    }
}

// Draw a filled rectangle
void VGADisplay::fillRect(int x, int y, int w, int h, uint16_t color) {
    if (!framebuffer_ptr) return;
    
    uint8_t vga_color = rgb565_to_vga6(color);
    uint8_t pixel = sync_bits_val | vga_color;
    int x1 = max(x, 0);
    int y1 = max(y, 0);
    int x2 = min(x + w, getWidth());
    int y2 = min(y + h, getHeight());
    
    for (int row = y1; row < y2; row++) {
        uint8_t* line = framebuffer_ptr[row];
        for (int col = x1; col < x2; col++) {
            int final_col = (col + screen_x_offset) ^ 2;
            line[final_col] = pixel;
        }
    }
}

// Draw hollow rectangle outline
void VGADisplay::drawRect(int x, int y, int w, int h, uint16_t color) {
    fillRect(x, y, w, 1, color);
    fillRect(x, y + h - 1, w, 1, color);
    fillRect(x, y + 1, 1, h - 2, color);
    fillRect(x + w - 1, y + 1, 1, h - 2, color);
}

// Draw a single character using 8x8 font
void VGADisplay::drawChar(char c, int x, int y, uint8_t color, uint8_t backcolor) {
    int auxId = c << 3;
    
    for (int row = 0; row < 8; row++) {
        int y_pos = y + row;
        if (y_pos >= getHeight()) break;
        
        uint8_t bits = gb_sdl_font_8x8[auxId + row];
        
        for (int col = 0; col < 8; col++) {
            int x_pos = x + (6 - col);
            if (x_pos >= getWidth()) continue;
            
            uint8_t auxColor = (bits >> col) & 0x01;
            fastDrawPixel(x_pos, y_pos, auxColor ? color : backcolor);
        }
    }
}

// Draw a string of text
void VGADisplay::drawString(const char* text, int x, int y, int font) {
    (void)font;
    int px = x;
    
    while (*text) {
        drawChar(*text++, px, y, textColor, bgColor);
        px += 7;
    }
}

// Set text foreground and background colors
void VGADisplay::setTextColor(uint16_t fg, uint16_t bg) {
    textColor = rgb565_to_vga6(fg);
    bgColor = rgb565_to_vga6(bg);
}

// Set cursor position for print operations
void VGADisplay::setCursor(int x, int y) {
    cursorX = x;
    cursorY = y;
}

// Print string at current cursor position
void VGADisplay::print(const char* text) {
    drawString(text, cursorX, cursorY);
    cursorX += strlen(text) * 7;
}

// Print single character
void VGADisplay::print(char c) {
    char str[2] = {c, 0};
    print(str);
}

// Calculate width of text string in pixels
int VGADisplay::textWidth(const char* text) {
    return strlen(text) * 7;
}

// Get screen width in pixels
int VGADisplay::getWidth() {
    return vga_get_xres();
}

// Get screen height in pixels
int VGADisplay::getHeight() {
    return vga_get_yres();
}

// Enter emulation mode (fullscreen game rendering)
void VGADisplay::beginEmulation() {
    emulationMode = true;
    fillScreen(VGA_COLOR_BLACK_RGB565);
}

// Exit emulation mode
void VGADisplay::endEmulation() {
    emulationMode = false;
}

// Draw a block of pixels from frame buffer
void VGADisplay::drawBlock(int startY, uint16_t* data, int width, int height) {
    if (!emulationMode || !data || !framebuffer_ptr) return;
    
    int xres = getWidth();
    int yres = getHeight();
    int xoffset = (xres - width) / 2;
    if (xoffset < 0) xoffset = 0;
    
    for (int y = 0; y < height && startY + y < yres; y++) {
        uint16_t* src = data + (y * width);
        uint8_t* dst = framebuffer_ptr[startY + y] + xoffset;
        
        for (int x = 0; x < width && x + xoffset < xres; x++) {
            dst[x] = sync_bits_val | rgb565_to_vga6(src[x]);
        }
    }
    
    bufferNeedsUpdate = true;
}

// Update display (sync with single buffer)
void VGADisplay::updateDisplay() {
    if (emulationMode && bufferNeedsUpdate) {
        // Single buffer configuration - no actual swap, just sync
        bufferNeedsUpdate = false;
    }
}

// Get frame buffer (placeholder)
uint16_t* VGADisplay::getFrameBuffer() {
    return nullptr;
}

// Get raw framebuffer pointer
uint8_t* VGADisplay::getRawFramebuffer() {
    return framebuffer_linear;
}

// Get sync bits value
uint8_t VGADisplay::getSyncBits() {
    return sync_bits_val;
}

// Set frameskip value (0 = no skip, >0 = skip frames)
void VGADisplay::setFrameskip(uint8_t skip) {
    frameskip = skip;
    frame_counter = 0;
}

// Get current frameskip value
uint8_t VGADisplay::getFrameskip() {
    return frameskip;
}