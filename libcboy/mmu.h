// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_MMU_H
#define LIBCBOY_MMU_H

#include <stdbool.h>

#include "mbc.h"

typedef struct {
    unsigned char ram[0x8000];
    unsigned char vram_bank[0x2000];
    unsigned char wram[7][0x1000];
    unsigned char bg_palette[0x1000];
    unsigned char sprite_palette[0x1000];
    Mbc mbc;
} Mmu;

unsigned char read_mmu(unsigned short addr);
void write_mmu(unsigned short addr, unsigned char value);

inline void set_interrupt(unsigned char value) { write_mmu(0xFF0F, read_mmu(0xFF0F) | (1 << value)); }
inline void set_vblank() { set_interrupt(0); }
inline void set_lcd_stat() { set_interrupt(1); }

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
inline void set_mode(unsigned char mode) {
    unsigned char value = read_mmu(0xFF41) & 3; // get 2-LSB
    unsigned char mask = value ^ mode;          // XOR with target mode
    write_mmu(0xFF41, read_mmu(0xFF41) ^ mask);
}

inline unsigned char lyc() { return read_mmu(0xFF45); }

inline void set_coincidence_flag(bool value) {
    if (value) {
        write_mmu(0xFF41, read_mmu(0xFF41) | (1 << 2));
    } else {
        write_mmu(0xFF41, read_mmu(0xFF41) & ~(1 << 2));
    }
}

inline bool coincidence_interrupt() { return read_mmu(0xFF41) >> 6 & 1; }

/*
 * FF44 - LY - LCDC Y-Coordinate (R) The LY indicates the vertical line to which the present data is transferred
 * to the LCD Driver. The LY can take on any value between 0 through 153. The values between 144 and 153
 * indicate the V-Blank period. Writing will reset the counter.
 */
inline void set_ly(unsigned char y) {
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

inline unsigned char lcdc() { return read_mmu(0xFF40); }

inline bool obj_sprite_size() { return lcdc() >> 2 & 1; }
inline bool bg_tile_map_display_select() { return lcdc() >> 3 & 1; }
inline bool bg_window_tile_data_select() { return lcdc() >> 4 & 1; }
inline bool window_display_enable() { return lcdc() >> 5 & 1; }
inline bool window_tile_map_display_select() { return lcdc() >> 6 & 1; }
inline bool lcd_display_enable() { return lcdc() >> 7 & 1; }

inline unsigned short get_tile(unsigned char x, unsigned char y, bool window) {
    bool map_display_select = window ? window_tile_map_display_select() : bg_tile_map_display_select();
    unsigned char tile = read_mmu((map_display_select ? 0x9C00 : 0x9800) + y * 32 + x);

    return (bg_window_tile_data_select() ? 0x8000 : 0x9000) +
           (!bg_window_tile_data_select() || map_display_select ? (tile ^ 0x80) - 0x80 : tile) * 16;
}

#endif // LIBCBOY_MMU_H