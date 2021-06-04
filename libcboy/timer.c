// SPDX-License-Identifier: GPL-3.0-only

#include "gameboy.h"
#include "mmu.h"

int count = 0;
int ticks = 0;

void timer(unsigned char cycles) {
    count += cycles;
    if (count < 16)
        return;

    ticks++;
    count %= 16;

    if (ticks % 4 == 0)
        // FF04 - DIV - Divider Register
        gameboy.mmu.ram[0xFF04 - 0x8000] += 1;

    /* FF07 - TAC - Timer Control
     * Bit 2    - Timer Stop  (0=Stop, 1=Start)
     * Bits 1-0 - Input Clock Select
     *            00:   4096 Hz    (~4194 Hz SGB)
     *            01: 262144 Hz  (~268400 Hz SGB)
     *            10:  65536 Hz   (~67110 Hz SGB)
     *            11:  16384 Hz   (~16780 Hz SGB)
     */
    unsigned char TAC = gameboy.mmu.ram[0xFF07 - 0x8000];

    // Timer enable
    if ((TAC >> 2) & 1) {
        unsigned char clock = TAC & 3;
        if ((clock == 0 && (ticks % 64 == 0)) ||
            clock == 1 ||
            (clock == 2 && (ticks % 4 == 0)) ||
            (clock == 3 && (ticks % 16 == 0))) {
            // FF05 - TIMA - Timer counter
            if (gameboy.mmu.ram[0xFF05 - 0x8000] == 0xff) {
                set_interrupt(2);
                gameboy.mmu.ram[0xFF05 - 0x8000] = gameboy.mmu.ram[0xFF06 - 0x8000];
            } else {
                gameboy.mmu.ram[0xFF05 - 0x8000] += 1;
            }
        }
    }
}