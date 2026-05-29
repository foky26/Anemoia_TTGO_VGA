#include "ppu2C02.h"
#include "bus.h"
#include "../vga_adapter.h"

#define READ_PALETTE(x) palette_table[((x) & 0x1F) ^ (((x) & 0x13) == 0x10 ? 0x10 : 0x00)]



// ==================== PALETTE DEFINITIONS ====================
constexpr uint16_t Ppu2C02::palette_NTSC565[8][64];
constexpr uint16_t Ppu2C02::palette_PAL565[8][64];
constexpr uint16_t Ppu2C02::palette_NTSC222[8][64];
constexpr uint16_t Ppu2C02::palette_PAL222[8][64];
constexpr uint8_t Ppu2C02::palette_mirror[32];

// ==================== NES->VGA CONVERSION TABLE (64 bytes) ====================
uint8_t Ppu2C02::nes_to_vga[64];



// ==================== CONSTRUCTOR ====================
Ppu2C02::Ppu2C02()
{
    memset(scanline_buffer, 0, sizeof(scanline_buffer));
    memset(scanline_metadata, 0, sizeof(scanline_metadata));
    memset(nametable, 0, sizeof(nametable));
    memset(palette_table, 0, sizeof(palette_table));
    memset(sprite, 0, sizeof(sprite));
    
    nes_palette = palette_NTSC565;
    current_emphasize = 0;
    vga_framebuffer = nullptr;
    vga_sync_bits = 0;
    
    rebuildNesToVga();
}

Ppu2C02::~Ppu2C02() {}

void Ppu2C02::rebuildNesToVga()
{
    const uint16_t (*pal)[64] = nes_palette;
    uint8_t emph = current_emphasize;
    for (int i = 0; i < 64; i++) {
        nes_to_vga[i] = rgb565_to_vga6(pal[emph][i]);
    }
}

void Ppu2C02::connectFramebuffer(uint8_t* fb, uint8_t sync)
{
    vga_framebuffer = fb;
    vga_sync_bits = sync;
}

inline void Ppu2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;
    if (cart->ppuWrite(addr, data)) return;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
        ptr_nametable[(addr >> 10) & 3][addr & 0x03FF] = data;
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr = palette_mirror[addr & 0x001F];
        palette_table[addr] = data;
    }
}

inline uint8_t Ppu2C02::ppuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;
    if (cart->ppuRead(addr, data)) return data;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
        data = ptr_nametable[(addr >> 10) & 3][addr & 0x03FF];
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x001F;
        switch (addr)
        {
            case 0x0010: addr = 0x0000; break;
            case 0x0014: addr = 0x0004; break;
            case 0x0018: addr = 0x0008; break;
            case 0x001C: addr = 0x000C; break;
        }
        data = palette_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }
    return data;
}

IRAM_ATTR void Ppu2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
        case 0x2000:
            control.reg = data;
            t.nametable_x = control.nametable_x;
            t.nametable_y = control.nametable_y;
            break;
        case 0x2001:
            mask.reg = data;
            if (current_emphasize != mask.emphasize) {
                current_emphasize = mask.emphasize;
                rebuildNesToVga();
            }
            break;
        case 0x2003: OAMADDR = data; break;
        case 0x2004: ptr_sprite[OAMADDR++] = data; break;
        case 0x2005:
            if (w == 0)
            {
                x = data & 0x07;
                t.coarse_x = data >> 3;
            }
            else
            {
                t.fine_y = data & 0x07;
                t.coarse_y = data >> 3;
            }
            w = ~w;
            break;
        case 0x2006:
            if (w == 0)
                t.reg = (t.reg & 0x00FF) | (uint16_t)((data & 0x3F) << 8);
            else
            {
                t.reg = (t.reg & 0xFF00) | data;
                v.reg = t.reg;
            }
            w = ~w;
            break;
        case 0x2007:
            ppuWrite(v.reg, data);
            v.reg += (control.VRAM_addr_increment ? 32 : 1);
            break;
    }
}

IRAM_ATTR uint8_t Ppu2C02::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    switch (addr)
    {
        case 0x2002:
            data = status.reg & 0xE0;
            status.VBlank = 0;
            w = 0;
            break;
        case 0x2004: data = ptr_sprite[OAMADDR]; break;
        case 0x2007:
            data = PPUDATA_buffer;
            PPUDATA_buffer = ppuRead(v.reg);
            if (v.reg >= 0x3F00 && v.reg <= 0x3FFF) data = PPUDATA_buffer;
            v.reg += (control.VRAM_addr_increment ? 32 : 1);
            break;
    }
    return data;
}

