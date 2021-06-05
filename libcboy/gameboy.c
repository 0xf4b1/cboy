// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameboy.h"

Gameboy gameboy = {.controls = 0xFF,
                   .mmu.mbc.rom_bank_number = 1,
                   .mmu.mbc.ram_bank_number = 0,
                   .mmu.mbc.rom_ram_select = 0};

void init() {
    memset(gameboy.mmu.ram, 0, 0x8000);

    cpu.PC = 0x100;
    cpu.SP = 0xfffe;
    set_AF(0x11b0);
    set_BC(0x13);
    set_DE(0xd8);
    set_HL(0x14d);

    write_mmu(0xFF05, 0x0);
    write_mmu(0xFF06, 0x0);
    write_mmu(0xFF07, 0x0);
    write_mmu(0xFF10, 0x80);
    write_mmu(0xFF11, 0xbf);
    write_mmu(0xFF12, 0xf3);
    write_mmu(0xFF14, 0xbf);
    write_mmu(0xFF16, 0x3f);
    write_mmu(0xFF17, 0x0);
    write_mmu(0xFF19, 0xbf);
    write_mmu(0xFF1a, 0x7f);
    write_mmu(0xFF1b, 0xff);
    write_mmu(0xFF1c, 0x9f);
    write_mmu(0xFF1e, 0xbf);
    write_mmu(0xFF20, 0xff);
    write_mmu(0xFF21, 0x0);
    write_mmu(0xFF22, 0x0);
    write_mmu(0xFF23, 0xbf);
    write_mmu(0xFF24, 0x77);
    write_mmu(0xFF25, 0xf3);
    write_mmu(0xFF26, 0xf1);
    write_mmu(0xFF40, 0x91);
    write_mmu(0xFF42, 0x0);
    write_mmu(0xFF43, 0x0);
    write_mmu(0xFF45, 0x0);
    write_mmu(0xFF47, 0xfc);
    write_mmu(0xFF48, 0xff);
    write_mmu(0xFF49, 0xff);
    write_mmu(0xFF4a, 0x0);
    write_mmu(0xFF4b, 0x0);
    write_mmu(0xFFFF, 0x0);
}

void load_rom(char *path) {
    printf("ROM path: %s\n", path);

    char *filename = malloc(strlen(path) + 1);
    strcpy(filename, path);
    gameboy.mmu.mbc.filename = filename;

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

    init();
}

void load_state() {
    char filename[strlen(gameboy.mmu.mbc.filename) + 4];
    stpcpy(filename, gameboy.mmu.mbc.filename);
    strcat(filename, ".sav");

    FILE *file = fopen(filename, "rb");

    if (!file) {
        printf("No state for current rom exists!\n");
        return;
    }

    // read ram state
    fread(gameboy.mmu.ram, sizeof(char), sizeof(gameboy.mmu.ram), file);

    // read cpu state
    fread(&cpu, sizeof(Cpu), 1, file);

    fclose(file);
}

void save_state() {
    char filename[strlen(gameboy.mmu.mbc.filename) + 4];
    stpcpy(filename, gameboy.mmu.mbc.filename);
    strcat(filename, ".sav");

    FILE *file = fopen(filename, "wb");

    // save ram state
    fwrite(gameboy.mmu.ram, sizeof(char), sizeof(gameboy.mmu.ram), file);

    // save cpu state
    fwrite(&cpu, sizeof(Cpu), 1, file);

    fclose(file);
}
