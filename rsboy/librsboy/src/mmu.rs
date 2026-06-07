// SPDX-License-Identifier: GPL-3.0-only

use crate::mbc::Mbc;

pub struct Mmu {
    pub ram: [u8; 0x8000],
    pub vram_bank: [u8; 0x2000],
    pub wram: [[u8; 0x1000]; 7],
    pub bg_palette: [u8; 0x40],
    pub sprite_palette: [u8; 0x40],
    pub mbc: Mbc,
    pub controls: u8,

    // Serial output callback
    pub serial_buf: Vec<u8>,
}

impl Default for Mmu {
    fn default() -> Self {
        Self {
            ram: [0u8; 0x8000],
            vram_bank: [0u8; 0x2000],
            wram: [[0u8; 0x1000]; 7],
            bg_palette: [0u8; 0x40],
            sprite_palette: [0u8; 0x40],
            mbc: Mbc::default(),
            controls: 0xFF,
            serial_buf: Vec::new(),
        }
    }
}

impl Mmu {
    // ---- MMU read/write ----
    pub fn read(&self, addr: u16) -> u8 {
        if addr < 0x8000 {
            return self.mbc.read(addr);
        }

        if addr == 0xFF69 {
            let bcps = self.read(0xFF68);
            return self.bg_palette[(bcps & 0x3f) as usize];
        }

        if addr == 0xFF6B {
            let ocps = self.read(0xFF6A);
            return self.sprite_palette[(ocps & 0x3f) as usize];
        }

        if self.cgb() {
            if addr >= 0x8000 && addr <= 0x9FFF && self.read(0xFF4F) & 1 != 0 {
                return self.vram_bank[(addr - 0x8000) as usize];
            }

            if addr >= 0xD000 && addr <= 0xDFFF {
                let bank = {
                    let b = self.read(0xFF70);
                    if b > 0 { (b - 1) as usize } else { 0 }
                };
                return self.wram[bank][(addr - 0xD000) as usize];
            }

            if addr == 0xFF4D {
                if self.ram[(addr - 0x8000) as usize] & 1 != 0 {
                    return 1 << 7;
                } else {
                    return 0;
                }
            }

            if addr == 0xFF55 {
                return 1 << 7;
            }
        }

        self.ram[(addr - 0x8000) as usize]
    }

    pub fn write(&mut self, addr: u16, value: u8) {
        if addr < 0x8000 {
            self.mbc.write(addr, value);
            return;
        }

        if addr == 0xFF00 {
            let buttons_selected = (value >> 5) & 1 == 0;
            let directions_selected = (value >> 4) & 1 == 0;
            let mut v = value;
            if buttons_selected && !directions_selected {
                v |= self.controls >> 4;
            } else if directions_selected && !buttons_selected {
                v |= self.controls & 0xF;
            } else {
                v |= 0xF;
            }
            self.ram[(addr - 0x8000) as usize] = v;
            return;
        }

        if addr == 0xFF02 {
            let c = self.ram[(0xFF01 - 0x8000) as usize];
            self.serial_output(c);
            return;
        }

        if addr == 0xFF04 {
            self.ram[(0xFF04 - 0x8000) as usize] = 0;
            return;
        }

        if addr == 0xFF46 {
            for i in 0u16..=0x9F {
                let src = self.read((value as u16) << 8 | i);
                self.write(0xFE00 + i, src);
            }
            return;
        }

        if !self.cgb() {
            self.ram[(addr - 0x8000) as usize] = value;
            return;
        }

        // CGB-specific writes
        if addr >= 0x8000 && addr <= 0x9FFF && self.read(0xFF4F) & 1 != 0 {
            self.vram_bank[(addr - 0x8000) as usize] = value;
            return;
        }

        if addr >= 0xD000 && addr <= 0xDFFF {
            let bank = {
                let b = self.read(0xFF70);
                if b > 0 { (b - 1) as usize } else { 0 }
            };
            self.wram[bank][(addr - 0xD000) as usize] = value;
            return;
        }

        if addr == 0xFF55 {
            let source = ((self.read(0xFF51) as u16) << 8) | self.read(0xFF52) as u16;
            let target = ((self.read(0xFF53) as u16) << 8) | self.read(0xFF54) as u16;
            let len = ((value as u16 & 0x7f) + 1) * 0x10;
            for i in 0..len {
                let v = self.read(source + i);
                self.write(target + i, v);
            }
            return;
        }

        if addr == 0xFF69 {
            let bcps = self.read(0xFF68);
            self.bg_palette[(bcps & 0x3f) as usize] = value;
            if bcps >> 7 & 1 != 0 {
                self.write(0xFF68, bcps.wrapping_add(1));
            }
            return;
        }

        if addr == 0xFF6B {
            let ocps = self.read(0xFF6A);
            self.sprite_palette[(ocps & 0x3f) as usize] = value;
            if ocps >> 7 & 1 != 0 {
                self.write(0xFF6A, ocps.wrapping_add(1));
            }
            return;
        }

        self.ram[(addr - 0x8000) as usize] = value;
    }

    pub fn cgb(&self) -> bool {
        return self.mbc.read(0x143) == 0x80 || self.mbc.read(0x143) == 0xC0;
    }

    // ---- Serial output (overridable via callback) ----
    pub fn serial_output(&mut self, c: u8) {
        self.serial_buf.push(c);
    }
}