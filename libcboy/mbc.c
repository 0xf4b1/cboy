// SPDX-License-Identifier: GPL-3.0-only

#include "gameboy.h"

unsigned char read_mbc(unsigned short addr) {
    if (addr < 0x4000) {
        return gameboy.mmu.mbc.rom[addr];
    }
    return gameboy.mmu.mbc.rom[(gameboy.mmu.mbc.rom_bank_number - 1) * 0x4000 + addr];
}

void write_mbc(unsigned short addr, unsigned char value) {
    if (addr >= 0x2000 && addr < 0x4000) {
        gameboy.mmu.mbc.rom_bank_number = value > 1 ? value : 1;
    } else if (addr >= 0x4000 && addr < 0x6000) {
        gameboy.mmu.mbc.ram_bank_number = value;
    } else if (addr >= 0x6000 && addr < 0x8000) {
        gameboy.mmu.mbc.rom_ram_select = value;
    }
}