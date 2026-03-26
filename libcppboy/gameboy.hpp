// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "controls.hpp"
#include "display.hpp"
#include "cpu.hpp"
#include "mmu.hpp"

namespace cboy {

class Gameboy {
public:
    Gameboy();
    // Construct with externally-owned Controls for dependency injection
    explicit Gameboy(Controls& external_controls);
    Gameboy(MMU& external_mmu, CPU& external_cpu, Controls& external_controls);
    Gameboy(const Gameboy&) = delete;
    Gameboy& operator=(const Gameboy&) = delete;
    
    void load_rom(const std::string& path);
    void load_state();
    void save_state();
    const display::Frame& run_frame();
    
    bool is_cgb() const { return m_cgb; }
    const display::Frame& get_framebuffer() const { return m_framebuffer; }
    
    void set_pixel(uint8_t x, uint8_t y, uint16_t color) {
        if (x < 160 && y < 144) {
            m_framebuffer[y][x] = color;
        }
    }

    MMU& mmu() { return *m_mmu; }
    const MMU& mmu() const { return *m_mmu; }
    CPU& cpu() { return *m_cpu; }
    Controls& controls() { return *m_controls; }
    
    // VRAM access
    uint8_t get_vram_byte(uint16_t addr, bool bank) const;
    
    // Palette access
    uint32_t get_bg_palette(uint8_t index) const;
    uint32_t get_sprite_palette(uint8_t index) const;
    
    void serial_print(char c);

private:
    std::unique_ptr<Controls> m_owned_controls;
    Controls* m_controls = nullptr;

    std::unique_ptr<MMU> m_owned_mmu;
    MMU* m_mmu = nullptr;

    std::unique_ptr<CPU> m_owned_cpu;
    CPU* m_cpu = nullptr;

    display::Frame m_framebuffer{};
    bool m_cgb = false;
};

} // namespace cboy