IRAM_ATTR void Ppu2C02::setVBlank() { status.VBlank = 1; if (control.Vblank_NMI) bus->NMI(); }
IRAM_ATTR void Ppu2C02::clearVBlank() { status.VBlank = 0; status.sprite_zero_hit = 0; status.sprite_overflow = 0; }

IRAM_ATTR void Ppu2C02::renderScanline(uint16_t scanline)
{
	sprite_count = 0;  
    transferScroll(scanline);
    renderBackground();
    renderSprites(scanline);
    incrementY();
    finishScanline(scanline);
}



inline void Ppu2C02::transferScroll(uint16_t scanline)
{
    if (!(mask.reg & (1 << 3) || mask.reg & (1 << 4))) return;
    v.reg = (scanline == 0) ? t.reg : (v.reg & ~0x041F) | (t.reg & 0x041F);
}

inline void Ppu2C02::incrementY()
{
    if (!(mask.render_background || mask.render_sprite)) return;
    if (v.fine_y < 7) v.fine_y++;
    else
    {
        v.fine_y = 0;
        if (v.coarse_y == 29) { v.coarse_y = 0; v.nametable_y = ~v.nametable_y; }
        else if (v.coarse_y == 31) v.coarse_y = 0;
        else v.coarse_y++;
    }
}

IRAM_ATTR void Ppu2C02::renderBackground()
{   
     if (render_enabled){
    if (!mask.render_background)
    {
        uint8_t bg_color = palette_table[0];  // NES index, not VGA
        memset(scanline_buffer, bg_color, BUFFER_SIZE);
        memset(scanline_metadata, 0x80, BUFFER_SIZE);
        ptr_buffer = scanline_buffer + x;
        ptr_scanline_meta = scanline_metadata + x;
        return;
    }

    // Cache current palette to avoid double indirection in inner loop
    const uint16_t (*current_palette)[64] = nes_palette;
    uint8_t emph = current_emphasize;
    
    uint8_t bg_color = palette_table[0];
    ptr_buffer = scanline_buffer;
    ptr_scanline_meta = scanline_metadata;
    x_tile = v.coarse_x;
    y_tile = v.coarse_y;
    offset = (control.background_table_addr ? 0x1000 : 0x0000) + v.fine_y;
    nametable_index = (v.reg >> 10) & 3;

    nametable_byte_base = v.reg & 0x03E0;
    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base + x_tile];

    attribute_byte_base = 0x03C0 + ((y_tile & 0x1C) << 1);
    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base + (x_tile >> 2)];
    attribute_byte = *ptr_attribute++;
    attribute_shift = ((y_tile & 2) << 1) + (x_tile & 2);
    attribute = ((attribute_byte >> attribute_shift) & 3) << 2;

    static constexpr uint8_t pixel_shift[8] = { 14, 6, 12, 4, 10, 2, 8, 0 };
    static constexpr uint8_t pixel_metadata[4] = { 0x80, 0x00, 0x00, 0x00 };
    
    for (int tile = 0; tile < 33; tile++)
    {
        tile_index = *ptr_tile++;
        ptr_pattern_tile = cart->ppuReadPtr(offset + (tile_index << 4)); 

        uint16_t pattern = ((ptr_pattern_tile[8] & 0xAA) << 8) | ((ptr_pattern_tile[8] & 0x55) << 1)
                    | ((ptr_pattern_tile[0] & 0xAA) << 7) | (ptr_pattern_tile[0] & 0x55);
        
        // Get NES palette indices (0-63)
        uint8_t tile_palette[4];
        tile_palette[0] = bg_color;
        for (int t = 1; t < 4; t++) {
            tile_palette[t] = READ_PALETTE(attribute + t);
        }
        
        for (int i = 0; i < 8; i++)
        {   
            uint8_t pixel = (pattern >> pixel_shift[i]) & 3;
            *ptr_buffer++ = tile_palette[pixel];
            *ptr_scanline_meta++ = pixel_metadata[pixel];
        }

        x_tile++;
        if ((x_tile & 1) == 0)
        {
            if ((x_tile & 3) == 0)
            {
                if (x_tile == 32)
                {
                    x_tile = 0;
                    nametable_index ^= 1;
                    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base];
                    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base];
                }
                attribute_byte = *ptr_attribute++;
            }
            attribute_shift ^= 2;
            attribute = ((attribute_byte >> attribute_shift) & 0x03) << 2;
        }
    }
    ptr_buffer = scanline_buffer + x;
}
}


