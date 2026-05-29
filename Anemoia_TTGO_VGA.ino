/* Anemoia_TTGO_VGA by Foky26 (2026)
Based on Anemoia ESP32 project by Shim Manaloto (Shim06)
Based on Anemoia Nintendo Entertainment System (NES) emulator
It uses FABGL Library by Fabrizio Di Vittorio; Website: https://github.com/fdivitto/fabgl
It uses VGA library by bitluni adapted by ackerman
GNU General Public License v3.0 (GPLv3).
*/

#pragma GCC optimize("O3")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("inline-functions")

#define CORE_DEBUG_LEVEL 0
#define CONFIG_ESP32_DEBUG_STACK_USAGE 0
#define CONFIG_OPTIMIZATION_LEVEL_RELEASE 1

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string>
#include <vector>

#include "config.h"
#include "src/core/bus.h"
#include "src/controller.h"
#include "src/ui.h"
#include "driver/i2s.h"
#include "esp32-hal-dac.h"
#include "src/vga_adapter.h"

// Global objects
VGADisplay screen;
SPIClass SD_SPI(HSPI);
UI ui(&screen);
Cartridge* cart;

// Global bus (not on stack - prevents stack overflow)
Bus nes;

// Function prototypes
void setupI2SDAC();
bool initSD();
void emulate();
void apuTask(void* param);

// ==================== SETUP ====================
void setup() 
{
    #if DEBUG
        Serial.begin(115200);
        delay(1000);
        Serial.println("\n\n=== ANEMOIA NES EMULATOR ===");
        Serial.println("Starting system...");
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    #endif
    
    // Set CPU to maximum frequency
    setCpuFrequencyMhz(240);
    
    // Initialize audio output
    setupI2SDAC();
    
    // Initialize VGA display (must be fast)
    VGADisplay::init();
    
    // Initialize SD card - no prints for speed
    if(!initSD()) {
        VGADisplay::fillScreen(VGA_COLOR_BLACK_RGB565);
        VGADisplay::setTextColor(VGA_COLOR_WHITE_RGB565, VGA_COLOR_BLACK_RGB565);
        VGADisplay::setCursor(50, 100);
        VGADisplay::print("SD Card Mount Failed!");
        VGADisplay::setCursor(50, 120);
        VGADisplay::print("Insert SD card and reset");
        VGADisplay::updateDisplay();
        while (true) {
            delay(1000);
        }
    }
    
    // Initialize game controller
    initController();
}

// ==================== MAIN LOOP ====================
void loop() 
{
    cart = ui.selectGame();
    emulate();
}

// ==================== I2S AUDIO SETUP ====================
void setupI2SDAC()
{
    i2s_config_t i2s_config = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags     = 0,
        .dma_buf_count        = 8,
        .dma_buf_len          = AUDIO_BUFFER_SIZE,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0
    };

    i2s_driver_uninstall(I2S_NUM_0);
    
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
        return;
    }
    
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
}

// ==================== SD CARD INITIALIZATION ====================
bool initSD() 
{
    SD_SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    return SD.begin(SD_CS_PIN, SD_SPI, SD_FREQ);
}

// ==================== EMULATION ENGINE ====================
void emulate()
{
    // Insert cartridge and connect peripherals
    nes.insertCartridge(cart);
    nes.connectScreen(&screen);
    nes.reset();
    ui.initializeSettings(&nes);
    screen.beginEmulation();
    
    // Configure frameskip (0 = all frames, 1 = show 1 of 2, 2 = show 1 of 3, etc.)
    uint8_t frameskip = 2;  // Show 1 out of every 3 frames
    
    // Create audio processing task on core 0
    TaskHandle_t apu_task_handle;
    xTaskCreatePinnedToCore(apuTask, "APU", 4096, &nes.cpu.apu, 3, &apu_task_handle, 0);
    
    #define FRAME_TIME 16639  // Microseconds per frame at ~60fps
    uint64_t next_frame = esp_timer_get_time();
    uint8_t frame_counter = 0;
    
    uint32_t totalFrames = 0;
    uint32_t renderedFrames = 0;
    uint64_t fpsTimer = esp_timer_get_time();

    while (true) 
    {
        // Determine if this frame should be rendered based on frameskip
        bool should_render = (frame_counter == 0);
        
        if (should_render) {
            // Enable rendering for this frame
            nes.controller = controllerRead();
            nes.ppu.setRenderEnabled(true);
            renderedFrames++;
        } else {
            // Skip rendering for this frame
            nes.ppu.setRenderEnabled(false);
        }
        
        // Execute one complete frame (CPU + PPU cycles)
        nes.clock();
        
        // Only update display if frame was rendered
        if (should_render) {
            screen.updateDisplay();
        } else {
            VGADisplay::bufferNeedsUpdate = false;
        }
        
        totalFrames++;
        
        // Advance frameskip counter (0,1,2,0,1,2,...)
        frame_counter++;
        if (frame_counter > frameskip) {
            frame_counter = 0;
        }
        
        uint64_t now = esp_timer_get_time();
        #ifdef DEBUG
        // Display statistics every 5 seconds
        if (now - fpsTimer >= 5000000) {
            float fps = totalFrames / 5.0f;
            float render_pct = (renderedFrames * 100.0f) / totalFrames;
            Serial.printf("[EMU] Real FPS: %.1f | Rendered: %d/%d (%.0f%%)\n", 
                         fps, renderedFrames, totalFrames, render_pct);
            totalFrames = 0;
            renderedFrames = 0;
            fpsTimer = now;
        }
        #endif

        // Frame timing synchronization
        if (now < next_frame) {
            ets_delay_us(next_frame - now);
        }
        next_frame += FRAME_TIME;
    }
}

// ==================== AUDIO PROCESSING TASK ====================
void apuTask(void* param) 
{
    Apu2A03* apu = (Apu2A03*)param;
    
    // Calculate cycles needed per audio buffer
    const uint32_t CYCLES_PER_BUFFER = 1789773 * AUDIO_BUFFER_SIZE / SAMPLE_RATE;
    const uint32_t DELAY_US = 0.3 * (AUDIO_BUFFER_SIZE * 1000000) / SAMPLE_RATE;
    
    while (true)
    {
        uint64_t t0 = esp_timer_get_time();
        
        // Generate audio samples
        for (uint32_t i = 0; i < CYCLES_PER_BUFFER; i++)
            apu->clock();
        
        // Maintain consistent timing
        uint64_t elapsed = esp_timer_get_time() - t0;
        if (elapsed < DELAY_US)
            ets_delay_us(DELAY_US - elapsed);
    }
}

