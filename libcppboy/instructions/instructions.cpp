// SPDX-License-Identifier: GPL-3.0-only

#include "instructions.hpp"
#include "../cpu.hpp"
#include "../mmu.hpp"

namespace cboy::instructions {

uint8_t execute(uint8_t opcode, CPU& cpu, MMU& mmu) {
    auto push = [&](uint16_t value) {
        mmu.write(cpu.stack_pointer - 1, value >> 8);
        mmu.write(cpu.stack_pointer - 2, value & 0xFF);
        cpu.stack_pointer -= 2;
    };

    auto pop = [&]() -> uint16_t {
        uint16_t value = (mmu.read(cpu.stack_pointer + 1) << 8 | mmu.read(cpu.stack_pointer)) & 0xFFFF;
        cpu.stack_pointer += 2;
        return value;
    };

    auto jr = [&](uint8_t value) {
        cpu.program_counter += (value ^ 0x80) - 0x80;
    };

    auto add = [&](uint8_t a, uint8_t b) {
        uint8_t res = (a + b) & 0xFF;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(false);
        cpu.set_flag_H((a & 0xF) + (b & 0xF) > 0xF);
        cpu.set_flag_C(a + b > 0xFF);
        return res;
    };

    auto add_hl = [&](uint16_t a, uint16_t b) {
        uint16_t res = (a + b) & 0xFFFF;
        cpu.set_flag_N(false);
        cpu.set_flag_H((a & 0xFFF) + (b & 0xFFF) > 0xFFF);
        cpu.set_flag_C(a + b > 0xFFFF);
        return res;
    };

    auto adc = [&](uint8_t a, uint8_t b) {
        uint8_t res = (a + b + cpu.flag_C()) & 0xFF;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(false);
        cpu.set_flag_H((a & 0xF) + (b & 0xF) + cpu.flag_C() > 0xF);
        cpu.set_flag_C(a + b + cpu.flag_C() > 0xFF);
        return res;
    };

    auto sub = [&](uint8_t a, uint8_t b) {
        uint8_t res = (a - b) & 0xFF;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(true);
        cpu.set_flag_H((a & 0xF) < (b & 0xF));
        cpu.set_flag_C(a < b);
        return res;
    };

    auto sbc = [&](uint8_t a, uint8_t b) {
        uint8_t res = (a - b - cpu.flag_C()) & 0xFF;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(true);
        cpu.set_flag_H((a & 0xF) < (b & 0xF) + cpu.flag_C());
        cpu.set_flag_C(a < b + cpu.flag_C());
        return res;
    };

    auto and_ = [&](uint8_t a, uint8_t b) {
        uint8_t res = a & b;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(false);
        cpu.set_flag_H(true);
        cpu.set_flag_C(false);
        return res;
    };

    auto or_ = [&](uint8_t a, uint8_t b) {
        uint8_t res = a | b;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(false);
        cpu.set_flag_H(false);
        cpu.set_flag_C(false);
        return res;
    };

    auto xor_ = [&](uint8_t a, uint8_t b) {
        uint8_t res = a ^ b;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(false);
        cpu.set_flag_H(false);
        cpu.set_flag_C(false);
        return res;
    };

    auto cp = [&](uint8_t a, uint8_t b) {
        uint8_t res = a - b;
        cpu.set_flag_Z(res == 0);
        cpu.set_flag_N(true);
        cpu.set_flag_H((a & 0xF) < (b & 0xF));
        cpu.set_flag_C(a < b);
        return a;
    };

    auto inc = [&](uint8_t reg) {
        cpu.set_flag_H(((reg & 0xF) + 1) & 0x10);
        reg = (reg + 1) & 0xFF;
        cpu.set_flag_Z(reg == 0);
        cpu.set_flag_N(false);
        return reg;
    };

    auto dec = [&](uint8_t reg) {
        cpu.set_flag_H((reg & 0xF) - 1 < 0);
        reg = (reg - 1) & 0xFF;
        cpu.set_flag_Z(reg == 0);
        cpu.set_flag_N(true);
        return reg;
    };
    switch (opcode) {
        // 0x00
        case 0x00: return 4; // NOP
        
        case 0x01: { // LD BC,d16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            cpu.set_BC(static_cast<uint16_t>((high << 8) | low));
            return 12;
        }
        case 0x02: { // LD (BC),A
            mmu.write(cpu.BC(), cpu.registers.A);
            return 8;
        }
        case 0x03: { // INC BC
            cpu.set_BC(cpu.BC() + 1);
            return 8;
        }
        case 0x04: { // INC B
            cpu.registers.B = inc(cpu.registers.B);
            return 4;
        }
        case 0x05: { // DEC B
            cpu.registers.B = dec(cpu.registers.B);
            return 4;
        }
        case 0x06: { // LD B,d8
            cpu.registers.B = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x07: { // RLCA
            bool c = cpu.registers.A >> 7 & 1;
            cpu.registers.A = (cpu.registers.A << 1 | c) & 0xFF;
            cpu.set_flag_Z(false);
            cpu.set_flag_N(false);
            cpu.set_flag_H(false);
            cpu.set_flag_C(c);
            return 4;
        }
        case 0x08: { // LD (a16),SP
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            mmu.write(addr, cpu.stack_pointer & 0xFF);
            mmu.write(addr + 1, cpu.stack_pointer >> 8 & 0xFF);
            return 20;
        }
        case 0x09: { // ADD HL,BC
            cpu.set_HL(add_hl(cpu.HL(), cpu.BC()));
            return 8;
        }
        case 0x0A: { // LD A,(BC)
            cpu.registers.A = mmu.read(cpu.BC());
            return 8;
        }
        case 0x0B: { // DEC BC
            cpu.set_BC(cpu.BC() - 1);
            return 8;
        }
        case 0x0C: { // INC C
            cpu.registers.C = inc(cpu.registers.C);
            return 4;
        }
        case 0x0D: { // DEC C
            cpu.registers.C = dec(cpu.registers.C);
            return 4;
        }
        case 0x0E: { // LD C,d8
            cpu.registers.C = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x0F: { // RRCA
            bool c = cpu.registers.A & 1;
            cpu.registers.A = ((cpu.registers.A >> 1) | c << 7) & 0xFF;
            cpu.set_flag_Z(false);
            cpu.set_flag_N(false);
            cpu.set_flag_H(false);
            cpu.set_flag_C(c);
            return 4;
        }
        
        // 0x10
        case 0x10: { // STOP
            mmu.read(cpu.program_counter++); // Consume next byte
            return 4;
        }
        case 0x11: { // LD DE,d16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            cpu.set_DE(static_cast<uint16_t>((high << 8) | low));
            return 12;
        }
        case 0x12: { // LD (DE),A
            mmu.write(cpu.DE(), cpu.registers.A);
            return 8;
        }
        case 0x13: { // INC DE
            cpu.set_DE(cpu.DE() + 1);
            return 8;
        }
        case 0x14: { // INC D
            cpu.registers.D = inc(cpu.registers.D);
            return 4;
        }
        case 0x15: { // DEC D
            cpu.registers.D = dec(cpu.registers.D);
            return 4;
        }
        case 0x16: { // LD D,d8
            cpu.registers.D = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x17: { // RLA
            bool c = (cpu.registers.A >> 7) & 1;
            cpu.registers.A = ((cpu.registers.A << 1) | cpu.flag_C()) & 0xFF;
            cpu.set_flag_Z(false);
            cpu.set_flag_N(false);
            cpu.set_flag_H(false);
            cpu.set_flag_C(c);
            return 4;
        }
        case 0x18: { // JR r8
            uint8_t val = mmu.read(cpu.program_counter++);
            jr(val);
            return 12;
        }
        case 0x19: { // ADD HL,DE
            cpu.set_HL(add_hl(cpu.HL(), cpu.DE()));
            return 8;
        }
        case 0x1A: { // LD A,(DE)
            cpu.registers.A = mmu.read(cpu.DE());
            return 8;
        }
        case 0x1B: { // DEC DE
            cpu.set_DE(cpu.DE() - 1);
            return 8;
        }
        case 0x1C: { // INC E
            cpu.registers.E = inc(cpu.registers.E);
            return 4;
        }
        case 0x1D: { // DEC E
            cpu.registers.E = dec(cpu.registers.E);
            return 4;
        }
        case 0x1E: { // LD E,d8
            cpu.registers.E = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x1F: { // RRA
            uint8_t c = cpu.registers.A & 1;
            cpu.registers.A = ((cpu.registers.A >> 1) | (cpu.flag_C() << 7)) & 0xFF;
            cpu.set_flag_Z(false);
            cpu.set_flag_N(false);
            cpu.set_flag_H(false);
            cpu.set_flag_C(c);
            return 4;
        }
        
        // 0x20
        case 0x20: { // JR NZ,r8
            uint8_t val = mmu.read(cpu.program_counter++);
            if (!cpu.flag_Z()) {
                jr(val);
                return 12;
            }
            return 8;
        }
        case 0x21: { // LD HL,d16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            cpu.set_HL(static_cast<uint16_t>((high << 8) | low));
            return 12;
        }
        case 0x22: { // LDI (HL),A
            mmu.write(cpu.HL(), cpu.registers.A);
            cpu.set_HL((cpu.HL() + 1) & 0xFFFF);
            return 8;
        }
        case 0x23: { // INC HL
            cpu.set_HL(cpu.HL() + 1);
            return 8;
        }
        case 0x24: { // INC H
            cpu.registers.H = inc(cpu.registers.H);
            return 4;
        }
        case 0x25: { // DEC H
            cpu.registers.H = dec(cpu.registers.H);
            return 4;
        }
        case 0x26: { // LD H,d8
            cpu.registers.H = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x27: { // DAA
            uint8_t t = cpu.registers.A;
            uint8_t corr = 0;
            if (cpu.flag_H())
                corr |= 0x06;
            if (cpu.flag_C())
                corr |= 0x60;
            if (cpu.flag_N())
                t -= corr;
            else {
                if ((t & 0x0F) > 0x09)
                    corr |= 0x06;
                if (t > 0x99)
                    corr |= 0x60;
                t += corr;
            }
            cpu.set_flag_Z((t & 0xFF) == 0);
            cpu.set_flag_H(false);
            cpu.set_flag_C((corr & 0x60) != 0);
            cpu.registers.A = t & 0xFF;
            return 4;
        }
        case 0x28: { // JR Z,r8
            uint8_t val = mmu.read(cpu.program_counter++);
            if (cpu.flag_Z()) {
                jr(val);
                return 12;
            }
            return 8;
        }
        case 0x29: { // ADD HL,HL
            cpu.set_HL(add_hl(cpu.HL(), cpu.HL()));
            return 8;
        }
        case 0x2A: { // LDI A,(HL)
            cpu.registers.A = mmu.read(cpu.HL());
            cpu.set_HL((cpu.HL() + 1) & 0xFFFF);
            return 8;
        }
        case 0x2B: { // DEC HL
            cpu.set_HL(cpu.HL() - 1);
            return 8;
        }
        case 0x2C: { // INC L
            cpu.registers.L = inc(cpu.registers.L);
            return 4;
        }
        case 0x2D: { // DEC L
            cpu.registers.L = dec(cpu.registers.L);
            return 4;
        }
        case 0x2E: { // LD L,d8
            cpu.registers.L = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x2F: { // CPL
            cpu.registers.A = ~cpu.registers.A & 0xFF;
            cpu.set_flag_N(true);
            cpu.set_flag_H(true);
            return 4;
        }
        
        // 0x30
        case 0x30: { // JR NC,r8
            uint8_t val = mmu.read(cpu.program_counter++);
            if (!cpu.flag_C()) {
                jr(val);
                return 12;
            }
            return 8;
        }
        case 0x31: { // LD SP,d16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            cpu.stack_pointer = static_cast<uint16_t>((high << 8) | low);
            return 12;
        }
        case 0x32: { // LDD (HL),A
            mmu.write(cpu.HL(), cpu.registers.A);
            cpu.set_HL(cpu.HL() - 1);
            return 8;
        }
        case 0x33: { // INC SP
            cpu.stack_pointer++;
            return 8;
        }
        case 0x34: { // INC (HL)
            mmu.write(cpu.HL(), inc(mmu.read(cpu.HL())));
            return 12;
        }
        case 0x35: { // DEC (HL)
            mmu.write(cpu.HL(), dec(mmu.read(cpu.HL())));
            return 12;
        }
        case 0x36: { // LD (HL),d8
            mmu.write(cpu.HL(), mmu.read(cpu.program_counter++));
            return 12;
        }
        case 0x37: { // SCF
            cpu.set_flag_C(true);
            cpu.set_flag_N(false);
            cpu.set_flag_H(false);
            return 4;
        }
        case 0x38: { // JR C,r8
            uint8_t val = mmu.read(cpu.program_counter++);
            if (cpu.flag_C()) {
                jr(val);
                return 12;
            }
            return 8;
        }
        case 0x39: { // ADD HL,SP
            cpu.set_HL(add_hl(cpu.HL(), cpu.stack_pointer));
            return 8;
        }
        case 0x3A: { // LDD A,(HL)
            cpu.registers.A = mmu.read(cpu.HL());
            cpu.set_HL((cpu.HL() - 1) & 0xFFFF);
            return 8;
        }
        case 0x3B: { // DEC SP
            cpu.stack_pointer--;
            return 8;
        }
        case 0x3C: { // INC A
            cpu.registers.A = inc(cpu.registers.A);
            return 4;
        }
        case 0x3D: { // DEC A
            cpu.registers.A = dec(cpu.registers.A);
            return 4;
        }
        case 0x3E: { // LD A,d8
            cpu.registers.A = mmu.read(cpu.program_counter++);
            return 8;
        }
        case 0x3F: { // CCF
            cpu.set_flag_C(!cpu.flag_C());
            cpu.set_flag_N(false);
            cpu.set_flag_H(false);
            return 4;
        }
        
        // 0x40
        case 0x40: { // LD B,B
            return 4;
        }
        case 0x41: { // LD B,C
            cpu.registers.B = cpu.registers.C;
            return 4;
        }
        case 0x42: { // LD B,D
            cpu.registers.B = cpu.registers.D;
            return 4;
        }
        case 0x43: { // LD B,E
            cpu.registers.B = cpu.registers.E;
            return 4;
        }
        case 0x44: { // LD B,H
            cpu.registers.B = cpu.registers.H;
            return 4;
        }
        case 0x45: { // LD B,L
            cpu.registers.B = cpu.registers.L;
            return 4;
        }
        case 0x46: { // LD B,(HL)
            cpu.registers.B = mmu.read(cpu.HL());
            return 8;
        }
        case 0x47: { // LD B,A
            cpu.registers.B = cpu.registers.A;
            return 4;
        }
        case 0x48: { // LD C,B
            cpu.registers.C = cpu.registers.B;
            return 4;
        }
        case 0x49: { // LD C,C
            return 4;
        }
        case 0x4A: { // LD C,D
            cpu.registers.C = cpu.registers.D;
            return 4;
        }
        case 0x4B: { // LD C,E
            cpu.registers.C = cpu.registers.E;
            return 4;
        }
        case 0x4C: { // LD C,H
            cpu.registers.C = cpu.registers.H;
            return 4;
        }
        case 0x4D: { // LD C,L
            cpu.registers.C = cpu.registers.L;
            return 4;
        }
        case 0x4E: { // LD C,(HL)
            cpu.registers.C = mmu.read(cpu.HL());
            return 8;
        }
        case 0x4F: { // LD C,A
            cpu.registers.C = cpu.registers.A;
            return 4;
        }
        
        // 0x50
        case 0x50: { // LD D,B
            cpu.registers.D = cpu.registers.B;
            return 4;
        }
        case 0x51: { // LD D,C
            cpu.registers.D = cpu.registers.C;
            return 4;
        }
        case 0x52: { // LD D,D
            return 4;
        }
        case 0x53: { // LD D,E
            cpu.registers.D = cpu.registers.E;
            return 4;
        }
        case 0x54: { // LD D,H
            cpu.registers.D = cpu.registers.H;
            return 4;
        }
        case 0x55: { // LD D,L
            cpu.registers.D = cpu.registers.L;
            return 4;
        }
        case 0x56: { // LD D,(HL)
            cpu.registers.D = mmu.read(cpu.HL());
            return 8;
        }
        case 0x57: { // LD D,A
            cpu.registers.D = cpu.registers.A;
            return 4;
        }
        case 0x58: { // LD E,B
            cpu.registers.E = cpu.registers.B;
            return 4;
        }
        case 0x59: { // LD E,C
            cpu.registers.E = cpu.registers.C;
            return 4;
        }
        case 0x5A: { // LD E,D
            cpu.registers.E = cpu.registers.D;
            return 4;
        }
        case 0x5B: { // LD E,E
            return 4;
        }
        case 0x5C: { // LD E,H
            cpu.registers.E = cpu.registers.H;
            return 4;
        }
        case 0x5D: { // LD E,L
            cpu.registers.E = cpu.registers.L;
            return 4;
        }
        case 0x5E: { // LD E,(HL)
            cpu.registers.E = mmu.read(cpu.HL());
            return 8;
        }
        case 0x5F: { // LD E,A
            cpu.registers.E = cpu.registers.A;
            return 4;
        }
        
        // 0x60
        case 0x60: { // LD H,B
            cpu.registers.H = cpu.registers.B;
            return 4;
        }
        case 0x61: { // LD H,C
            cpu.registers.H = cpu.registers.C;
            return 4;
        }
        case 0x62: { // LD H,D
            cpu.registers.H = cpu.registers.D;
            return 4;
        }
        case 0x63: { // LD H,E
            cpu.registers.H = cpu.registers.E;
            return 4;
        }
        case 0x64: { // LD H,H
            return 4;
        }
        case 0x65: { // LD H,L
            cpu.registers.H = cpu.registers.L;
            return 4;
        }
        case 0x66: { // LD H,(HL)
            cpu.registers.H = mmu.read(cpu.HL());
            return 8;
        }
        case 0x67: { // LD H,A
            cpu.registers.H = cpu.registers.A;
            return 4;
        }
        case 0x68: { // LD L,B
            cpu.registers.L = cpu.registers.B;
            return 4;
        }
        case 0x69: { // LD L,C
            cpu.registers.L = cpu.registers.C;
            return 4;
        }
        case 0x6A: { // LD L,D
            cpu.registers.L = cpu.registers.D;
            return 4;
        }
        case 0x6B: { // LD L,E
            cpu.registers.L = cpu.registers.E;
            return 4;
        }
        case 0x6C: { // LD L,H
            cpu.registers.L = cpu.registers.H;
            return 4;
        }
        case 0x6D: { // LD L,L
            return 4;
        }
        case 0x6E: { // LD L,(HL)
            cpu.registers.L = mmu.read(cpu.HL());
            return 8;
        }
        case 0x6F: { // LD L,A
            cpu.registers.L = cpu.registers.A;
            return 4;
        }
        
        // 0x70
        case 0x70: { // LD (HL),B
            mmu.write(cpu.HL(), cpu.registers.B);
            return 8;
        }
        case 0x71: { // LD (HL),C
            mmu.write(cpu.HL(), cpu.registers.C);
            return 8;
        }
        case 0x72: { // LD (HL),D
            mmu.write(cpu.HL(), cpu.registers.D);
            return 8;
        }
        case 0x73: { // LD (HL),E
            mmu.write(cpu.HL(), cpu.registers.E);
            return 8;
        }
        case 0x74: { // LD (HL),H
            mmu.write(cpu.HL(), cpu.registers.H);
            return 8;
        }
        case 0x75: { // LD (HL),L
            mmu.write(cpu.HL(), cpu.registers.L);
            return 8;
        }
        case 0x76: { // HALT
            cpu.halt = true;
            return 4;
        }
        case 0x77: { // LD (HL),A
            mmu.write(cpu.HL(), cpu.registers.A);
            return 8;
        }
        case 0x78: { // LD A,B
            cpu.registers.A = cpu.registers.B;
            return 4;
        }
        case 0x79: { // LD A,C
            cpu.registers.A = cpu.registers.C;
            return 4;
        }
        case 0x7A: { // LD A,D
            cpu.registers.A = cpu.registers.D;
            return 4;
        }
        case 0x7B: { // LD A,E
            cpu.registers.A = cpu.registers.E;
            return 4;
        }
        case 0x7C: { // LD A,H
            cpu.registers.A = cpu.registers.H;
            return 4;
        }
        case 0x7D: { // LD A,L
            cpu.registers.A = cpu.registers.L;
            return 4;
        }
        case 0x7E: { // LD A,(HL)
            cpu.registers.A = mmu.read(cpu.HL());
            return 8;
        }
        case 0x7F: { // LD A,A
            return 4;
        }
        
        // 0x80
        case 0x80: { // ADD A,B
            cpu.registers.A = add(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0x81: { // ADD A,C
            cpu.registers.A = add(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0x82: { // ADD A,D
            cpu.registers.A = add(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0x83: { // ADD A,E
            cpu.registers.A = add(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0x84: { // ADD A,H
            cpu.registers.A = add(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0x85: { // ADD A,L
            cpu.registers.A = add(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0x86: { // ADD A,(HL)
            cpu.registers.A = add(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0x87: { // ADD A,A
            cpu.registers.A = add(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        case 0x88: { // ADC A,B
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0x89: { // ADC A,C
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0x8A: { // ADC A,D
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0x8B: { // ADC A,E
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0x8C: { // ADC A,H
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0x8D: { // ADC A,L
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0x8E: { // ADC A,(HL)
            cpu.registers.A = adc(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0x8F: { // ADC A,A
            cpu.registers.A = adc(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        
        // 0x90
        case 0x90: { // SUB B
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0x91: { // SUB C
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0x92: { // SUB D
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0x93: { // SUB E
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0x94: { // SUB H
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0x95: { // SUB L
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0x96: { // SUB (HL)
            cpu.registers.A = sub(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0x97: { // SUB A
            cpu.registers.A = sub(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        case 0x98: { // SBC A,B
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0x99: { // SBC A,C
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0x9A: { // SBC A,D
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0x9B: { // SBC A,E
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0x9C: { // SBC A,H
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0x9D: { // SBC A,L
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0x9E: { // SBC A,(HL)
            cpu.registers.A = sbc(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0x9F: { // SBC A,A
            cpu.registers.A = sbc(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        
        // 0xA0
        case 0xA0: { // AND B
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0xA1: { // AND C
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0xA2: { // AND D
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0xA3: { // AND E
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0xA4: { // AND H
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0xA5: { // AND L
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0xA6: { // AND (HL)
            cpu.registers.A = and_(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0xA7: { // AND A
            cpu.registers.A = and_(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        case 0xA8: { // XOR B
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0xA9: { // XOR C
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0xAA: { // XOR D
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0xAB: { // XOR E
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0xAC: { // XOR H
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0xAD: { // XOR L
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0xAE: { // XOR (HL)
            cpu.registers.A = xor_(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0xAF: { // XOR A
            cpu.registers.A = xor_(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        
        // 0xB0
        case 0xB0: { // OR B
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0xB1: { // OR C
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0xB2: { // OR D
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0xB3: { // OR E
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0xB4: { // OR H
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0xB5: { // OR L
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0xB6: { // OR (HL)
            cpu.registers.A = or_(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0xB7: { // OR A
            cpu.registers.A = or_(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        case 0xB8: { // CP B
            cp(cpu.registers.A, cpu.registers.B);
            return 4;
        }
        case 0xB9: { // CP C
            cp(cpu.registers.A, cpu.registers.C);
            return 4;
        }
        case 0xBA: { // CP D
            cp(cpu.registers.A, cpu.registers.D);
            return 4;
        }
        case 0xBB: { // CP E
            cp(cpu.registers.A, cpu.registers.E);
            return 4;
        }
        case 0xBC: { // CP H
            cp(cpu.registers.A, cpu.registers.H);
            return 4;
        }
        case 0xBD: { // CP L
            cp(cpu.registers.A, cpu.registers.L);
            return 4;
        }
        case 0xBE: { // CP (HL)
            cp(cpu.registers.A, mmu.read(cpu.HL()));
            return 8;
        }
        case 0xBF: { // CP A
            cp(cpu.registers.A, cpu.registers.A);
            return 4;
        }
        
        // 0xC0
        case 0xC0: { // RET NZ
            if (!cpu.flag_Z()) {
                cpu.program_counter = pop();
                return 20;
            }
            return 8;
        }
        case 0xC1: { // POP BC
            cpu.set_BC(pop());
            return 12;
        }
        case 0xC2: { // JP NZ,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (!cpu.flag_Z()) {
                cpu.program_counter = addr;
                return 16;
            }
            return 12;
        }
        case 0xC3: { // JP a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            cpu.program_counter = static_cast<uint16_t>((high << 8) | low);
            return 16;
        }
        case 0xC4: { // CALL NZ,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (!cpu.flag_Z()) {
                push(cpu.program_counter);
                cpu.program_counter = addr;
                return 24;
            }
            return 12;
        }
        case 0xC5: { // PUSH BC
            push(cpu.BC());
            return 16;
        }
        case 0xC6: { // ADD A,d8
            cpu.registers.A = add(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xC7: { // RST 0x0
            push(cpu.program_counter);
            cpu.program_counter = 0x0;
            return 16;
        }
        case 0xC8: { // RET Z
            if (cpu.flag_Z()) {
                cpu.program_counter = pop();
                return 20;
            }
            return 8;
        }
        case 0xC9: { // RET
            cpu.program_counter = pop();
            return 16;
        }
        case 0xCA: { // JP Z,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (cpu.flag_Z()) {
                cpu.program_counter = addr;
                return 16;
            }
            return 12;
        }
        case 0xCB: { // CB prefix (handled separately)
            return 0; // Should not reach here
        }
        case 0xCC: { // CALL Z,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (cpu.flag_Z()) {
                push(cpu.program_counter);
                cpu.program_counter = addr;
                return 24;
            }
            return 12;
        }
        case 0xCD: { // CALL a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            push(cpu.program_counter);
            cpu.program_counter = addr;
            return 24;
        }
        case 0xCE: { // ADC A,d8
            cpu.registers.A = adc(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xCF: { // RST 0x8
            push(cpu.program_counter);
            cpu.program_counter = 0x8;
            return 16;
        }
        
        // 0xD0
        case 0xD0: { // RET NC
            if (!cpu.flag_C()) {
                cpu.program_counter = pop();
                return 20;
            }
            return 8;
        }
        case 0xD1: { // POP DE
            cpu.set_DE(pop());
            return 12;
        }
        case 0xD2: { // JP NC,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (!cpu.flag_C()) {
                cpu.program_counter = addr;
                return 16;
            }
            return 12;
        }
        case 0xD3: { // Unused
            return 4;
        }
        case 0xD4: { // CALL NC,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (!cpu.flag_C()) {
                push(cpu.program_counter);
                cpu.program_counter = addr;
                return 24;
            }
            return 12;
        }
        case 0xD5: { // PUSH DE
            push(cpu.DE());
            return 16;
        }
        case 0xD6: { // SUB d8
            cpu.registers.A = sub(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xD7: { // RST 0x10
            push(cpu.program_counter);
            cpu.program_counter = 0x10;
            return 16;
        }
        case 0xD8: { // RET C
            if (cpu.flag_C()) {
                cpu.program_counter = pop();
                return 20;
            }
            return 8;
        }
        case 0xD9: { // RETI
            cpu.program_counter = pop();
            cpu.interrupt_master_enable = true;
            return 16;
        }
        case 0xDA: { // JP C,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (cpu.flag_C()) {
                cpu.program_counter = addr;
                return 16;
            }
            return 12;
        }
        case 0xDB: { // Unused
            return 4;
        }
        case 0xDC: { // CALL C,a16
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            if (cpu.flag_C()) {
                push(cpu.program_counter);
                cpu.program_counter = addr;
                return 24;
            }
            return 12;
        }
        case 0xDD: { // Unused
            return 4;
        }
        case 0xDE: { // SBC A,d8
            cpu.registers.A = sbc(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xDF: { // RST 0x18
            push(cpu.program_counter);
            cpu.program_counter = 0x18;
            return 16;
        }
        
        // 0xE0
        case 0xE0: { // LDH (a8),A
            uint8_t addr = mmu.read(cpu.program_counter++);
            mmu.write(0xFF00 + addr, cpu.registers.A);
            return 12;
        }
        case 0xE1: { // POP HL
            cpu.set_HL(pop());
            return 12;
        }
        case 0xE2: { // LD (C),A
            mmu.write(0xFF00 + cpu.registers.C, cpu.registers.A);
            return 8;
        }
        case 0xE3: { // Unused
            return 4;
        }
        case 0xE4: { // Unused
            return 4;
        }
        case 0xE5: { // PUSH HL
            push(cpu.HL());
            return 16;
        }
        case 0xE6: { // AND d8
            cpu.registers.A = and_(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xE7: { // RST 0x20
            push(cpu.program_counter);
            cpu.program_counter = 0x20;
            return 16;
        }
        case 0xE8: { // ADD SP,r8
            int8_t val = static_cast<int8_t>(mmu.read(cpu.program_counter++));
            uint16_t res = (cpu.stack_pointer + val) & 0xFFFF;
            cpu.set_flag_Z(false);
            cpu.set_flag_N(false);
            cpu.set_flag_H((cpu.stack_pointer & 0xF) + (val & 0xF) > 0xF);
            cpu.set_flag_C((cpu.stack_pointer & 0xFF) + (val & 0xFF) > 0xFF);
            cpu.stack_pointer = res;
            return 16;
        }
        case 0xE9: { // JP HL
            cpu.program_counter = cpu.HL();
            return 4;
        }
        case 0xEA: { // LD (a16),A
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            mmu.write(addr, cpu.registers.A);
            return 16;
        }
        case 0xEB: { // Unused
            return 4;
        }
        case 0xEC: { // Unused
            return 4;
        }
        case 0xED: { // Unused
            return 4;
        }
        case 0xEE: { // XOR d8
            cpu.registers.A = xor_(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xEF: { // RST 0x28
            push(cpu.program_counter);
            cpu.program_counter = 0x28;
            return 16;
        }
        
        // 0xF0
        case 0xF0: { // LDH A,(a8)
            uint8_t addr = mmu.read(cpu.program_counter++);
            cpu.registers.A = mmu.read(0xFF00 + addr);
            return 12;
        }
        case 0xF1: { // POP AF
            cpu.set_AF(pop());
            return 12;
        }
        case 0xF2: { // LD A,(C)
            cpu.registers.A = mmu.read(0xFF00 + cpu.registers.C);
            return 8;
        }
        case 0xF3: { // DI
            cpu.interrupt_master_enable = false;
            return 4;
        }
        case 0xF4: { // Unused
            return 4;
        }
        case 0xF5: { // PUSH AF
            push(cpu.AF());
            return 16;
        }
        case 0xF6: { // OR d8
            cpu.registers.A = or_(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xF7: { // RST 0x30
            push(cpu.program_counter);
            cpu.program_counter = 0x30;
            return 16;
        }
        case 0xF8: { // LD HL,SP+r8
            int8_t val = static_cast<int8_t>(mmu.read(cpu.program_counter++));
            uint16_t res = static_cast<uint16_t>(cpu.stack_pointer + val);
            cpu.set_flag_Z(false);
            cpu.set_flag_N(false);
            cpu.set_flag_H((cpu.stack_pointer & 0xF) + (val & 0xF) > 0xF);
            cpu.set_flag_C((cpu.stack_pointer & 0xFF) + (val & 0xFF) > 0xFF);
            cpu.set_HL(res & 0xFFFF);
            return 12;
        }
        case 0xF9: { // LD SP,HL
            cpu.stack_pointer = cpu.HL();
            return 8;
        }
        case 0xFA: { // LD A,(a16)
            uint8_t low = mmu.read(cpu.program_counter++);
            uint8_t high = mmu.read(cpu.program_counter++);
            uint16_t addr = static_cast<uint16_t>((high << 8) | low);
            cpu.registers.A = mmu.read(addr);
            return 16;
        }
        case 0xFB: { // EI
            cpu.interrupt_master_enable = true;
            return 4;
        }
        case 0xFC: { // Unused
            return 4;
        }
        case 0xFD: { // Unused
            return 4;
        }
        case 0xFE: { // CP d8
            cp(cpu.registers.A, mmu.read(cpu.program_counter++));
            return 8;
        }
        case 0xFF: { // RST 0x38
            push(cpu.program_counter);
            cpu.program_counter = 0x38;
            return 16;
        }
        
        default:
            return 4;
    }
}

} // namespace cboy::instructions
