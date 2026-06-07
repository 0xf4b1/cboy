// SPDX-License-Identifier: GPL-3.0-only

use std::fs;
use crate::mmu::Mmu;
use crate::display::{Frame, HEIGHT};

pub struct Gameboy {
    pub mmu: Mmu,
    pub framebuffer: Frame,

    // CPU registers
    pub a: u8,
    pub f: u8,
    pub b: u8,
    pub c: u8,
    pub d: u8,
    pub e: u8,
    pub h: u8,
    pub l: u8,
    pub sp: u16,
    pub pc: u16,
    pub ime: bool,
    pub halt: bool,

    // Timer state
    pub timer_count: i32,
    pub timer_ticks: i32,

    // Display state
    pub scy: [u8; HEIGHT + 1],
    pub scx: [u8; HEIGHT + 1],
    pub wy: [u8; HEIGHT + 1],
    pub wx: [u8; HEIGHT + 1],
}

impl Default for Gameboy {
    fn default() -> Self {
        Self {
            mmu: Mmu::default(),
            framebuffer: Frame::default(),
            a: 0,
            f: 0,
            b: 0,
            c: 0,
            d: 0,
            e: 0,
            h: 0,
            l: 0,
            sp: 0xFFFF,
            pc: 0,
            ime: true,
            halt: false,
            timer_count: 0,
            timer_ticks: 0,
            scy: [0u8; HEIGHT + 1],
            scx: [0u8; HEIGHT + 1],
            wy: [0u8; HEIGHT + 1],
            wx: [0u8; HEIGHT + 1],
        }
    }
}

impl Gameboy {
    // ---- Register pair helpers ----
    pub fn af(&self) -> u16 { ((self.a as u16) << 8) | self.f as u16 }
    pub fn bc(&self) -> u16 { ((self.b as u16) << 8) | self.c as u16 }
    pub fn de(&self) -> u16 { ((self.d as u16) << 8) | self.e as u16 }
    pub fn hl(&self) -> u16 { ((self.h as u16) << 8) | self.l as u16 }

    pub fn set_af(&mut self, v: u16) { self.a = (v >> 8) as u8; self.f = (v & 0xF0) as u8; }
    pub fn set_bc(&mut self, v: u16) { self.b = (v >> 8) as u8; self.c = v as u8; }
    pub fn set_de(&mut self, v: u16) { self.d = (v >> 8) as u8; self.e = v as u8; }
    pub fn set_hl(&mut self, v: u16) { self.h = (v >> 8) as u8; self.l = v as u8; }

    // ---- Flag helpers ----
    pub fn flag_z(&self) -> bool { self.f >> 7 & 1 != 0 }
    pub fn flag_n(&self) -> bool { self.f >> 6 & 1 != 0 }
    pub fn flag_h(&self) -> bool { self.f >> 5 & 1 != 0 }
    pub fn flag_c(&self) -> bool { self.f >> 4 & 1 != 0 }

    pub fn set_flag_z(&mut self, v: bool) { if v { self.f |= 1 << 7 } else { self.f &= !(1 << 7) } }
    pub fn set_flag_n(&mut self, v: bool) { if v { self.f |= 1 << 6 } else { self.f &= !(1 << 6) } }
    pub fn set_flag_h(&mut self, v: bool) { if v { self.f |= 1 << 5 } else { self.f &= !(1 << 5) } }
    pub fn set_flag_c(&mut self, v: bool) { if v { self.f |= 1 << 4 } else { self.f &= !(1 << 4) } }

    // ---- MMU read/write ----
    pub fn read_mmu(&self, addr: u16) -> u8 {
        return self.mmu.read(addr);
    }

    pub fn write_mmu(&mut self, addr: u16, value: u8) {
        self.mmu.write(addr, value);
    }

    // ---- Interrupt helpers ----
    pub fn set_interrupt(&mut self, bit: u8) {
        let v = self.read_mmu(0xFF0F) | (1 << bit);
        self.write_mmu(0xFF0F, v);
    }

    pub fn set_vblank(&mut self) { self.set_interrupt(0); }
    pub fn set_lcd_stat(&mut self) { self.set_interrupt(1); }

