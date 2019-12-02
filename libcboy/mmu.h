// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_MMU_H
#define LIBCBOY_MMU_H

#include "mbc.h"

typedef struct {
    unsigned char ram[0x10000];
    Mbc mbc;
} Mmu;

unsigned char read_mmu(unsigned short addr);

void write_mmu(unsigned short addr, unsigned char value);

void set_mode(unsigned char mode);

void set_ly(unsigned char y);

bool lcd_display_enable();

bool obj_sprite_size();

bool window_display_enable();

unsigned short get_bg_tile(unsigned char x, unsigned char y);

unsigned short get_window_tile(unsigned char x, unsigned char y);

void set_vblank();

#endif // LIBCBOY_MMU_H