// SPDX-License-Identifier: GPL-3.0-only

#include "timer.hpp"
#include "mmu.hpp"

namespace cboy {

static int count = 0;
static int ticks = 0;

void timer(MMU& mmu, uint8_t cycles) {
    count += cycles;
    if (count < 16)
        return;
    
    ticks++;
    count %= 16;
    
    if (ticks % 4 == 0) {
        // FF04 - DIV - Divider Register
        mmu.write(0xFF04, mmu.read(0xFF04) + 1);
    }
    
    /* FF07 - TAC - Timer Control
     * Bit 2    - Timer Stop  (0=Stop, 1=Start)
     * Bits 1-0 - Input Clock Select
     *            00:   4096 Hz    (~4194 Hz SGB)
     *            01: 262144 Hz  (~268400 Hz SGB)
     *            10:  65536 Hz   (~67110 Hz SGB)
     *            11:  16384 Hz   (~16780 Hz SGB)
     */
    uint8_t tac = mmu.read(0xFF07);
    
    // Timer enable
    if ((tac >> 2) & 1) {
        uint8_t clock = tac & 3;
        if ((clock == 0 && (ticks % 64 == 0)) ||
            clock == 1 ||
            (clock == 2 && (ticks % 4 == 0)) ||
            (clock == 3 && (ticks % 16 == 0))) {
            
            uint8_t tima = mmu.read(0xFF05);
            if (tima == 0xFF) {
                mmu.set_interrupt(2);
                mmu.write(0xFF05, mmu.read(0xFF06));
            } else {
                mmu.write(0xFF05, tima + 1);
            }
        }
    }
}

} // namespace cboy
