// SPDX-License-Identifier: GPL-3.0-only

#include <string.h>

#include "gameboy.h"

unsigned char read_mmu(unsigned short addr) {
    if (addr < 0x8000)
        return read_mbc(addr);

    if (addr == 0xFF69) {
        unsigned char bcps = read_mmu(0xFF68);
        return gameboy.mmu.bg_palette[bcps & 0x3f];
    }

    if (addr == 0xFF6B) {
        unsigned char ocps = read_mmu(0xFF6A);
        return gameboy.mmu.sprite_palette[ocps & 0x3f];
    }

    if (gameboy.cgb) {
        // CGB VRAM
        if (addr >= 0x8000 && addr <= 0x9FFF && read_mmu(0xFF4F) & 1)
            return gameboy.mmu.vram_bank[addr - 0x8000];

        // CGB WRAM
        if (addr >= 0xD000 && addr <= 0xDFFF) {
            unsigned char bank = read_mmu(0xFF70);
            if (bank > 0)
                bank--;
            return gameboy.mmu.wram[bank][addr - 0xD000];
        }

        // FF4D - KEY1 - CGB Mode Only - Prepare Speed Switch
        // FIXME
        if (addr == 0xFF4D) {
            if (gameboy.mmu.ram[addr - 0x8000] & 1)
                return 1 << 7;
            else
                return 0;
        }

        // FIXME
        if (addr == 0xFF55)
            return 1 << 7;
    }

    return gameboy.mmu.ram[addr - 0x8000];
}

void write_mmu(unsigned short addr, unsigned char value) {
    if (addr < 0x8000) {
        // MBC
        write_mbc(addr, value);
        return;
    }

    if (addr == 0xFF00) {
        // controls
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
        return;
    }

    if (addr == 0xFF02) {
        // serial
        serial_print(gameboy.mmu.ram[0xFF01 - 0x8000]);
        return;
    }

    if (addr == 0xFF04) {
        // timer
        gameboy.mmu.ram[0xFF04 - 0x8000] = 0;
        return;
    }

    if (addr == 0xFF46) {
        // DMA
        for (unsigned char i = 0; i <= 0x9F; i++) {
            write_mmu(0xFE00 + i, read_mmu((value << 8) + i));
        }
        return;
    }

    if (!gameboy.cgb) {
        gameboy.mmu.ram[addr - 0x8000] = value;
        return;
    }

    // Color
    if (addr >= 0x8000 && addr <= 0x9FFF && read_mmu(0xFF4F) & 1) {
        // CGB VRAM
        gameboy.mmu.vram_bank[addr - 0x8000] = value;
        return;
    }

    if (addr >= 0xD000 && addr <= 0xDFFF) {
        // CGB WRAM
        unsigned char bank = read_mmu(0xFF70);
        if (bank > 0)
            bank--;
        gameboy.mmu.wram[bank][addr - 0xD000] = value;
        return;
    }

    if (addr == 0xFF55) {
        // CGB DMA
        unsigned short source = (read_mmu(0xFF51) << 8) | read_mmu(0xFF52);
        unsigned short target = (read_mmu(0xFF53) << 8) | read_mmu(0xFF54);
        unsigned short len = ((value & 0x7f) + 1) * 0x10;

        for (unsigned short i = 0; i < len; i++) {
            write_mmu(target + i, read_mmu(source + i));
        }
        return;
    }

    if (addr == 0xFF69) {
        unsigned char bcps = read_mmu(0xFF68);
        gameboy.mmu.bg_palette[bcps & 0x3f] = value;

        // Bit 7     Auto Increment  (0=Disabled, 1=Increment after Writing)
        if (bcps >> 7 & 1)
            write_mmu(0xFF68, bcps + 1);

        return;
    }

    if (addr == 0xFF6B) {
        unsigned char ocps = read_mmu(0xFF6A);
        gameboy.mmu.sprite_palette[ocps & 0x3f] = value;

        // Bit 7     Auto Increment  (0=Disabled, 1=Increment after Writing)
        if (ocps >> 7 & 1)
            write_mmu(0xFF6A, ocps + 1);

        return;
    }

    gameboy.mmu.ram[addr - 0x8000] = value;
}