IRAM_ATTR void Ppu2C02::renderSprites(uint16_t scanline)
{
    static constexpr uint8_t pixel_shift_normal[8]  = { 14, 6, 12, 4, 10, 2, 8, 0 };
    static constexpr uint8_t pixel_shift_flipped[8] = {  0, 8, 2, 10, 4, 12, 6, 14 };

    if (!mask.render_sprite) return;

    OAM* ptr_sprite_OAM = sprite;
    const uint8_t sprite_size = control.sprite_size ? 16 : 8;
    const uint8_t bg_color    = palette_table[0];
    offset = control.sprite_table_addr ? 0x1000 : 0;

    uint8_t* buffer_offset   = scanline_buffer  + x;
    uint8_t* metadata_offset = scanline_metadata + x;

    for (int i = 0; i < 64; i++, ptr_sprite_OAM++)
    {
        uint8_t sprite_y = ptr_sprite_OAM->y + 1;
        if (sprite_y > scanline || sprite_y <= (scanline - sprite_size) || sprite_y >= 240) continue;

        const uint8_t attribute_byte_local = ptr_sprite_OAM->attribute;
        const uint8_t tile_index_local     = ptr_sprite_OAM->index;
        const uint8_t sprite_x             = ptr_sprite_OAM->x;

        uint16_t tile_addr = control.sprite_size
            ? ((tile_index_local & 0x01) << 12) | ((tile_index_local & 0xFE) << 4)
            : offset + (tile_index_local << 4);
        const uint8_t* t = cart->ppuReadPtr(tile_addr);

        int16_t y_offset = scanline - sprite_y;
        if (y_offset > 7) y_offset += 8;
        if (attribute_byte_local & 0x80) {
            y_offset -= control.sprite_size ? 23 : 7;
            t -= y_offset;
        } else {
            t += y_offset;
        }

        // Extract 8 pixels with constant shifts — no 16-bit pattern
        const uint8_t lo = t[0], hi = t[8];
        uint8_t px[8];
        if (attribute_byte_local & 0x40) {
            px[0]=((lo>>0)&1)|(((hi>>0)&1)<<1); px[1]=((lo>>1)&1)|(((hi>>1)&1)<<1);
            px[2]=((lo>>2)&1)|(((hi>>2)&1)<<1); px[3]=((lo>>3)&1)|(((hi>>3)&1)<<1);
            px[4]=((lo>>4)&1)|(((hi>>4)&1)<<1); px[5]=((lo>>5)&1)|(((hi>>5)&1)<<1);
            px[6]=((lo>>6)&1)|(((hi>>6)&1)<<1); px[7]=((lo>>7)&1)|(((hi>>7)&1)<<1);
        } else {
            px[0]=((lo>>7)&1)|(((hi>>7)&1)<<1); px[1]=((lo>>6)&1)|(((hi>>6)&1)<<1);
            px[2]=((lo>>5)&1)|(((hi>>5)&1)<<1); px[3]=((lo>>4)&1)|(((hi>>4)&1)<<1);
            px[4]=((lo>>3)&1)|(((hi>>3)&1)<<1); px[5]=((lo>>2)&1)|(((hi>>2)&1)<<1);
            px[6]=((lo>>1)&1)|(((hi>>1)&1)<<1); px[7]=((lo>>0)&1)|(((hi>>0)&1)<<1);
        }

        if (!(px[0]|px[1]|px[2]|px[3]|px[4]|px[5]|px[6]|px[7])) continue;

        // Palette resolved here, only if sprite has visible pixels
        const uint8_t spr_pal = (attribute_byte_local & 0x03) << 2;
        uint8_t tile_palette[4] = {
            bg_color,
            READ_PALETTE(17 + spr_pal),
            READ_PALETTE(18 + spr_pal),
            READ_PALETTE(19 + spr_pal)
        };

        uint8_t* buf  = buffer_offset   + sprite_x;
        uint8_t* meta = metadata_offset + sprite_x;

        if (i == 0 && !status.sprite_zero_hit) {
            for (int j = 0; j < 8; j++) {
                if (px[j] && !(meta[j] & 0x80)) { status.sprite_zero_hit = true; break; }
            }
        }

        if (attribute_byte_local & 0x20) {
            for (int j = 0; j < 8; j++) {
                if (px[j]) {
                    if (meta[j] & 0x80) buf[j] = tile_palette[px[j]];
                    meta[j] |= 0x40;
                }
            }
        } else {
            for (int j = 0; j < 8; j++) {
                if (px[j] && !(meta[j] & 0x40)) {
                    buf[j] = tile_palette[px[j]];
                    meta[j] |= 0x40;
                }
            }
        }

        if (++sprite_count == 8) { status.sprite_overflow = true; break; }
    }
    ptr_buffer = buffer_offset;
}