    // ---- LCD helpers ----
    pub fn set_mode(&mut self, mode: u8) {
        let value = self.read_mmu(0xFF41) & 3;
        let mask = value ^ mode;
        let stat = self.read_mmu(0xFF41) ^ mask;
        self.write_mmu(0xFF41, stat);
    }

    pub fn lyc(&self) -> u8 { self.read_mmu(0xFF45) }

    pub fn set_coincidence_flag(&mut self, value: bool) {
        if value {
            let v = self.read_mmu(0xFF41) | (1 << 2);
            self.write_mmu(0xFF41, v);
        } else {
            let v = self.read_mmu(0xFF41) & !(1 << 2);
            self.write_mmu(0xFF41, v);
        }
    }

    pub fn coincidence_interrupt(&self) -> bool { self.read_mmu(0xFF41) >> 6 & 1 != 0 }

    pub fn set_ly(&mut self, y: u8) {
        self.write_mmu(0xFF44, y);
        if self.lyc() == y {
            self.set_coincidence_flag(true);
            if self.coincidence_interrupt() {
                self.set_lcd_stat();
            }
        } else {
            self.set_coincidence_flag(false);
        }
    }

    pub fn lcdc(&self) -> u8 { self.read_mmu(0xFF40) }
    pub fn obj_sprite_size(&self) -> bool { self.lcdc() >> 2 & 1 != 0 }
    pub fn bg_tile_map_display_select(&self) -> bool { self.lcdc() >> 3 & 1 != 0 }
    pub fn bg_window_tile_data_select(&self) -> bool { self.lcdc() >> 4 & 1 != 0 }
    pub fn window_display_enable(&self) -> bool { self.lcdc() >> 5 & 1 != 0 }
    pub fn window_tile_map_display_select(&self) -> bool { self.lcdc() >> 6 & 1 != 0 }
    pub fn lcd_display_enable(&self) -> bool { self.lcdc() >> 7 & 1 != 0 }

    pub fn get_tile(&self, x: u8, y: u8, window: bool) -> u16 {
        let map_select = if window { self.window_tile_map_display_select() } else { self.bg_tile_map_display_select() };
        let tile = self.read_mmu((if map_select { 0x9C00 } else { 0x9800 }) + y as u16 * 32 + x as u16);
        let base: u16 = if self.bg_window_tile_data_select() { 0x8000 } else { 0x9000 };
        let index: i32 = if !self.bg_window_tile_data_select() || map_select {
            ((tile ^ 0x80) as i32) - 0x80
        } else {
            tile as i32
        };
        (base as i32 + index * 16) as u16
    }

    // ---- Controls ----
    pub fn press(&mut self, i: u8) { self.mmu.controls &= !(1 << i); }
    pub fn release(&mut self, i: u8) { self.mmu.controls |= 1 << i; }
    pub fn release_all(&mut self) { self.mmu.controls = 0xFF; }

    // ---- ROM loading ----
    pub fn load_rom(&mut self, path: &str) {
        let rom = fs::read(path).expect("Failed to read ROM file");
        self.mmu.mbc.filename = path.to_string();
        self.mmu.mbc.rom = rom;
        self.init();
    }

    fn init(&mut self) {
        self.mmu.ram = [0u8; 0x8000];
        self.mmu.bg_palette = [0u8; 0x40];
        self.mmu.sprite_palette = [0u8; 0x40];
        self.mmu.vram_bank = [0u8; 0x2000];

        self.pc = 0x100;
        self.sp = 0xFFFE;
        self.set_af(0x11B0);
        self.set_bc(0x0013);
        self.set_de(0x00D8);
        self.set_hl(0x014D);
        self.ime = true;
        self.halt = false;
        self.timer_count = 0;
        self.timer_ticks = 0;

        self.write_mmu(0xFF05, 0x00);
        self.write_mmu(0xFF06, 0x00);
        self.write_mmu(0xFF07, 0x00);
        self.write_mmu(0xFF10, 0x80);
        self.write_mmu(0xFF11, 0xBF);
        self.write_mmu(0xFF12, 0xF3);
        self.write_mmu(0xFF14, 0xBF);
        self.write_mmu(0xFF16, 0x3F);
        self.write_mmu(0xFF17, 0x00);
        self.write_mmu(0xFF19, 0xBF);
        self.write_mmu(0xFF1A, 0x7F);
        self.write_mmu(0xFF1B, 0xFF);
        self.write_mmu(0xFF1C, 0x9F);
        self.write_mmu(0xFF1E, 0xBF);
        self.write_mmu(0xFF20, 0xFF);
        self.write_mmu(0xFF21, 0x00);
        self.write_mmu(0xFF22, 0x00);
        self.write_mmu(0xFF23, 0xBF);
        self.write_mmu(0xFF24, 0x77);
        self.write_mmu(0xFF25, 0xF3);
        self.write_mmu(0xFF26, 0xF1);
        self.write_mmu(0xFF40, 0x91);
        self.write_mmu(0xFF42, 0x00);
        self.write_mmu(0xFF43, 0x00);
        self.write_mmu(0xFF45, 0x00);
        self.write_mmu(0xFF47, 0xFC);
        self.write_mmu(0xFF48, 0xFF);
        self.write_mmu(0xFF49, 0xFF);
        self.write_mmu(0xFF4A, 0x00);
        self.write_mmu(0xFF4B, 0x00);
        self.write_mmu(0xFFFF, 0x00);
    }

