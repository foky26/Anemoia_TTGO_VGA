// ps2_keyboard.cpp
#include "ps2_keyboard.h"
#include "../config.h"

fabgl::PS2Controller PS2Keyboard::ps2;
fabgl::Keyboard PS2Keyboard::keyboard;
uint8_t PS2Keyboard::currentState = 0;

// Initialize PS/2 keyboard interface
void PS2Keyboard::init() {
    #ifdef DEBUG
        Serial.println("PS2: Initializing...");
    #endif
    
    ps2.begin((gpio_num_t)PS2_CLK_PIN, (gpio_num_t)PS2_DAT_PIN);
    
    // generateVirtualKeys = true, createVKQueue = false (no buffer)
    keyboard.begin(true, false, 0);
    
    #ifdef DEBUG
        Serial.println("PS2: Initialized (no event queue)");
        Serial.println("Key mapping:");
        Serial.println("  NES A    -> Z / z (uppercase or lowercase)");
        Serial.println("  NES B    -> X / x (uppercase or lowercase)");
        Serial.println("  NES START-> ENTER");
        Serial.println("  NES SELECT-> SPACE");
        Serial.println("  ESC      -> Reset system");
    #endif
}

// Update controller state by reading current keyboard input
void PS2Keyboard::updateState() {
    currentState = 0;
    
    // NES A button
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_Z) || 
        keyboard.isVKDown(fabgl::VirtualKey::VK_z)) currentState |= NES_A;

    // NES B button
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_X) || 
        keyboard.isVKDown(fabgl::VirtualKey::VK_x)) currentState |= NES_B;
    
    // System buttons
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_RETURN)) currentState |= NES_START;
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_SPACE)) currentState |= NES_SELECT;
    
    // Directional pad
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_UP)) currentState |= NES_UP;
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_DOWN)) currentState |= NES_DOWN;
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_LEFT)) currentState |= NES_LEFT;
    if (keyboard.isVKDown(fabgl::VirtualKey::VK_RIGHT)) currentState |= NES_RIGHT;
    
    // ESC key triggers system reset
    static bool lastEsc = false;
    bool currentEsc = keyboard.isVKDown(fabgl::VirtualKey::VK_ESCAPE);
    if (currentEsc && !lastEsc) {
        #ifdef DEBUG
            Serial.println("ESC pressed - Resetting system...");
        #endif
        ESP.restart();
    }
    lastEsc = currentEsc;
}

// Return current controller state as bitmask
uint8_t PS2Keyboard::read() {
    updateState();
    return currentState;
}