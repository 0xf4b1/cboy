// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <fstream>
#include <array>

namespace cboy {

class MBC {
public:
    MBC();
    ~MBC();
    
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t value);
    
    void load_rom(std::ifstream& file, size_t size);
    
    const std::string& get_filename() const { return m_filename; }
    void set_filename(const std::string& path) { m_filename = path; }
    
private:
    std::unique_ptr<uint8_t[]> m_rom;
    std::array<std::array<uint8_t, 0x2000>, 4> m_ram;
    std::string m_filename;
    
    uint8_t m_rom_bank_number = 1;
    uint8_t m_ram_bank_number = 0;
    bool m_rom_ram_select = false;
    bool m_ram_enable = false;
};

} // namespace cboy
