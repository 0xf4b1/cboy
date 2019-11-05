// SPDX-License-Identifier: GPL-3.0-only

#include "gameboy.h"

void press(unsigned char i) { gameboy.controls &= ~(1 << i); }

void release(unsigned char i) { gameboy.controls |= (1 << i); }

void release_all() { gameboy.controls = 0xFF; }