// SPDX-License-Identifier: GPL-3.0-only

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "display.h"
#include "gameboy.h"
#include "instructions/opcodes.h"

Gameboy gameboy = {.controls = 0xFF, .cpu.SP = 0xFFFF, .cpu.ime = true};

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

    fread(gameboy.mmu.ram, fileLen, 1, file);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("No rom file specified");
        exit(1);
    }

    load_rom(argv[1]);
    glutInit(&argc, argv);
    display_loop();
}