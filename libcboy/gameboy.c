// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>

#include "gameboy.h"
#include "instructions/opcodes.h"

Gameboy gameboy = {.controls = 0xFF,
                   .cpu.SP = 0xFFFF,
                   .cpu.ime = true,
                   .mmu.mbc.rom_bank_number = 1,
                   .mmu.mbc.ram_bank_number = 0,
                   .mmu.mbc.rom_ram_select = 0};

void inject_bootrom() {
    gameboy.mmu.mbc.rom[0x00] = 0x31;
    gameboy.mmu.mbc.rom[0x01] = 0xFE;
    gameboy.mmu.mbc.rom[0x02] = 0xFF;
    gameboy.mmu.mbc.rom[0x03] = 0xC3;
    gameboy.mmu.mbc.rom[0x04] = 0xFC;
    gameboy.mmu.mbc.rom[0x05] = 0x00;
    gameboy.mmu.mbc.rom[0xFC] = 0x3E;
    gameboy.mmu.mbc.rom[0xFD] = 0x01;
    gameboy.mmu.mbc.rom[0xFE] = 0xE0;
    gameboy.mmu.mbc.rom[0xFF] = 0x50;
}

void load_rom(char *path) {
    init_ops();

    printf("ROM path: %s\n", path);

    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("Error reading rom file!");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    unsigned long fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    gameboy.mmu.mbc.rom = malloc(fileLen + 1);

    if (!gameboy.mmu.mbc.rom) {
        fprintf(stderr, "Memory error!");
        fclose(file);
        return;
    }

    fread(gameboy.mmu.mbc.rom, fileLen, 1, file);
    fclose(file);

    inject_bootrom();
}