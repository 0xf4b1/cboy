// SPDX-License-Identifier: GPL-3.0-only

#include "controls.hpp"
#include "mmu.hpp"
#include <cstring>
#include <fstream>

namespace cboy {

MMU::MMU(Controls& controls, IsCgbCallback is_cgb, SerialPrintCallback serial_print, void* context)
    : m_controls(controls),
      m_is_cgb(is_cgb),
      m_serial_print(serial_print),
      m_callback_context(context),
      m_ram{},
      m_vram_bank{},
      m_wram{},
      m_bg_palette{},
      m_sprite_palette{} {
}

void MMU::load_rom(std::ifstream& file, size_t size) {
    m_mbc.load_rom(file, size);
}

void MMU::initialize() {
    std::memset(m_ram.data(), 0, RAM_SIZE);
    std::memset(m_bg_palette.data(), 0, PALETTE_SIZE);
    std::memset(m_sprite_palette.data(), 0, PALETTE_SIZE);
    std::memset(m_vram_bank.data(), 0, VRAM_SIZE);
    
    // Initialize sound registers
    write(0xFF10, 0x80);
    write(0xFF11, 0xBF);
    write(0xFF12, 0xF3);
    write(0xFF14, 0xBF);
    write(0xFF16, 0x3F);
    write(0xFF17, 0x00);
    write(0xFF19, 0xBF);
    write(0xFF1A, 0x7F);
    write(0xFF1B, 0xFF);
    write(0xFF1C, 0x9F);
    write(0xFF1E, 0xBF);
    write(0xFF20, 0xFF);
    write(0xFF21, 0x00);
    write(0xFF22, 0x00);
    write(0xFF23, 0xBF);
    write(0xFF24, 0x77);
    write(0xFF25, 0xF3);
    write(0xFF26, 0xF1);
    
    // Initialize display registers
    write(0xFF40, 0x91);
    write(0xFF42, 0x00);
    write(0xFF43, 0x00);
    write(0xFF45, 0x00);
    write(0xFF47, 0xFC);
    write(0xFF48, 0xFF);
    write(0xFF49, 0xFF);
    write(0xFF4A, 0x00);
    write(0xFF4B, 0x00);
    write(0xFFFF, 0x00);
}

uint8_t MMU::read(uint16_t addr) const {
    if (addr < 0x8000)
        return m_mbc.read(addr);
    
    if (addr == 0xFF69) {
        uint8_t bcps = read(0xFF68);
        return m_bg_palette[bcps & 0x3F];
    }
    
    if (addr == 0xFF6B) {
        uint8_t ocps = read(0xFF6A);
        return m_sprite_palette[ocps & 0x3F];
    }
    
    if (m_is_cgb(m_callback_context)) {
        // CGB VRAM
        if (addr >= 0x8000 && addr <= 0x9FFF && read(0xFF4F) & 1)
            return m_vram_bank[addr - 0x8000];
        
        // CGB WRAM
        if (addr >= 0xD000 && addr <= 0xDFFF) {
            uint8_t bank = read(0xFF70);
            if (bank > 0)
                bank--;
            return m_wram[bank][addr - 0xD000];
        }
        
        // FF4D - CGB Speed Switch
        if (addr == 0xFF4D) {
            if (m_ram[addr - 0x8000] & 1)
                return 1 << 7;
            return 0;
        }
        
        // FF55 - CGB DMA
        if (addr == 0xFF55)
            return 1 << 7;
    }
    
    return m_ram[addr - 0x8000];
}

void MMU::write(uint16_t addr, uint8_t value) {
    if (addr < 0x8000) {
        m_mbc.write(addr, value);
        return;
    }
    
    if (addr == 0xFF00) {
        // Joypad register
        bool buttons_selected = ((value >> 5) & 1) == 0;
        bool directions_selected = ((value >> 4) & 1) == 0;
        
        if (buttons_selected && !directions_selected) {
            value |= m_controls.get_state() >> 4;
        } else if (directions_selected && !buttons_selected) {
            value |= m_controls.get_state() & 0xF;
        } else {
            value |= 0xF;
        }
        m_ram[addr - 0x8000] = value;
        return;
    }
    
    if (addr == 0xFF02) {
        // Serial output
        m_serial_print(m_callback_context, static_cast<char>(m_ram[0xFF01 - 0x8000]));
        return;
    }
    
    if (addr == 0xFF04) {
        // Timer DIV reset
        m_ram[0xFF04 - 0x8000] = 0;
        return;
    }
    
    if (addr == 0xFF46) {
        // DMA transfer
        for (uint8_t i = 0; i <= 0x9F; ++i) {
            write(0xFE00 + i, read(static_cast<uint16_t>((value << 8) + i)));
        }
        return;
    }
    
    if (!m_is_cgb(m_callback_context)) {
        m_ram[addr - 0x8000] = value;
        return;
    }
    
    // CGB-specific handling
    if (addr >= 0x8000 && addr <= 0x9FFF && read(0xFF4F) & 1) {
        m_vram_bank[addr - 0x8000] = value;
        return;
    }
    
    if (addr >= 0xD000 && addr <= 0xDFFF) {
        uint8_t bank = read(0xFF70);
        if (bank > 0)
            bank--;
        m_wram[bank][addr - 0xD000] = value;
        return;
    }
    
    if (addr == 0xFF55) {
        // CGB DMA
        uint16_t source = static_cast<uint16_t>((read(0xFF51) << 8) | read(0xFF52));
        uint16_t target = static_cast<uint16_t>((read(0xFF53) << 8) | read(0xFF54));
        uint16_t len = static_cast<uint16_t>(((value & 0x7F) + 1) * 0x10);
        
        for (uint16_t i = 0; i < len; ++i) {
            write(target + i, read(source + i));
        }
        return;
    }
    
    if (addr == 0xFF69) {
        uint8_t bcps = read(0xFF68);
        m_bg_palette[bcps & 0x3F] = value;
        
        if ((bcps >> 7) & 1)
            write(0xFF68, bcps + 1);
        return;
    }
    
    if (addr == 0xFF6B) {
        uint8_t ocps = read(0xFF6A);
        m_sprite_palette[ocps & 0x3F] = value;
        
        if ((ocps >> 7) & 1)
            write(0xFF6A, ocps + 1);
        return;
    }
    
    m_ram[addr - 0x8000] = value;
}

uint8_t MMU::read_rom(uint16_t addr) const {
    return m_mbc.read(addr);
}

uint8_t MMU::get_vram_byte(uint16_t addr, bool bank) const {
    if (bank)
        return m_vram_bank[addr];
    return m_ram[addr];
}

uint32_t MMU::get_bg_palette(uint8_t index) const {
    return static_cast<uint32_t>(m_bg_palette[index * 4]) |
           (static_cast<uint32_t>(m_bg_palette[index * 4 + 1]) << 8) |
           (static_cast<uint32_t>(m_bg_palette[index * 4 + 2]) << 16) |
           (static_cast<uint32_t>(m_bg_palette[index * 4 + 3]) << 24);
}

uint32_t MMU::get_sprite_palette(uint8_t index) const {
    return static_cast<uint32_t>(m_sprite_palette[index * 4]) |
           (static_cast<uint32_t>(m_sprite_palette[index * 4 + 1]) << 8) |
           (static_cast<uint32_t>(m_sprite_palette[index * 4 + 2]) << 16) |
           (static_cast<uint32_t>(m_sprite_palette[index * 4 + 3]) << 24);
}

void MMU::set_interrupt(uint8_t value) {
    write(0xFF0F, static_cast<uint8_t>(read(0xFF0F) | (1 << value)));
}

void MMU::set_mode(uint8_t mode) {
    uint8_t value = read(0xFF41) & 3;
    uint8_t mask = value ^ mode;
    write(0xFF41, read(0xFF41) ^ mask);
}

void MMU::set_ly(uint8_t y) {
    write(0xFF44, y);
    
    if (lyc() == y) {
        write(0xFF41, read(0xFF41) | (1 << 2));
        if ((read(0xFF41) >> 6) & 1) {
            set_lcd_stat();
        }
    } else {
        write(0xFF41, read(0xFF41) & ~(1 << 2));
    }
}

uint16_t MMU::get_tile(uint8_t x, uint8_t y, bool window) const {
    bool map_display_select = window ? window_tile_map_display_select() :
                                       bg_tile_map_display_select();
    uint8_t tile = read((map_display_select ? 0x9C00 : 0x9800) + y * 32 + x);
    
    return (bg_window_tile_data_select() ? 0x8000 : 0x9000) +
           (!bg_window_tile_data_select() || map_display_select ? 
            (tile ^ 0x80) - 0x80 : tile) * 16;
}

} // namespace cboy::mmu
