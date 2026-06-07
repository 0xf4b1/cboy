// SPDX-License-Identifier: GPL-3.0-only
use crate::gameboy::Gameboy;

impl Gameboy {
    fn fetch(&mut self) -> u8 {
        let v = self.read_mmu(self.pc);
        self.pc = self.pc.wrapping_add(1);
        v
    }

    fn fetch16(&mut self) -> u16 {
        let lo = self.fetch() as u16;
        let hi = self.fetch() as u16;
        (hi << 8) | lo
    }

    fn check_interrupt(&mut self) {
        let ie = self.read_mmu(0xFFFF);
        let ifl = self.read_mmu(0xFF0F);
        for i in 0u8..5 {
            if ie >> i & 1 != 0 && ifl >> i & 1 != 0 {
                if self.halt { self.halt = false; }
                if !self.ime { return; }
                let new_ifl = ifl & !(1 << i);
                self.write_mmu(0xFF0F, new_ifl);
                self.ime = false;
                let pc = self.pc;
                self.write_mmu(self.sp.wrapping_sub(1), (pc >> 8) as u8);
                self.write_mmu(self.sp.wrapping_sub(2), pc as u8);
                self.sp = self.sp.wrapping_sub(2);
                self.pc = 0x40 + (i as u16) * 8;
                self.halt = false;
                return;
            }
        }
    }

    pub fn next_instruction(&mut self) -> u8 {
        self.check_interrupt();
        if self.halt { return 4; }

        let opcode = self.fetch();

        if opcode == 0xCB {
            let cb_op = self.fetch();
            self.execute_cb(cb_op);
            return 8;
        }

        self.execute_opcode(opcode)
    }

    pub fn next_instructions(&mut self, mut cycles: i32) {
        while cycles > 0 {
            let c = self.next_instruction();
            self.timer_step(c);
            cycles -= c as i32;
        }
    }

