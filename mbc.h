// SPDX-License-Identifier: GPL-3.0-only

#ifndef CBOY_MBC_H
#define CBOY_MBC_H

typedef struct {
    unsigned char *rom;

    unsigned char rom_bank_number;
    unsigned char ram_bank_number;
    unsigned char rom_ram_select;
} Mbc;

unsigned char read_mbc(unsigned short addr);

void write_mbc(unsigned short addr, unsigned char value);

#endif // CBOY_MBC_H
