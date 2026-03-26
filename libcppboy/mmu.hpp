// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <array>
#include "controls.hpp"
#include "mbc.hpp"

namespace cboy {

constexpr size_t RAM_SIZE = 0x8000;
constexpr size_t VRAM_SIZE = 0x2000;
constexpr size_t WRAM_SIZE = 0x1000;
constexpr size_t PALETTE_SIZE = 0x1000;

class MMU {
public:
    using IsCgbCallback = bool (*)(void*);
    using SerialPrintCallback = void (*)(void*, char);

    MMU(Controls& controls, IsCgbCallback is_cgb, SerialPrintCallback serial_print, void* context = nullptr);
    
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t value);
    
    uint8_t read_rom(uint16_t addr) const;
    uint8_t get_vram_byte(uint16_t addr, bool bank) const;
    uint32_t get_bg_palette(uint8_t index) const;
    uint32_t get_sprite_palette(uint8_t index) const;
    
    void load_rom(std::ifstream& file, size_t size);
    void initialize();
    
    // LCD Status functions
    void set_interrupt(uint8_t value);
    void set_vblank() { set_interrupt(0); }
    void set_lcd_stat() { set_interrupt(1); }
    
    void set_mode(uint8_t mode);
    void set_ly(uint8_t y);
    bool is_cgb() const { return m_is_cgb(m_callback_context); }
    
    uint8_t lyc() const { return read(0xFF45); }
    uint8_t lcdc() const { return read(0xFF40); }
    
    bool obj_sprite_size() const { return (lcdc() >> 2) & 1; }
    bool bg_tile_map_display_select() const { return (lcdc() >> 3) & 1; }
    bool bg_window_tile_data_select() const { return (lcdc() >> 4) & 1; }
    bool window_display_enable() const { return (lcdc() >> 5) & 1; }
    bool window_tile_map_display_select() const { return (lcdc() >> 6) & 1; }
    bool lcd_display_enable() const { return (lcdc() >> 7) & 1; }
    
    uint16_t get_tile(uint8_t x, uint8_t y, bool window) const;

private:
    Controls& m_controls;
    IsCgbCallback m_is_cgb;
    SerialPrintCallback m_serial_print;
    void* m_callback_context = nullptr;
    std::array<uint8_t, RAM_SIZE> m_ram;
    std::array<uint8_t, VRAM_SIZE> m_vram_bank;
    std::array<std::array<uint8_t, WRAM_SIZE>, 7> m_wram;
    std::array<uint8_t, PALETTE_SIZE> m_bg_palette;
    std::array<uint8_t, PALETTE_SIZE> m_sprite_palette;
    MBC m_mbc;
    
    uint8_t read_internal(uint16_t addr) const;
    void write_internal(uint16_t addr, uint8_t value);
};

} // namespace cboy