    #[allow(clippy::too_many_lines)]
    fn execute_opcode(&mut self, op: u8) -> u8 {
        match op {
            // --- Row 0x00 ---
            0x00 => 4, // NOP
            0x01 => { let v = self.fetch16(); self.set_bc(v); 12 }
            0x02 => { let bc = self.bc(); self.write_mmu(bc, self.a); 8 }
            0x03 => { let v = self.bc().wrapping_add(1); self.set_bc(v); 8 }
            0x04 => { self.b = self.alu_inc(self.b); 4 }
            0x05 => { self.b = self.alu_dec(self.b); 4 }
            0x06 => { self.b = self.fetch(); 8 }
            0x07 => { // RLCA
                let c = self.a >> 7 & 1 != 0;
                self.a = (self.a << 1) | (c as u8);
                self.set_flag_z(false); self.set_flag_n(false); self.set_flag_h(false); self.set_flag_c(c);
                4
            }
            0x08 => { let addr = self.fetch16(); self.write_mmu(addr, self.sp as u8); self.write_mmu(addr+1, (self.sp>>8) as u8); 20 }
            0x09 => { let v = self.alu_add_hl(self.hl(), self.bc()); self.set_hl(v); 8 }
            0x0A => { let bc = self.bc(); self.a = self.read_mmu(bc); 8 }
            0x0B => { let v = self.bc().wrapping_sub(1); self.set_bc(v); 8 }
            0x0C => { self.c = self.alu_inc(self.c); 4 }
            0x0D => { self.c = self.alu_dec(self.c); 4 }
            0x0E => { self.c = self.fetch(); 8 }
            0x0F => { // RRCA
                let c = self.a & 1 != 0;
                self.a = (self.a >> 1) | ((c as u8) << 7);
                self.set_flag_z(false); self.set_flag_n(false); self.set_flag_h(false); self.set_flag_c(c);
                4
            }
            // --- Row 0x10 ---
            0x10 => 4, // STOP (treat as NOP)
            0x11 => { let v = self.fetch16(); self.set_de(v); 12 }
            0x12 => { let de = self.de(); self.write_mmu(de, self.a); 8 }
            0x13 => { let v = self.de().wrapping_add(1); self.set_de(v); 8 }
            0x14 => { self.d = self.alu_inc(self.d); 4 }
            0x15 => { self.d = self.alu_dec(self.d); 4 }
            0x16 => { self.d = self.fetch(); 8 }
            0x17 => { // RLA
                let c = self.a >> 7 & 1 != 0;
                self.a = (self.a << 1) | (self.flag_c() as u8);
                self.set_flag_z(false); self.set_flag_n(false); self.set_flag_h(false); self.set_flag_c(c);
                4
            }
            0x18 => { let v = self.fetch(); self.jr(v); 12 }
            0x19 => { let v = self.alu_add_hl(self.hl(), self.de()); self.set_hl(v); 8 }
            0x1A => { let de = self.de(); self.a = self.read_mmu(de); 8 }
            0x1B => { let v = self.de().wrapping_sub(1); self.set_de(v); 8 }
            0x1C => { self.e = self.alu_inc(self.e); 4 }
            0x1D => { self.e = self.alu_dec(self.e); 4 }
            0x1E => { self.e = self.fetch(); 8 }
            0x1F => { // RRA
                let c = self.a & 1 != 0;
                self.a = (self.a >> 1) | ((self.flag_c() as u8) << 7);
                self.set_flag_z(false); self.set_flag_n(false); self.set_flag_h(false); self.set_flag_c(c);
                4
            }
            // --- Row 0x20 ---
            0x20 => { let v = self.fetch(); if !self.flag_z() { self.jr(v); 12 } else { 8 } }
            0x21 => { let v = self.fetch16(); self.set_hl(v); 12 }
            0x22 => { let hl = self.hl(); self.write_mmu(hl, self.a); self.set_hl(hl.wrapping_add(1)); 8 }
            0x23 => { let v = self.hl().wrapping_add(1); self.set_hl(v); 8 }
            0x24 => { self.h = self.alu_inc(self.h); 4 }
            0x25 => { self.h = self.alu_dec(self.h); 4 }
            0x26 => { self.h = self.fetch(); 8 }
            0x27 => { // DAA
                let mut t = self.a;
                let mut corr: u8 = 0;
                if self.flag_h() { corr |= 0x06; }
                if self.flag_c() { corr |= 0x60; }
                if self.flag_n() {
                    t = t.wrapping_sub(corr);
                } else {
                    if t & 0x0F > 0x09 { corr |= 0x06; }
                    if t > 0x99 { corr |= 0x60; }
                    t = t.wrapping_add(corr);
                }
                self.set_flag_z(t == 0);
                self.set_flag_h(false);
                self.set_flag_c(corr & 0x60 != 0);
                self.a = t;
                4
            }
            0x28 => { let v = self.fetch(); if self.flag_z() { self.jr(v); 12 } else { 8 } }
            0x29 => { let v = self.alu_add_hl(self.hl(), self.hl()); self.set_hl(v); 8 }
            0x2A => { let hl = self.hl(); self.a = self.read_mmu(hl); self.set_hl(hl.wrapping_add(1)); 8 }
            0x2B => { let v = self.hl().wrapping_sub(1); self.set_hl(v); 8 }
            0x2C => { self.l = self.alu_inc(self.l); 4 }
            0x2D => { self.l = self.alu_dec(self.l); 4 }
            0x2E => { self.l = self.fetch(); 8 }
            0x2F => { self.a = !self.a; self.set_flag_n(true); self.set_flag_h(true); 4 } // CPL
            // --- Row 0x30 ---
            0x30 => { let v = self.fetch(); if !self.flag_c() { self.jr(v); 12 } else { 8 } }
            0x31 => { let v = self.fetch16(); self.sp = v; 12 }
            0x32 => { let hl = self.hl(); self.write_mmu(hl, self.a); self.set_hl(hl.wrapping_sub(1)); 8 }
            0x33 => { self.sp = self.sp.wrapping_add(1); 8 }
            0x34 => { let hl = self.hl(); let v = self.read_mmu(hl); let r = self.alu_inc(v); self.write_mmu(hl, r); 12 }
            0x35 => { let hl = self.hl(); let v = self.read_mmu(hl); let r = self.alu_dec(v); self.write_mmu(hl, r); 12 }
            0x36 => { let v = self.fetch(); let hl = self.hl(); self.write_mmu(hl, v); 12 }
            0x37 => { self.set_flag_n(false); self.set_flag_h(false); self.set_flag_c(true); 4 } // SCF
            0x38 => { let v = self.fetch(); if self.flag_c() { self.jr(v); 12 } else { 8 } }
            0x39 => { let v = self.alu_add_hl(self.hl(), self.sp); self.set_hl(v); 8 }
            0x3A => { let hl = self.hl(); self.a = self.read_mmu(hl); self.set_hl(hl.wrapping_sub(1)); 8 }
            0x3B => { self.sp = self.sp.wrapping_sub(1); 8 }
            0x3C => { self.a = self.alu_inc(self.a); 4 }
            0x3D => { self.a = self.alu_dec(self.a); 4 }
            0x3E => { self.a = self.fetch(); 8 }
            0x3F => { let c = self.flag_c(); self.set_flag_c(!c); self.set_flag_n(false); self.set_flag_h(false); 4 } // CCF
            // --- Row 0x40-0x7F: LD r,r and HALT ---
            0x40 => 4, // LD B,B
            0x41 => { self.b = self.c; 4 }
            0x42 => { self.b = self.d; 4 }
            0x43 => { self.b = self.e; 4 }
            0x44 => { self.b = self.h; 4 }
            0x45 => { self.b = self.l; 4 }
            0x46 => { let hl = self.hl(); self.b = self.read_mmu(hl); 8 }
            0x47 => { self.b = self.a; 4 }
            0x48 => { self.c = self.b; 4 }
            0x49 => 4, // LD C,C
            0x4A => { self.c = self.d; 4 }
            0x4B => { self.c = self.e; 4 }
            0x4C => { self.c = self.h; 4 }
            0x4D => { self.c = self.l; 4 }
            0x4E => { let hl = self.hl(); self.c = self.read_mmu(hl); 8 }
            0x4F => { self.c = self.a; 4 }
            0x50 => { self.d = self.b; 4 }
            0x51 => { self.d = self.c; 4 }
            0x52 => 4, // LD D,D
            0x53 => { self.d = self.e; 4 }
            0x54 => { self.d = self.h; 4 }
            0x55 => { self.d = self.l; 4 }
            0x56 => { let hl = self.hl(); self.d = self.read_mmu(hl); 8 }
            0x57 => { self.d = self.a; 4 }
            0x58 => { self.e = self.b; 4 }
            0x59 => { self.e = self.c; 4 }
            0x5A => { self.e = self.d; 4 }
            0x5B => 4, // LD E,E
            0x5C => { self.e = self.h; 4 }
            0x5D => { self.e = self.l; 4 }
            0x5E => { let hl = self.hl(); self.e = self.read_mmu(hl); 8 }
            0x5F => { self.e = self.a; 4 }
            0x60 => { self.h = self.b; 4 }
            0x61 => { self.h = self.c; 4 }
            0x62 => { self.h = self.d; 4 }
            0x63 => { self.h = self.e; 4 }
            0x64 => 4, // LD H,H
            0x65 => { self.h = self.l; 4 }
            0x66 => { let hl = self.hl(); self.h = self.read_mmu(hl); 8 }
            0x67 => { self.h = self.a; 4 }
            0x68 => { self.l = self.b; 4 }
            0x69 => { self.l = self.c; 4 }
            0x6A => { self.l = self.d; 4 }
            0x6B => { self.l = self.e; 4 }
            0x6C => { self.l = self.h; 4 }
            0x6D => 4, // LD L,L
            0x6E => { let hl = self.hl(); self.l = self.read_mmu(hl); 8 }
            0x6F => { self.l = self.a; 4 }
            0x70 => { let hl = self.hl(); self.write_mmu(hl, self.b); 8 }
            0x71 => { let hl = self.hl(); self.write_mmu(hl, self.c); 8 }
            0x72 => { let hl = self.hl(); self.write_mmu(hl, self.d); 8 }
            0x73 => { let hl = self.hl(); self.write_mmu(hl, self.e); 8 }
            0x74 => { let hl = self.hl(); self.write_mmu(hl, self.h); 8 }
            0x75 => { let hl = self.hl(); self.write_mmu(hl, self.l); 8 }
            0x76 => { self.halt = true; 4 } // HALT
            0x77 => { let hl = self.hl(); self.write_mmu(hl, self.a); 8 }
            0x78 => { self.a = self.b; 4 }
            0x79 => { self.a = self.c; 4 }
            0x7A => { self.a = self.d; 4 }
            0x7B => { self.a = self.e; 4 }
            0x7C => { self.a = self.h; 4 }
            0x7D => { self.a = self.l; 4 }
            0x7E => { let hl = self.hl(); self.a = self.read_mmu(hl); 8 }
            0x7F => 4, // LD A,A
            // --- Row 0x80-0xBF: ALU ops ---
            0x80 => { let v = self.b; self.a = self.alu_add(self.a, v); 4 }
            0x81 => { let v = self.c; self.a = self.alu_add(self.a, v); 4 }
            0x82 => { let v = self.d; self.a = self.alu_add(self.a, v); 4 }
            0x83 => { let v = self.e; self.a = self.alu_add(self.a, v); 4 }
            0x84 => { let v = self.h; self.a = self.alu_add(self.a, v); 4 }
            0x85 => { let v = self.l; self.a = self.alu_add(self.a, v); 4 }
            0x86 => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_add(self.a, v); 8 }
            0x87 => { let v = self.a; self.a = self.alu_add(self.a, v); 4 }
            0x88 => { let v = self.b; self.a = self.alu_adc(self.a, v); 4 }
            0x89 => { let v = self.c; self.a = self.alu_adc(self.a, v); 4 }
            0x8A => { let v = self.d; self.a = self.alu_adc(self.a, v); 4 }
            0x8B => { let v = self.e; self.a = self.alu_adc(self.a, v); 4 }
            0x8C => { let v = self.h; self.a = self.alu_adc(self.a, v); 4 }
            0x8D => { let v = self.l; self.a = self.alu_adc(self.a, v); 4 }
            0x8E => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_adc(self.a, v); 8 }
            0x8F => { let v = self.a; self.a = self.alu_adc(self.a, v); 4 }
            0x90 => { let v = self.b; self.a = self.alu_sub(self.a, v); 4 }
            0x91 => { let v = self.c; self.a = self.alu_sub(self.a, v); 4 }
            0x92 => { let v = self.d; self.a = self.alu_sub(self.a, v); 4 }
            0x93 => { let v = self.e; self.a = self.alu_sub(self.a, v); 4 }
            0x94 => { let v = self.h; self.a = self.alu_sub(self.a, v); 4 }
            0x95 => { let v = self.l; self.a = self.alu_sub(self.a, v); 4 }
            0x96 => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_sub(self.a, v); 8 }
            0x97 => { let v = self.a; self.a = self.alu_sub(self.a, v); 4 }
            0x98 => { let v = self.b; self.a = self.alu_sbc(self.a, v); 4 }
            0x99 => { let v = self.c; self.a = self.alu_sbc(self.a, v); 4 }
            0x9A => { let v = self.d; self.a = self.alu_sbc(self.a, v); 4 }
            0x9B => { let v = self.e; self.a = self.alu_sbc(self.a, v); 4 }
            0x9C => { let v = self.h; self.a = self.alu_sbc(self.a, v); 4 }
            0x9D => { let v = self.l; self.a = self.alu_sbc(self.a, v); 4 }
            0x9E => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_sbc(self.a, v); 8 }
            0x9F => { let v = self.a; self.a = self.alu_sbc(self.a, v); 4 }
            0xA0 => { let v = self.b; self.a = self.alu_and(self.a, v); 4 }
            0xA1 => { let v = self.c; self.a = self.alu_and(self.a, v); 4 }
            0xA2 => { let v = self.d; self.a = self.alu_and(self.a, v); 4 }
            0xA3 => { let v = self.e; self.a = self.alu_and(self.a, v); 4 }
            0xA4 => { let v = self.h; self.a = self.alu_and(self.a, v); 4 }
            0xA5 => { let v = self.l; self.a = self.alu_and(self.a, v); 4 }
            0xA6 => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_and(self.a, v); 8 }
            0xA7 => { let v = self.a; self.a = self.alu_and(self.a, v); 4 }
            0xA8 => { let v = self.b; self.a = self.alu_xor(self.a, v); 4 }
            0xA9 => { let v = self.c; self.a = self.alu_xor(self.a, v); 4 }
            0xAA => { let v = self.d; self.a = self.alu_xor(self.a, v); 4 }
            0xAB => { let v = self.e; self.a = self.alu_xor(self.a, v); 4 }
            0xAC => { let v = self.h; self.a = self.alu_xor(self.a, v); 4 }
            0xAD => { let v = self.l; self.a = self.alu_xor(self.a, v); 4 }
            0xAE => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_xor(self.a, v); 8 }
            0xAF => { let v = self.a; self.a = self.alu_xor(self.a, v); 4 }
            0xB0 => { let v = self.b; self.a = self.alu_or(self.a, v); 4 }
            0xB1 => { let v = self.c; self.a = self.alu_or(self.a, v); 4 }
            0xB2 => { let v = self.d; self.a = self.alu_or(self.a, v); 4 }
            0xB3 => { let v = self.e; self.a = self.alu_or(self.a, v); 4 }
            0xB4 => { let v = self.h; self.a = self.alu_or(self.a, v); 4 }
            0xB5 => { let v = self.l; self.a = self.alu_or(self.a, v); 4 }
            0xB6 => { let hl = self.hl(); let v = self.read_mmu(hl); self.a = self.alu_or(self.a, v); 8 }
            0xB7 => { let v = self.a; self.a = self.alu_or(self.a, v); 4 }
            0xB8 => { let v = self.b; self.alu_cp(self.a, v); 4 }
            0xB9 => { let v = self.c; self.alu_cp(self.a, v); 4 }
            0xBA => { let v = self.d; self.alu_cp(self.a, v); 4 }
            0xBB => { let v = self.e; self.alu_cp(self.a, v); 4 }
            0xBC => { let v = self.h; self.alu_cp(self.a, v); 4 }
            0xBD => { let v = self.l; self.alu_cp(self.a, v); 4 }
            0xBE => { let hl = self.hl(); let v = self.read_mmu(hl); self.alu_cp(self.a, v); 8 }
            0xBF => { let v = self.a; self.alu_cp(self.a, v); 4 }
            // --- Row 0xC0-0xFF: Control flow, stack, misc ---
            0xC0 => { if !self.flag_z() { self.pc = self.stack_pop(); 20 } else { 8 } } // RET NZ
            0xC1 => { let v = self.stack_pop(); self.set_bc(v); 12 } // POP BC
            0xC2 => { let a = self.fetch16(); if !self.flag_z() { self.pc = a; 16 } else { 12 } } // JP NZ,a16
            0xC3 => { let a = self.fetch16(); self.pc = a; 16 } // JP a16
            0xC4 => { let a = self.fetch16(); if !self.flag_z() { self.stack_push(self.pc); self.pc = a; 24 } else { 12 } } // CALL NZ,a16
            0xC5 => { let v = self.bc(); self.stack_push(v); 16 } // PUSH BC
            0xC6 => { let v = self.fetch(); self.a = self.alu_add(self.a, v); 8 } // ADD A,d8
            0xC7 => { self.stack_push(self.pc); self.pc = 0x00; 16 } // RST 00
            0xC8 => { if self.flag_z() { self.pc = self.stack_pop(); 20 } else { 8 } } // RET Z
            0xC9 => { self.pc = self.stack_pop(); 16 } // RET
            0xCA => { let a = self.fetch16(); if self.flag_z() { self.pc = a; 16 } else { 12 } } // JP Z,a16
            0xCB => unreachable!(), // handled above
            0xCC => { let a = self.fetch16(); if self.flag_z() { self.stack_push(self.pc); self.pc = a; 24 } else { 12 } } // CALL Z,a16
            0xCD => { let a = self.fetch16(); self.stack_push(self.pc); self.pc = a; 24 } // CALL a16
            0xCE => { let v = self.fetch(); self.a = self.alu_adc(self.a, v); 8 } // ADC A,d8
            0xCF => { self.stack_push(self.pc); self.pc = 0x08; 16 } // RST 08
            0xD0 => { if !self.flag_c() { self.pc = self.stack_pop(); 20 } else { 8 } } // RET NC
            0xD1 => { let v = self.stack_pop(); self.set_de(v); 12 } // POP DE
            0xD2 => { let a = self.fetch16(); if !self.flag_c() { self.pc = a; 16 } else { 12 } } // JP NC,a16
            0xD3 => 4, // NOP (illegal)
            0xD4 => { let a = self.fetch16(); if !self.flag_c() { self.stack_push(self.pc); self.pc = a; 24 } else { 12 } } // CALL NC,a16
            0xD5 => { let v = self.de(); self.stack_push(v); 16 } // PUSH DE
            0xD6 => { let v = self.fetch(); self.a = self.alu_sub(self.a, v); 8 } // SUB d8
            0xD7 => { self.stack_push(self.pc); self.pc = 0x10; 16 } // RST 10
            0xD8 => { if self.flag_c() { self.pc = self.stack_pop(); 20 } else { 8 } } // RET C
            0xD9 => { self.pc = self.stack_pop(); self.ime = true; 16 } // RETI
            0xDA => { let a = self.fetch16(); if self.flag_c() { self.pc = a; 16 } else { 12 } } // JP C,a16
            0xDB => 4, // NOP (illegal)
            0xDC => { let a = self.fetch16(); if self.flag_c() { self.stack_push(self.pc); self.pc = a; 24 } else { 12 } } // CALL C,a16
            0xDD => 4, // NOP (illegal)
            0xDE => { let v = self.fetch(); self.a = self.alu_sbc(self.a, v); 8 } // SBC A,d8
            0xDF => { self.stack_push(self.pc); self.pc = 0x18; 16 } // RST 18
            0xE0 => { let n = self.fetch(); self.write_mmu(0xFF00 | n as u16, self.a); 12 } // LDH (n),A
            0xE1 => { let v = self.stack_pop(); self.set_hl(v); 12 } // POP HL
            0xE2 => { let c = self.c; self.write_mmu(0xFF00 | c as u16, self.a); 8 } // LD (C),A
            0xE3 => 4, // NOP (illegal)
            0xE4 => 4, // NOP (illegal)
            0xE5 => { let v = self.hl(); self.stack_push(v); 16 } // PUSH HL
            0xE6 => { let v = self.fetch(); self.a = self.alu_and(self.a, v); 8 } // AND d8
            0xE7 => { self.stack_push(self.pc); self.pc = 0x20; 16 } // RST 20
            0xE8 => { // ADD SP,r8
                let n = self.fetch() as i8 as i16;
                let sp = self.sp;
                let res = (sp as i16).wrapping_add(n) as u16;
                self.set_flag_z(false);
                self.set_flag_n(false);
                self.set_flag_h((sp & 0xF) + (n as u16 & 0xF) > 0xF);
                self.set_flag_c((sp & 0xFF) + (n as u16 & 0xFF) > 0xFF);
                self.sp = res;
                16
            }
            0xE9 => { self.pc = self.hl(); 4 } // JP (HL)
            0xEA => { let a = self.fetch16(); self.write_mmu(a, self.a); 16 } // LD (a16),A
            0xEB => 4, // NOP (illegal)
            0xEC => 4, // NOP (illegal)
            0xED => 4, // NOP (illegal)
            0xEE => { let v = self.fetch(); self.a = self.alu_xor(self.a, v); 8 } // XOR d8
            0xEF => { self.stack_push(self.pc); self.pc = 0x28; 16 } // RST 28
            0xF0 => { let n = self.fetch(); self.a = self.read_mmu(0xFF00 | n as u16); 12 } // LDH A,(n)
            0xF1 => { let v = self.stack_pop(); self.set_af(v); 12 } // POP AF
            0xF2 => { let c = self.c; self.a = self.read_mmu(0xFF00 | c as u16); 8 } // LD A,(C)
            0xF3 => { self.ime = false; 4 } // DI
            0xF4 => 4, // NOP (illegal)
            0xF5 => { let v = self.af(); self.stack_push(v); 16 } // PUSH AF
            0xF6 => { let v = self.fetch(); self.a = self.alu_or(self.a, v); 8 } // OR d8
            0xF7 => { self.stack_push(self.pc); self.pc = 0x30; 16 } // RST 30
            0xF8 => { // LD HL,SP+r8
                let n = self.fetch() as i8 as i16;
                let sp = self.sp;
                let res = (sp as i16).wrapping_add(n) as u16;
                self.set_flag_z(false);
                self.set_flag_n(false);
                self.set_flag_h((sp & 0xF) + (n as u16 & 0xF) > 0xF);
                self.set_flag_c((sp & 0xFF) + (n as u16 & 0xFF) > 0xFF);
                self.set_hl(res);
                12
            }
            0xF9 => { self.sp = self.hl(); 8 } // LD SP,HL
            0xFA => { let a = self.fetch16(); self.a = self.read_mmu(a); 16 } // LD A,(a16)
            0xFB => { self.ime = true; 4 } // EI
            0xFC => 4, // NOP (illegal)
            0xFD => 4, // NOP (illegal)
            0xFE => { let v = self.fetch(); self.alu_cp(self.a, v); 8 } // CP d8
            0xFF => { self.stack_push(self.pc); self.pc = 0x38; 16 } // RST 38
        }
    }
}
