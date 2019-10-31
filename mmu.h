// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEMORY_H
#define MEMORY_H

typedef struct {
    unsigned char ram[0x10000];
} Mmu;

unsigned char read_mmu(unsigned short addr);

void write_mmu(unsigned short addr, unsigned char value);

void set_mode(unsigned char mode);

void set_ly(unsigned char y);

bool lcd_display_enable();

unsigned short get_bg_tile(unsigned char x, unsigned char y);

void set_vblank();

#endif