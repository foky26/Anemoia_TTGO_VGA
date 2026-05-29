// ps2_keyboard.h
#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

#include <Arduino.h>
#include <fabgl.h>

class PS2Keyboard {
public:
    static void init();
    static uint8_t read();       // Single function for all controller input
    
    // NES button bitmask values
    enum NESButton : uint8_t {
        NES_A      = 0x01,
        NES_B      = 0x02,
        NES_SELECT = 0x04,
        NES_START  = 0x08,
        NES_UP     = 0x10,
        NES_DOWN   = 0x20,
        NES_LEFT   = 0x40,
        NES_RIGHT  = 0x80
    };
    
private:
    static fabgl::PS2Controller ps2;
    static fabgl::Keyboard keyboard;
    static uint8_t currentState;
    static void updateState();
};

#endif