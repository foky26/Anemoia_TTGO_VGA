#include "bus.h"
#include "../vga_adapter.h"



bool vga_buffer_needs_update = false;

Bus::Bus()
{
    memset(RAM, 0, sizeof(RAM));
    cpu.connectBus(this);
    cpu.apu.connectBus(this);
    ppu.connectBus(this);
}

Bus::~Bus()
{
}

IRAM_ATTR void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
    if (cart->cpuWrite(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {
        RAM[addr & 0x07FF] = data;
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        ppu.cpuWrite(addr, data);
    }
    else if ((addr & 0xF000) == 0x4000 && (addr <= 0x4013 || addr == 0x4015 || addr == 0x4017))
    {
        cpu.apuWrite(addr, data);
    }
    else if (addr == 0x4014)
    {
        cpu.OAM_DMA(data);
    }
    else if (addr == 0x4016)
    {
        controller_strobe = data & 1;
        if (controller_strobe)
        {
            controller_state = controller;
        }
    }
}

IRAM_ATTR uint8_t Bus::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;

    if (cart->cpuRead(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {   
        data = RAM[addr & 0x07FF];
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        data = ppu.cpuRead(addr);
    }
    else if (addr == 0x4016)
    {
        uint8_t value = controller_state & 1;
        if (!controller_strobe)
            controller_state >>= 1;
        data = value | 0x40;
    }
    return data;
}

void Bus::reset()
{
    VGADisplay* display = (VGADisplay*)ptr_screen;
    if (display) display->fillScreen(0x0000);
    
    for (auto& i : RAM) i = 0x00;
    cart->reset();
    cpu.reset();
    cpu.apu.reset();
    ppu.reset();
}

IRAM_ATTR void Bus::clock()
{
    if (!frame_latch)
    {
        for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
        {
            cpu.clock(113);
            ppu.renderScanline(ppu_scanline);

            cpu.clock(114);
            ppu.renderScanline(ppu_scanline + 1);

            cpu.clock(114);
            ppu.renderScanline(ppu_scanline + 2);
        }
    }
    else
    {
        for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
        {
            cpu.clock(113);
            ppu.fakeScanline(ppu_scanline);

            cpu.clock(114);
            ppu.fakeScanline(ppu_scanline + 1);

            cpu.clock(114);
            ppu.fakeScanline(ppu_scanline + 2);
        }
    }

    cpu.clock(113);
    ppu.setVBlank();
    cpu.clock(2501);
    ppu.clearVBlank();
    cpu.clock(114);

#ifdef NESSKIP
    frame_latch = !frame_latch;
#endif
}

IRAM_ATTR void Bus::setPPUMirrorMode(Cartridge::MIRROR mirror)
{
    ppu.setMirror(mirror);
}

Cartridge::MIRROR Bus::getPPUMirrorMode()
{
    return ppu.getMirror();
}

IRAM_ATTR void Bus::OAM_Write(uint8_t addr, uint8_t data)
{
    ppu.ptr_sprite[addr] = data;
}

void Bus::insertCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    cpu.connectCartridge(cartridge);
    ppu.connectCartridge(cartridge);
    cart->connectBus(this);
}

void Bus::connectScreen(VGADisplay* screen)
{
    ptr_screen = screen;
    // Connect linear framebuffer and sync bits directly to the PPU
    ppu.connectFramebuffer(screen->getRawFramebuffer(), screen->getSyncBits());
}

IRAM_ATTR void Bus::renderImage(uint16_t scanline)
{
    (void)scanline;
}

IRAM_ATTR void Bus::IRQ()
{
    cpu.IRQ();
}

IRAM_ATTR void Bus::NMI()
{
    cpu.NMI();
}

void Bus::saveState()
{
    if (!SD.exists("/states")) SD.mkdir("/states");
    uint32_t CRC32 = cart->CRC32;

    char CRC32_str[9];
    sprintf(CRC32_str, "%08X", CRC32);

    char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);

    File state = SD.open(filename, FILE_WRITE);
    if (!state) return;

    state.print("ANEMOIA");
    state.write((const uint8_t*)CRC32_str, 8);

    state.write(RAM, sizeof(RAM));
    cpu.dumpState(state);
    ppu.dumpState(state);
    cart->dumpState(state);

    state.close();
}

void Bus::loadState()
{
    uint32_t CRC32 = cart->CRC32;

    char CRC32_str[9];
    sprintf(CRC32_str, "%08X", CRC32);

    char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);
    if (!SD.exists(filename)) return;

    File state = SD.open(filename, FILE_READ);
    if (!state) return;

    char header[8];
    char CRC[9];
    state.read((uint8_t*)&header, 7);
    header[7] = '\0';
    state.read((uint8_t*)&CRC, 8);
    CRC[8] = '\0';

    if (strcmp(header, "ANEMOIA") != 0)
    {
        state.close();
        return;
    }
    if (strcmp(CRC, CRC32_str) != 0)
    {
        state.close();
        return;
    }
    
    state.read(RAM, sizeof(RAM));
    cpu.loadState(state);
    ppu.loadState(state);
    cart->loadState(state);

    state.close();
}
