// SPDX-License-Identifier: GPL-3.0-only

use crate::gameboy::Gameboy;

impl Gameboy {
    pub fn timer_step(&mut self, cycles: u8) {
        self.timer_count += cycles as i32;
        if self.timer_count < 16 {
            return;
        }

        self.timer_ticks += 1;
        self.timer_count %= 16;

        if self.timer_ticks % 4 == 0 {
            let div = self.mmu.ram[(0xFF04 - 0x8000) as usize].wrapping_add(1);
            self.mmu.ram[(0xFF04 - 0x8000) as usize] = div;
        }

        let tac = self.mmu.ram[(0xFF07 - 0x8000) as usize];

        if (tac >> 2) & 1 != 0 {
            let clock = tac & 3;
            let should_tick = match clock {
                0 => self.timer_ticks % 64 == 0,
                1 => true,
                2 => self.timer_ticks % 4 == 0,
                3 => self.timer_ticks % 16 == 0,
                _ => false,
            };

            if should_tick {
                let tima = self.mmu.ram[(0xFF05 - 0x8000) as usize];
                if tima == 0xFF {
                    self.set_interrupt(2);
                    let tma = self.mmu.ram[(0xFF06 - 0x8000) as usize];
                    self.mmu.ram[(0xFF05 - 0x8000) as usize] = tma;
                } else {
                    self.mmu.ram[(0xFF05 - 0x8000) as usize] = tima + 1;
                }
            }
        }
    }
}
