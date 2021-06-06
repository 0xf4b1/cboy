// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_GAMEBOY_H
#define LIBCBOY_GAMEBOY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cpu.h"
#include "mmu.h"

typedef struct {
    Mmu mmu;
    unsigned char controls;
    Frame framebuffer;
    bool cgb;
} Gameboy;

extern Gameboy gameboy;

void serial_print(char c);

void load_rom(char *path);

void load_state();

void save_state();

#ifdef __cplusplus
}
#endif

#endif // LIBCBOY_GAMEBOY_H