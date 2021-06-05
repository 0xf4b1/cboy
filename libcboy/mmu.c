// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameboy.h"

unsigned char read_mmu(unsigned short addr) {
    if (addr < 0x8000) {
        return read_mbc(addr);
    } else if (addr == 0xFF04) {
        return rand() % 0x100;
    }

    return gameboy.mmu.ram[addr - 0x8000];
}

void write_mmu(unsigned short addr, unsigned char value) {
    if (addr < 0x8000) {
        write_mbc(addr, value);
    } else if (addr == 0xFF00) {

        bool buttons_selected = ((value >> 5) & 1) == 0;
        bool directions_selected = ((value >> 4) & 1) == 0;

        if (buttons_selected && !directions_selected) {
            value |= gameboy.controls >> 4;
        } else if (directions_selected && !buttons_selected) {
            value |= gameboy.controls & 0xF;
        } else {
            value |= 0xF;
        }
        gameboy.mmu.ram[addr - 0x8000] = value;

    } else if (addr == 0xFF02) {
        serial_print(gameboy.mmu.ram[0xFF01 - 0x8000]);
    } else if (addr == 0xFF04) {
        gameboy.mmu.ram[0xFF04 - 0x8000] = 0;
    } else if (addr == 0xFF46) {
        memcpy(gameboy.mmu.ram + 0xFE00 - 0x8000, gameboy.mmu.ram + (value << 8) - 0x8000, 0x9F);
    } else {
        gameboy.mmu.ram[addr - 0x8000] = value;
    }
}