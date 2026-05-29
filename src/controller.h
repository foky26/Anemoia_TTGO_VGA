// controller.h
#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <stdint.h>

// Button bitmask values - MUST match PS2Keyboard::NESButton
enum CONTROLLER
{
    A = (1 << 0),      // 0x01 - Z key
    B = (1 << 1),      // 0x02 - X key
    Select = (1 << 2), // 0x04 - SPACE key
    Start = (1 << 3),  // 0x08 - ENTER key
    Up = (1 << 4),     // 0x10 - UP ARROW
    Down = (1 << 5),   // 0x20 - DOWN ARROW
    Left = (1 << 6),   // 0x40 - LEFT ARROW
    Right = (1 << 7)   // 0x80 - RIGHT ARROW
};

// Function declarations
void initController();
uint8_t controllerRead();
bool isDownPressed(CONTROLLER button);

#endif