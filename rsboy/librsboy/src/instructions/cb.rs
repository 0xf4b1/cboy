// SPDX-License-Identifier: GPL-3.0-only

use crate::gameboy::Gameboy;

impl Gameboy {
    fn cb_rlc(&mut self, v: u8) -> u8 {
        let c = v >> 7 & 1 != 0;
        let res = (v << 1) | (c as u8);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_rrc(&mut self, v: u8) -> u8 {
        let c = v & 1 != 0;
        let res = (v >> 1) | ((c as u8) << 7);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_rl(&mut self, v: u8) -> u8 {
        let c = v >> 7 & 1 != 0;
        let res = (v << 1) | (self.flag_c() as u8);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_rr(&mut self, v: u8) -> u8 {
        let c = v & 1 != 0;
        let res = (v >> 1) | ((self.flag_c() as u8) << 7);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_sla(&mut self, v: u8) -> u8 {
        let c = v >> 7 & 1 != 0;
        let res = v << 1;
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_sra(&mut self, v: u8) -> u8 {
        let c = v & 1 != 0;
        let res = (v >> 1) | (v & 0x80);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_swap(&mut self, v: u8) -> u8 {
        let res = (v << 4) | (v >> 4);
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(false);
        res
    }

    fn cb_srl(&mut self, v: u8) -> u8 {
        let c = v & 1 != 0;
        let res = v >> 1;
        self.set_flag_z(res == 0);
        self.set_flag_n(false);
        self.set_flag_h(false);
        self.set_flag_c(c);
        res
    }

    fn cb_bit(&mut self, v: u8, bit: u8) {
        self.set_flag_z(v >> bit & 1 == 0);
        self.set_flag_n(false);
        self.set_flag_h(true);
    }

    fn cb_res(v: u8, bit: u8) -> u8 { v & !(1 << bit) }
    fn cb_set(v: u8, bit: u8) -> u8 { v | (1 << bit) }

    pub fn execute_cb(&mut self, opcode: u8) {
        let reg = opcode & 0x07;
        let op  = opcode >> 3;

        let val = self.cb_get_reg(reg);

        if op < 8 {
            // rotate/shift/swap
            let result = match op {
                0 => self.cb_rlc(val),
                1 => self.cb_rrc(val),
                2 => self.cb_rl(val),
                3 => self.cb_rr(val),
                4 => self.cb_sla(val),
                5 => self.cb_sra(val),
                6 => self.cb_swap(val),
                7 => self.cb_srl(val),
                _ => unreachable!(),
            };
            self.cb_set_reg(reg, result);
        } else {
            let bit = (op - 8) % 8;
            match (op - 8) / 8 {
                0 => { self.cb_bit(val, bit); } // BIT - no write back
                1 => { let r = Self::cb_res(val, bit); self.cb_set_reg(reg, r); }
                2 => { let r = Self::cb_set(val, bit); self.cb_set_reg(reg, r); }
                _ => unreachable!(),
            }
        }
    }

    fn cb_get_reg(&self, reg: u8) -> u8 {
        match reg {
            0 => self.b,
            1 => self.c,
            2 => self.d,
            3 => self.e,
            4 => self.h,
            5 => self.l,
            6 => self.read_mmu(self.hl()),
            7 => self.a,
            _ => unreachable!(),
        }
    }

    fn cb_set_reg(&mut self, reg: u8, val: u8) {
        match reg {
            0 => self.b = val,
            1 => self.c = val,
            2 => self.d = val,
            3 => self.e = val,
            4 => self.h = val,
            5 => self.l = val,
            6 => { let hl = self.hl(); self.write_mmu(hl, val); }
            7 => self.a = val,
            _ => unreachable!(),
        }
    }
}
