// SPDX-License-Identifier: GPL-3.0-only

pub struct Mbc {
    pub filename: String,
    pub rom: Vec<u8>,
    pub ram: [[u8; 0x2000]; 4],
    pub rom_bank_number: u8,
    pub ram_bank_number: u8,
    pub rom_ram_select: bool,
    pub ram_enable: bool,
}

impl Default for Mbc {
    fn default() -> Self {
        Self {
            filename: String::new(),
            rom: Vec::new(),
            ram: [[0u8; 0x2000]; 4],
            rom_bank_number: 1,
            ram_bank_number: 0,
            rom_ram_select: false,
            ram_enable: false,
        }
    }
}

impl Mbc {
    pub fn read(&self, addr: u16) -> u8 {
        if addr < 0x4000 {
            return self.rom[addr as usize];
        }
        let bank = (self.rom_bank_number as usize).saturating_sub(1);
        let offset = bank * 0x4000 + addr as usize;
        if offset < self.rom.len() {
            self.rom[offset]
        } else {
            0xFF
        }
    }

    pub fn write(&mut self, addr: u16, value: u8) {
        if addr >= 0x2000 && addr < 0x4000 {
            self.rom_bank_number = if value > 1 { value } else { 1 };
        } else if addr < 0x6000 {
            self.ram_bank_number = value;
        } else if addr < 0x8000 {
            self.rom_ram_select = value != 0;
        }
    }
}
