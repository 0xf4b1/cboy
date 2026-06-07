// SPDX-License-Identifier: GPL-3.0-only

use crate::gameboy::Gameboy;

impl Gameboy {
    pub fn alu_add(&mut self, a: u8, b: u8) -> u8 {
        let res = a.wrapping_add(b);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h((a & 0xF) + (b & 0xF) > 0xF);
        self.set_flag_c((a as u16) + (b as u16) > 0xFF);
        res
    }

    pub fn alu_add_hl(&mut self, a: u16, b: u16) -> u16 {
        let res = a.wrapping_add(b);
        self.set_flag_n(false);
        self.set_flag_h((a & 0xFFF) + (b & 0xFFF) > 0xFFF);
        self.set_flag_c((a as u32) + (b as u32) > 0xFFFF);
        res
    }

    pub fn alu_adc(&mut self, a: u8, b: u8) -> u8 {
        let c = self.flag_c() as u8;
        let res = a.wrapping_add(b).wrapping_add(c);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h((a & 0xF) + (b & 0xF) + c > 0xF);
        self.set_flag_c((a as u16) + (b as u16) + (c as u16) > 0xFF);
        res
    }

    pub fn alu_sub(&mut self, a: u8, b: u8) -> u8 {
        let res = a.wrapping_sub(b);
        self.set_flag_z(res == 0);
        self.set_flag_n(true);
        self.set_flag_h((a & 0xF) < (b & 0xF));
        self.set_flag_c(a < b);
        res
    }

    pub fn alu_sbc(&mut self, a: u8, b: u8) -> u8 {
        let c = self.flag_c() as u8;
        let res = a.wrapping_sub(b).wrapping_sub(c);
        self.set_flag_z(res == 0);
        self.set_flag_n(true);
        self.set_flag_h((a & 0xF) < (b & 0xF) + c);
        self.set_flag_c((a as u16) < (b as u16) + (c as u16));
        res
    }

    pub fn alu_and(&mut self, a: u8, b: u8) -> u8 {
        let res = a & b;
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(true);
        self.set_flag_c(false);
        res
    }

    pub fn alu_or(&mut self, a: u8, b: u8) -> u8 {
        let res = a | b;
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(false);
        res
    }

    pub fn alu_xor(&mut self, a: u8, b: u8) -> u8 {
        let res = a ^ b;
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(false);
        res
    }

    pub fn alu_cp(&mut self, a: u8, b: u8) {
        self.set_flag_z(a == b);
        self.set_flag_n(true);
        self.set_flag_h((a & 0xF) < (b & 0xF));
        self.set_flag_c(a < b);
    }

    pub fn alu_inc(&mut self, v: u8) -> u8 {
        self.set_flag_h((v & 0xF) + 1 > 0xF);
        let res = v.wrapping_add(1);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        res
    }

    pub fn alu_dec(&mut self, v: u8) -> u8 {
        self.set_flag_h((v & 0xF) == 0);
        let res = v.wrapping_sub(1);
        self.set_flag_z(res == 0);
        self.set_flag_n(true);
        res
    }

    pub fn stack_push(&mut self, value: u16) {
        self.write_mmu(self.sp.wrapping_sub(1), (value >> 8) as u8);
        self.write_mmu(self.sp.wrapping_sub(2), value as u8);
        self.sp = self.sp.wrapping_sub(2);
    }

    pub fn stack_pop(&mut self) -> u16 {
        let lo = self.read_mmu(self.sp) as u16;
        let hi = self.read_mmu(self.sp.wrapping_add(1)) as u16;
        self.sp = self.sp.wrapping_add(2);
        (hi << 8) | lo
    }

    pub fn jr(&mut self, offset: u8) {
        let signed = (offset as i8) as i16;
        self.pc = (self.pc as i16).wrapping_add(signed) as u16;
    }
}
