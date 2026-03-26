// SPDX-License-Identifier: GPL-3.0-only

#include "gameboy.hpp"
#include "cpu.hpp"
#include <fstream>
#include <cstring>
#include <iostream>

namespace cboy {
namespace {

bool gameboy_is_cgb(void* ctx) {
    return static_cast<Gameboy*>(ctx)->is_cgb();
}

void gameboy_serial_print(void* ctx, char c) {
    static_cast<Gameboy*>(ctx)->serial_print(c);
}

} // namespace

Gameboy::Gameboy()
        : m_owned_controls(std::make_unique<Controls>()),
            m_controls(m_owned_controls.get()) {
        m_owned_mmu = std::make_unique<MMU>(*m_controls,
                        &gameboy_is_cgb,
                        &gameboy_serial_print,
                        this);
        m_mmu = m_owned_mmu.get();
        m_owned_cpu = std::make_unique<CPU>(*m_mmu);
        m_cpu = m_owned_cpu.get();
}

Gameboy::Gameboy(Controls& external_controls)
        : m_owned_controls(nullptr),
            m_controls(&external_controls) {
        m_owned_mmu = std::make_unique<MMU>(*m_controls,
                        &gameboy_is_cgb,
                        &gameboy_serial_print,
                        this);
        m_mmu = m_owned_mmu.get();
        m_owned_cpu = std::make_unique<CPU>(*m_mmu);
        m_cpu = m_owned_cpu.get();
}

Gameboy::Gameboy(MMU& external_mmu, CPU& external_cpu, Controls& external_controls)
        : m_owned_controls(nullptr), m_controls(&external_controls),
            m_owned_mmu(nullptr), m_mmu(&external_mmu),
            m_owned_cpu(nullptr), m_cpu(&external_cpu) {
        // external mmu/cpu assumed already wired by caller
}

void Gameboy::load_rom(const std::string& path) {
    std::cout << "ROM path: " << path << "\n";
    
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error reading rom file!\n";
        throw std::runtime_error("Cannot open ROM file");
    }
    
    std::streamsize file_len = file.tellg();
    file.seekg(0, std::ios::beg);
    
    m_mmu->load_rom(file, static_cast<size_t>(file_len));
    
    // Detect CGB mode
    uint8_t cgb_byte = m_mmu->read_rom(0x143);
    m_cgb = (cgb_byte == 0x80 || cgb_byte == 0xC0);
    
    // Initialize CPU and MMU
    m_cpu->program_counter = 0x100;
    m_cpu->stack_pointer = 0xFFFE;
    m_cpu->set_AF(0x11B0);
    m_cpu->set_BC(0x0013);
    m_cpu->set_DE(0x00D8);
    m_cpu->set_HL(0x014D);
    
    m_mmu->initialize();
}

const display::Frame& Gameboy::run_frame() {
    return m_cpu->next_frame(m_framebuffer, m_cgb);
}

void Gameboy::load_state() {
    // Implementation
}

void Gameboy::save_state() {
    // Implementation
}

uint8_t Gameboy::get_vram_byte(uint16_t addr, bool bank) const {
    return m_mmu->get_vram_byte(addr, bank);
}

uint32_t Gameboy::get_bg_palette(uint8_t index) const {
    return m_mmu->get_bg_palette(index);
}

uint32_t Gameboy::get_sprite_palette(uint8_t index) const {
    return m_mmu->get_sprite_palette(index);
}

void Gameboy::serial_print(char c) {
    std::cout << c;
}

} // namespace cboy