IRAM_ATTR void Ppu2C02::fakeScanline(uint16_t scanline)
{
    sprite_count = 0;
    transferScroll(scanline);      // keeps v synchronized
    incrementY();                  // advances internal Y scroll
    if (mask.render_background || mask.render_sprite) cart->ppuScanline();
    if (!mask.render_sprite || status.sprite_zero_hit) return;

    // Only sprite zero hit, same as before
    uint8_t sprite_size = control.sprite_size ? 16 : 8;
    uint8_t sprite_y = sprite[0].y + 1;
    if (sprite_y > scanline || sprite_y <= (scanline - sprite_size) || sprite_y >= 240) return;

    offset = control.sprite_table_addr ? 0x1000 : 0;
    uint16_t tile_addr = control.sprite_size
        ? ((sprite[0].index & 0x01) << 12) | ((sprite[0].index & 0xFE) << 4)
        : offset + (sprite[0].index << 4);
    const uint8_t* t = cart->ppuReadPtr(tile_addr);

    int16_t y_offset = scanline - sprite_y;
    if (y_offset > 7) y_offset += 8;
    if (sprite[0].attribute & 0x80) t -= (y_offset - (control.sprite_size ? 23 : 7));
    else t += y_offset;

    uint16_t pattern = ((t[8] & 0xAA) << 8) | ((t[8] & 0x55) << 1)
                     | ((t[0] & 0xAA) << 7) | (t[0] & 0x55);
    if (pattern) status.sprite_zero_hit = true;
}




IRAM_ATTR void Ppu2C02::finishScanline(uint16_t scanline)
{
    if (mask.render_background || mask.render_sprite) 
        cart->ppuScanline();

    // Only render if enabled
    if (render_enabled && vga_framebuffer && scanline < 240) {
        uint8_t* dst = vga_framebuffer + (scanline * 320) + 32;
        const uint8_t* src = scanline_buffer;
        const uint8_t sv = vga_sync_bits;
       

 for (int i = 0; i <SCANLINE_SIZE; i += 4) {
        dst[i+2] = sv | nes_to_vga[src[i]   & 0x3F];
        dst[i+3] = sv | nes_to_vga[src[i+1] & 0x3F];
        dst[i+0] = sv | nes_to_vga[src[i+2] & 0x3F];
        dst[i+1] = sv | nes_to_vga[src[i+3] & 0x3F];
    }

    }

    vga_buffer_needs_update = true;
}

void Ppu2C02::reset()
{
    status.reg = 0x00;
    mask.reg = 0x1E;
    control.reg = 0x00;
    t.reg = 0x00;
    v.reg = 0x00;
    x = 0x00;
    w = 0x00;
    OAMADDR = 0x00;
    OAMDATA = 0x00;
    PPUDATA_buffer = 0x00;
    palette_table[0] = 0x0F;
    current_emphasize = 0;
    rebuildNesToVga();
}

void Ppu2C02::connectCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    setMirror((Cartridge::MIRROR)cart->hardware_mirror);
}

void Ppu2C02::setMirror(Cartridge::MIRROR mirror)
{
    switch (mirror)
    {
        case Cartridge::MIRROR::VERTICAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0400];
            ptr_nametable[2] = &nametable[0x0000];
            ptr_nametable[3] = &nametable[0x0400];
            break;
        case Cartridge::MIRROR::HORIZONTAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0000];
            ptr_nametable[2] = &nametable[0x0400];
            ptr_nametable[3] = &nametable[0x0400];
            break;
        case Cartridge::MIRROR::ONESCREEN_LOW:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0000];
            break;
        case Cartridge::MIRROR::ONESCREEN_HIGH:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0400];
            break;
    }
}

