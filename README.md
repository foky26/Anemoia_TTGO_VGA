<h1 align="center">
  <br>
  <b>Anemoia for TTGO VGA32</b>
  <br>
</h1>

<p align="center">
  Anemoia is a high-performance Nintendo Entertainment System (NES) emulator designed specifically for the <strong>TTGO VGA32</strong> board.  
  It outputs crisp <strong>320x240 VGA video</strong> at <strong>6-bit color (64 colors)</strong> and accepts input via <strong>PS/2 keyboard</strong>.  
  
</p>

---

## Hardware Requirements

| Component               | Specification                                |
|-------------------------|----------------------------------------------|
| **Board**               | TTGO VGA32 (ESP32-based)                    |
| **Video Output**        | VGA (320x240, 6-bit color, 64 colors)       |
| **Input**               | PS/2 Keyboard                               |
| **Audio**               | I2S DAC (built-in)                          |
| **Storage**             | microSD card (FAT32 formatted)              |

---

## Default Pin Configuration

### VGA Output
| Signal      | GPIO Pin |
|-------------|----------|
| RED Low     | 21       |
| RED High    | 22       |
| GREEN Low   | 18       |
| GREEN High  | 19       |
| BLUE Low    | 4        |
| BLUE High   | 5        |
| HSYNC       | 23       |
| VSYNC       | 15       |

### PS/2 Keyboard Input
| Signal      | GPIO Pin |
|-------------|----------|
| Clock       | 33       |
| Data        | 32       |

### SD Card (SPI)
| Signal      | GPIO Pin |
|-------------|----------|
| CS          | 13       |
| MOSI        | 12       |
| MISO        | 2        |
| SCLK        | 14       |

---

## Keyboard Controls

| NES Button  | Keyboard Key |
|-------------|--------------|
| **A**       | `Z` or `z`   |
| **B**       | `X` or `x`   |
| **Start**   | `ENTER`      |
| **Select**  | `SPACE`      |
| **Up**      | `↑` (Up Arrow) |
| **Down**    | `↓` (Down Arrow) |
| **Left**    | `←` (Left Arrow) |
| **Right**   | `→` (Right Arrow) |
| **Reset**   | `ESC` (resets the system) |

---

## Features

- **Full NES hardware emulation** (CPU 6502, PPU 2C02, APU 2A03)
- **VGA output at 320x240 resolution** with 64 simultaneous colors
- **PS/2 keyboard input** with full NES button mapping
- **I2S audio output** via built-in DAC (22.05 kHz sample rate)
- **microSD card support** for loading ROM files
- **Game selection menu** with scrollable list
- **Configurable frameskip** (adjustable for performance)


---

## Performance & Frameskip

The emulator uses an adaptive rendering system to maintain smooth gameplay:

- **Frameskip is configurable** (default: 2 = render 1 of every 3 frames)
- **CPU runs at 240 MHz** for maximum performance
- **Audio runs on separate core** (Core 0) to prevent stuttering
- **PPU rendering can be disabled** during skipped frames

---

## Required Libraries

This project would not be possible without these amazing libraries:

| Library | Author(s) | Purpose |
|---------|-----------|---------|
| [fabgl](https://github.com/fabGL-VGA/fabGL) | fdivitto | PS/2 keyboard input |
| [VGA_6bit](https://github.com/bitluni) | bitluni, Ricardo Massaro | VGA output via I2S |
| [SD](https://github.com/espressif/arduino-esp32) | Espressif | SD card access |
| [SPI](https://github.com/espressif/arduino-esp32) | Espressif | SPI communication |

### Special Thanks

- **Shim Manaloto (Shim06)** - For the great port of Anemoia to ESP32
- **bitluni** - For the original VGA library and inspiration
- **ackerman** - For the TinyNes port and VGA optimization for TTGO
- **Ricardo Massaro** - For VGA optimizations and DMA improvements
- **fdivitto (fabGL)** - For excellent PS/2 keyboard support
- **NESDev community** - For documentation on NES hardware
- **Espressif** - For the powerful ESP32 platform

---

## Getting Started

### Step 1: Prepare SD Card
1. Format a microSD card as **FAT32**
2. Copy your `.nes` ROM files to the root directory

### Step 2: Upload the Code
1. Open `Anemoia_TTGO_Claude_.ino` in Arduino IDE
2. Select **ESP32 Dev Module** as the board
3. Set **Partition Scheme** to "No OAuth (Large APP)"
4. Upload to your TTGO VGA32 board

### Step 3: Play!
1. Connect VGA monitor to TTGO VGA32
2. Connect PS/2 keyboard to the PS/2 port
3. Power on the device
4. Select a game using arrow keys and `ENTER`
5. Play! Press `ESC` at any time to reset

---

## Building from Source

### Arduino IDE Settings
Board: ESP32 Dev Module
CPU Frequency: 240 MHz (WiFi/BT)
Flash Size: 4MB (32Mb)
Partition: No OAuth (Large APP)
PSRAM: Disabled

### Compiler Optimizations

The code uses aggressive GCC optimizations:
- `-O3` (maximum speed optimization)
- `-omit-frame-pointer`
- `-finline-functions`
- Functions placed in IRAM for faster access

---

## Known Limitations

- Complex mappers (MMC3, VRC) may have minor slowdowns
- Audio is mono only (built-in DAC limitation)
- No wireless controller support
- Requires PS/2 keyboard (USB keyboards not supported without adapter)

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No VGA output | Check pin connections, ensure VGA cable is connected |
| Keyboard not working | Check PS/2 connections, try restarting |
| "SD Card Mount Failed" | Check SD card format (must be FAT32), reinsert card |
| Game runs slow | Increase frameskip value in code |
| No audio | Check I2S configuration, ensure speaker is connected |

---

## License

This project is based on the original Anemoia NES emulator and is released under the **GNU General Public License v3.0 (GPLv3)**.

See the [LICENSE](LICENSE) file for more details.

---

<p align="center">
  <sub>Built with ❤️ for the TTGO VGA32 community</sub>
</p>