    // ---- Save/Load state ----
    pub fn save_state(&self) {
        // Serialize minimal state: RAM + CPU registers
        let mut data = Vec::new();
        data.extend_from_slice(&self.mmu.ram);
        data.extend_from_slice(&self.mmu.vram_bank);
        for bank in &self.mmu.wram { data.extend_from_slice(bank); }
        data.extend_from_slice(&self.mmu.bg_palette);
        data.extend_from_slice(&self.mmu.sprite_palette);
        for bank in &self.mmu.mbc.ram { data.extend_from_slice(bank); }
        data.push(self.mmu.mbc.rom_bank_number);
        data.push(self.mmu.mbc.ram_bank_number);
        data.push(self.mmu.mbc.rom_ram_select as u8);
        data.push(self.mmu.mbc.ram_enable as u8);
        // CPU
        data.push(self.a); data.push(self.f); data.push(self.b); data.push(self.c);
        data.push(self.d); data.push(self.e); data.push(self.h); data.push(self.l);
        data.extend_from_slice(&self.sp.to_le_bytes());
        data.extend_from_slice(&self.pc.to_le_bytes());
        data.push(self.ime as u8);
        data.push(self.halt as u8);

        let path = format!("{}.sav", self.mmu.mbc.filename);
        let _ = fs::write(&path, &data);
    }

    pub fn load_state(&mut self) {
        let path = format!("{}.sav", self.mmu.mbc.filename);
        let data = match fs::read(&path) {
            Ok(d) => d,
            Err(_) => { eprintln!("No state for current rom exists!"); return; }
        };
        let mut i = 0;
        self.mmu.ram.copy_from_slice(&data[i..i+0x8000]); i += 0x8000;
        self.mmu.vram_bank.copy_from_slice(&data[i..i+0x2000]); i += 0x2000;
        for bank in &mut self.mmu.wram { bank.copy_from_slice(&data[i..i+0x1000]); i += 0x1000; }
        self.mmu.bg_palette.copy_from_slice(&data[i..i+0x40]); i += 0x40;
        self.mmu.sprite_palette.copy_from_slice(&data[i..i+0x40]); i += 0x40;
        for bank in &mut self.mmu.mbc.ram { bank.copy_from_slice(&data[i..i+0x2000]); i += 0x2000; }
        self.mmu.mbc.rom_bank_number = data[i]; i += 1;
        self.mmu.mbc.ram_bank_number = data[i]; i += 1;
        self.mmu.mbc.rom_ram_select = data[i] != 0; i += 1;
        self.mmu.mbc.ram_enable = data[i] != 0; i += 1;
        self.a = data[i]; i += 1; self.f = data[i]; i += 1;
        self.b = data[i]; i += 1; self.c = data[i]; i += 1;
        self.d = data[i]; i += 1; self.e = data[i]; i += 1;
        self.h = data[i]; i += 1; self.l = data[i]; i += 1;
        self.sp = u16::from_le_bytes([data[i], data[i+1]]); i += 2;
        self.pc = u16::from_le_bytes([data[i], data[i+1]]); i += 2;
        self.ime = data[i] != 0; i += 1;
        self.halt = data[i] != 0;
    }
}