Cartridge::MIRROR Ppu2C02::getMirror()
{
    if (ptr_nametable[0] == &nametable[0x0000] && ptr_nametable[1] == &nametable[0x0400] &&
        ptr_nametable[2] == &nametable[0x0000] && ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::VERTICAL;
    if (ptr_nametable[0] == &nametable[0x0000] && ptr_nametable[1] == &nametable[0x0000] &&
        ptr_nametable[2] == &nametable[0x0400] && ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::HORIZONTAL;
    if (ptr_nametable[0] == &nametable[0x0000] && ptr_nametable[1] == &nametable[0x0000] &&
        ptr_nametable[2] == &nametable[0x0000] && ptr_nametable[3] == &nametable[0x0000])
        return Cartridge::MIRROR::ONESCREEN_LOW;
    if (ptr_nametable[0] == &nametable[0x0400] && ptr_nametable[1] == &nametable[0x0400] &&
        ptr_nametable[2] == &nametable[0x0400] && ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::ONESCREEN_HIGH;
    return Cartridge::MIRROR::HORIZONTAL;
}

void Ppu2C02::setPalette(uint8_t palette)
{
    switch (palette)
    {
        case NTSC565: nes_palette = palette_NTSC565; break;
        case PAL565:  nes_palette = palette_PAL565; break;
        case NTSC222: nes_palette = palette_NTSC222; break;
        case PAL222:  nes_palette = palette_PAL222; break;
        default: nes_palette = palette_NTSC565; break;
    }
    rebuildNesToVga();
}

void Ppu2C02::dumpState(File& state)
{
    state.write(scanline_buffer, sizeof(scanline_buffer));
    state.write(scanline_metadata, sizeof(scanline_metadata));
    state.write(nametable, sizeof(nametable));
    for (int i = 0; i < 4; i++)
    {
        uint8_t map = (ptr_nametable[i] == &nametable[0x0400]) ? 1 : 0;
        state.write(&map, sizeof(map));
    }
    state.write(palette_table, sizeof(palette_table));
    state.write((uint8_t*)&control.reg, sizeof(control.reg));
    state.write((uint8_t*)&mask.reg, sizeof(mask.reg));
    state.write((uint8_t*)&status.reg, sizeof(status.reg));
    state.write(&OAMADDR, sizeof(OAMADDR));
    state.write(&OAMDATA, sizeof(OAMDATA));
    state.write((uint8_t*)sprite, sizeof(sprite));
    state.write((uint8_t*)&v.reg, sizeof(v.reg));
    state.write((uint8_t*)&t.reg, sizeof(t.reg));
    state.write(&x, sizeof(x));
    state.write(&w, sizeof(w));
    state.write(&PPUDATA_buffer, sizeof(PPUDATA_buffer));
    state.write((uint8_t*)&offset, sizeof(offset));
    state.write(&nametable_index, sizeof(nametable_index));
    state.write((uint8_t*)&nametable_byte_base, sizeof(nametable_byte_base));
    state.write((uint8_t*)&attribute_byte_base, sizeof(attribute_byte_base));
    state.write(&attribute_byte, sizeof(attribute_byte));
    state.write(&x_tile, sizeof(x_tile));
    state.write(&y_tile, sizeof(y_tile));
    state.write(&attribute_shift, sizeof(attribute_shift));
    state.write(&attribute, sizeof(attribute));
    state.write(&tile_index, sizeof(tile_index));
    state.write(&sprite_count, sizeof(sprite_count));
}

void Ppu2C02::loadState(File& state)
{
    state.read(scanline_buffer, sizeof(scanline_buffer));
    state.read(scanline_metadata, sizeof(scanline_metadata));
    state.read(nametable, sizeof(nametable));
    for (int i = 0; i < 4; i++)
    {
        uint8_t map;
        state.read(&map, sizeof(map));
        ptr_nametable[i] = &nametable[map ? 0x0400 : 0x0000];
    }
    state.read(palette_table, sizeof(palette_table));
    state.read((uint8_t*)&control.reg, sizeof(control.reg));
    state.read((uint8_t*)&mask.reg, sizeof(mask.reg));
    state.read((uint8_t*)&status.reg, sizeof(status.reg));
    state.read(&OAMADDR, sizeof(OAMADDR));
    state.read(&OAMDATA, sizeof(OAMDATA));
    state.read((uint8_t*)sprite, sizeof(sprite));
    state.read((uint8_t*)&v.reg, sizeof(v.reg));
    state.read((uint8_t*)&t.reg, sizeof(t.reg));
    state.read(&x, sizeof(x));
    state.read(&w, sizeof(w));
    state.read(&PPUDATA_buffer, sizeof(PPUDATA_buffer));
    state.read((uint8_t*)&offset, sizeof(offset));
    state.read(&nametable_index, sizeof(nametable_index));
    state.read((uint8_t*)&nametable_byte_base, sizeof(nametable_byte_base));
    state.read((uint8_t*)&attribute_byte_base, sizeof(attribute_byte_base));
    state.read(&attribute_byte, sizeof(attribute_byte));
    state.read(&x_tile, sizeof(x_tile));
    state.read(&y_tile, sizeof(y_tile));
    state.read(&attribute_shift, sizeof(attribute_shift));
    state.read(&attribute, sizeof(attribute));
    state.read(&tile_index, sizeof(tile_index));
    state.read(&sprite_count, sizeof(sprite_count));
    
    // Rebuild conversion table after loading state
    rebuildNesToVga();
}