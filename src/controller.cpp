// controller.cpp
#include "../config.h"
#include "controller.h"
#include "ps2_keyboard.h"
#include <Arduino.h>

// Read current controller state and return as bitmask
uint8_t controllerRead()
{
    return PS2Keyboard::read();
}

// Check if a specific button is currently pressed
bool isDownPressed(CONTROLLER button)
{
    return (controllerRead() & button) != 0;
}

// Initialize the controller input system
void initController()
{
    PS2Keyboard::init();
}