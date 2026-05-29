// config.h
#ifndef CONFIG_H
#define CONFIG_H

// ========== DISABLE UNUSED FEATURES ==========
#define CONFIG_ESP32_WIFI_ENABLED 0
#define CONFIG_BT_ENABLED 0
#define CONFIG_BLE_ENABLED 0

// ========== VIDEO OUTPUT ==========
#define USE_VGA

// VGA pins 
#define PIN_RED_LOW     21
#define PIN_RED_HIGH    22
#define PIN_GREEN_LOW   18
#define PIN_GREEN_HIGH  19
#define PIN_BLUE_LOW    4
#define PIN_BLUE_HIGH   5
#define PIN_HSYNC       23
#define PIN_VSYNC       15

// VGA pin configuration macro
#define VGA_PINS { PIN_RED_LOW, PIN_RED_HIGH, PIN_GREEN_LOW, PIN_GREEN_HIGH, PIN_BLUE_LOW, PIN_BLUE_HIGH, PIN_HSYNC, PIN_VSYNC }

// ========== CONTROLLER INPUT ==========
#define USE_PS2_KEYBOARD

// PS/2 pins for TTGO VGA32
#define PS2_CLK_PIN     33
#define PS2_DAT_PIN     32

// ========== I2S AUDIO OUTPUT ==========
#define SAMPLE_RATE     22050
#define AUDIO_BUFFER_SIZE 128

// ========== SD CARD ==========
#define SD_CS_PIN       13
#define SD_MOSI_PIN     12
#define SD_MISO_PIN     2
#define SD_SCLK_PIN     14
#define SD_FREQ         20000000

#define NESSKIP

// ========== DEBUG ==========
//#define DEBUG 1


#endif