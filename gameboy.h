// SPDX-License-Identifier: GPL-3.0-only

#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu.h"
#include "mmu.h"

typedef struct {
    Cpu cpu;
    Mmu mmu;
    unsigned char controls;
} Gameboy;

Gameboy gameboy;

#endif