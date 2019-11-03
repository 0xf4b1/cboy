// SPDX-License-Identifier: GPL-3.0-only

#include <GL/glut.h>
#include <stdio.h>

#include "cpu.h"
#include "display.h"
#include "gameboy.h"
#include "instructions/opcodes.h"
#include "joystick.h"

Gameboy gameboy = {.controls = 0xFF,
                   .cpu.SP = 0xFFFF,
                   .cpu.ime = true,
                   .mmu.mbc.rom_bank_number = 1,
                   .mmu.mbc.ram_bank_number = 0,
                   .mmu.mbc.rom_ram_select = 0};

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
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("No rom file specified");
        exit(1);
    }

    init_joystick();
    load_rom(argv[1]);
    glutInit(&argc, argv);
    display_loop();
}