// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameboy.h"

unsigned char read_mmu(unsigned short addr) {
    if (addr == 0xFF04) {
        return rand() % 0x100;
    }

    if (addr < 0x8000) {
        return read_mbc(addr);
    }

    return gameboy.mmu.ram[addr];
}

void write_mmu(unsigned short addr, unsigned char value) {
    if (addr >= 0x2000 && addr < 0x8000) {
        write_mbc(addr, value);
        return;
    }
    if (addr <= 0x7FFF) {
        return;
    } else if (addr == 0xFF00) {
        bool buttons_selected = ((value >> 5) & 1) == 0;
        bool directions_selected = ((value >> 4) & 1) == 0;
        unsigned char res = value;

        if (buttons_selected && !directions_selected) {
            res |= gameboy.controls >> 4;
        } else if (directions_selected && !buttons_selected) {
            res |= gameboy.controls & 0xF;
        } else {
            res |= 0xF;
        }
        gameboy.mmu.ram[addr] = res;
        return;
    } else if (addr == 0xFF02) {
        printf("%c", gameboy.mmu.ram[0xFF01]);
    } else if (addr == 0xFF04) {
        gameboy.mmu.ram[0xFF04] = 0;
        return;
    } else if (addr == 0xFF46) {
        memcpy(gameboy.mmu.ram + 0xFE00, gameboy.mmu.ram + (value << 8), 0x9F);
        return;
    }

    gameboy.mmu.ram[addr] = value;
}

void set_interrupt(unsigned char value) { write_mmu(0xFF0F, read_mmu(0xFF0F) | (1 << value)); }

void set_vblank() { set_interrupt(0); }

void set_lcd_stat() { set_interrupt(1); }

/*
 * FF41 - STAT - LCDC Status (R/W)
 * Bit 6 - LYC=LY Coincidence Interrupt (1=Enable) (Read/Write)
 * Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
 * Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
 * Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
 * Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
 * Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
 *       0: During H-Blank
 *       1: During V-Blank
 *       2: During Searching OAM-RAM
 *       3: During Transfering Data to LCD Driver
 */
void set_mode(unsigned char mode) {
    unsigned char value = read_mmu(0xFF41) & 3; // get 2-LSB
    unsigned char mask = value ^ mode;          // XOR with target mode
    write_mmu(0xFF41, read_mmu(0xFF41) ^ mask);
}

unsigned char lyc() { return read_mmu(0xFF45); }

void set_coincidence_flag(bool value) {
    if (value) {
        write_mmu(0xFF41, read_mmu(0xFF41) | (1 << 2));
    } else {
        write_mmu(0xFF41, read_mmu(0xFF41) & ~(1 << 2));
    }
}

bool coincidence_interrupt() { return read_mmu(0xFF41) >> 6 & 1; }

/*
 * FF44 - LY - LCDC Y-Coordinate (R) The LY indicates the vertical line to which the present data is transferred
 * to the LCD Driver. The LY can take on any value between 0 through 153. The values between 144 and 153
 * indicate the V-Blank period. Writing will reset the counter.
 */
void set_ly(unsigned char y) {
    write_mmu(0xFF44, y);

    if (lyc() == y) {
        set_coincidence_flag(true);
        if (coincidence_interrupt()) {
            set_lcd_stat();
        }
    } else {
        set_coincidence_flag(false);
    }
}

unsigned char lcdc() { return read_mmu(0xFF40); }

bool obj_sprite_size() { return lcdc() >> 2 & 1; }

bool bg_tile_map_display_select() { return lcdc() >> 3 & 1; }

bool bg_window_tile_data_select() { return lcdc() >> 4 & 1; }

bool window_display_enable() { return lcdc() >> 5 & 1; }

bool window_tile_map_display_select() { return lcdc() >> 6 & 1; }

bool lcd_display_enable() { return lcdc() >> 7 & 1; }

unsigned short get_bg_tile(unsigned char x, unsigned char y) {
    unsigned char tile = read_mmu((bg_tile_map_display_select() ? 0x9C00 : 0x9800) + y * 32 + x);

    return (bg_window_tile_data_select() ? 0x8000 : 0x9000) +
           (!bg_window_tile_data_select() || bg_tile_map_display_select() ? (char)tile : tile) * 16;
}

unsigned short get_window_tile(unsigned char x, unsigned char y) {
    unsigned char tile = read_mmu((window_tile_map_display_select() ? 0x9C00 : 0x9800) + y * 32 + x);

    return (bg_window_tile_data_select() ? 0x8000 : 0x9000) +
           (!bg_window_tile_data_select() || window_tile_map_display_select() ? (char)tile : tile) * 16;
}
