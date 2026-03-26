// SPDX-License-Identifier: GPL-3.0-only

#include "mbc.hpp"

namespace cboy {

MBC::MBC() : m_rom(nullptr) {
}

MBC::~MBC() = default;

void MBC::load_rom(std::ifstream& file, size_t size) {
    m_rom = std::make_unique<uint8_t[]>(size);
    file.read(reinterpret_cast<char*>(m_rom.get()), static_cast<std::streamsize>(size));
}

uint8_t MBC::read(uint16_t addr) const {
    if (!m_rom)
        return 0;
    
    if (addr < 0x4000) {
        return m_rom[addr];
    }
    return m_rom[(m_rom_bank_number - 1) * 0x4000 + addr];
}

void MBC::write(uint16_t addr, uint8_t value) {
    if (addr >= 0x2000 && addr < 0x4000) {
        m_rom_bank_number = value > 1 ? value : 1;
    } else if (addr < 0x6000) {
        m_ram_bank_number = value;
    } else if (addr < 0x8000) {
        m_rom_ram_select = value != 0;
    }
}

} // namespace cboy
