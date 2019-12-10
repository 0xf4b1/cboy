// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_MBC_H
#define LIBCBOY_MBC_H

typedef struct {
    char *filename;
    unsigned char *rom;

    unsigned char rom_bank_number;
    unsigned char ram_bank_number;
    unsigned char rom_ram_select;
} Mbc;

unsigned char read_mbc(unsigned short addr);

void write_mbc(unsigned short addr, unsigned char value);

#endif // LIBCBOY_MBC_H
