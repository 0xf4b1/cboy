// SPDX-License-Identifier: GPL-3.0-only

#include "cb.hpp"
#include "../cpu.hpp"
#include "../mmu.hpp"

namespace cboy::instructions::cb {

namespace {

inline uint8_t rlc(CPU& cpu, uint8_t value) {
    bool c = (value >> 7) & 1;
    value = ((value << 1) | c) & 0xFF;
    cpu.set_flag_Z(value == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(c);
    return value;
}

inline uint8_t rrc(CPU& cpu, uint8_t value) {
    bool c = value & 1;
    value = ((value >> 1) | (c << 7)) & 0xFF;
    cpu.set_flag_Z(value == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(c);
    return value;
}

inline uint8_t rl(CPU& cpu, uint8_t value) {
    bool c = (value >> 7) & 1;
    value = ((value << 1) | cpu.flag_C()) & 0xFF;
    cpu.set_flag_Z(value == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(c);
    return value;
}

inline uint8_t rr(CPU& cpu, uint8_t value) {
    bool c = value & 1;
    value = ((value >> 1) | (cpu.flag_C() << 7)) & 0xFF;
    cpu.set_flag_Z(value == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(c);
    return value;
}

inline uint8_t sla(CPU& cpu, uint8_t value) {
    bool c = value >> 7 & 1;
    value = value << 1 & 0xFF;
    cpu.set_flag_Z(value == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(c);
    return value;
}

inline uint8_t sra(CPU& cpu, uint8_t value) {
    bool c = value & 1;
    value = (value >> 1 | (value & (1 << 7))) & 0xFF;
    cpu.set_flag_Z(value == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(c);
    return value;
}

inline uint8_t swap(CPU& cpu, uint8_t value) {
    uint8_t res = (value << 4 | value >> 4) & 0xFF;
    cpu.set_flag_Z(res == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(false);
    return res;
}

inline uint8_t srl(CPU& cpu, uint8_t value) {
    uint8_t res = value >> 1;
    cpu.set_flag_Z(res == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(false);
    cpu.set_flag_C(value & 1);
    return res;
}

inline uint8_t bit(CPU& cpu, uint8_t value, uint8_t i) {
    cpu.set_flag_Z((value >> i & 1) == 0);
    cpu.set_flag_N(false);
    cpu.set_flag_H(true);
    return value;
}

inline uint8_t res(uint8_t value, uint8_t i) {
    return static_cast<uint8_t>(value & ~(1 << i));
}

inline uint8_t set(uint8_t value, uint8_t i) {
    return static_cast<uint8_t>(value | (1 << i));
}

} // namespace

uint8_t execute(uint8_t opcode, CPU& cpu, MMU& mmu) {
    switch (opcode) {
        // 0x00 - RLC operations
        case 0x00: { cpu.registers.B = rlc(cpu, cpu.registers.B); return 8; }
        case 0x01: { cpu.registers.C = rlc(cpu, cpu.registers.C); return 8; }
        case 0x02: { cpu.registers.D = rlc(cpu, cpu.registers.D); return 8; }
        case 0x03: { cpu.registers.E = rlc(cpu, cpu.registers.E); return 8; }
        case 0x04: { cpu.registers.H = rlc(cpu, cpu.registers.H); return 8; }
        case 0x05: { cpu.registers.L = rlc(cpu, cpu.registers.L); return 8; }
        case 0x06: { mmu.write(cpu.HL(), rlc(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x07: { cpu.registers.A = rlc(cpu, cpu.registers.A); return 8; }
        
        // 0x08 - RRC operations
        case 0x08: { cpu.registers.B = rrc(cpu, cpu.registers.B); return 8; }
        case 0x09: { cpu.registers.C = rrc(cpu, cpu.registers.C); return 8; }
        case 0x0A: { cpu.registers.D = rrc(cpu, cpu.registers.D); return 8; }
        case 0x0B: { cpu.registers.E = rrc(cpu, cpu.registers.E); return 8; }
        case 0x0C: { cpu.registers.H = rrc(cpu, cpu.registers.H); return 8; }
        case 0x0D: { cpu.registers.L = rrc(cpu, cpu.registers.L); return 8; }
        case 0x0E: { mmu.write(cpu.HL(), rrc(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x0F: { cpu.registers.A = rrc(cpu, cpu.registers.A); return 8; }
        
        // 0x10 - RL operations
        case 0x10: { cpu.registers.B = rl(cpu, cpu.registers.B); return 8; }
        case 0x11: { cpu.registers.C = rl(cpu, cpu.registers.C); return 8; }
        case 0x12: { cpu.registers.D = rl(cpu, cpu.registers.D); return 8; }
        case 0x13: { cpu.registers.E = rl(cpu, cpu.registers.E); return 8; }
        case 0x14: { cpu.registers.H = rl(cpu, cpu.registers.H); return 8; }
        case 0x15: { cpu.registers.L = rl(cpu, cpu.registers.L); return 8; }
        case 0x16: { mmu.write(cpu.HL(), rl(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x17: { cpu.registers.A = rl(cpu, cpu.registers.A); return 8; }
        
        // 0x18 - RR operations
        case 0x18: { cpu.registers.B = rr(cpu, cpu.registers.B); return 8; }
        case 0x19: { cpu.registers.C = rr(cpu, cpu.registers.C); return 8; }
        case 0x1A: { cpu.registers.D = rr(cpu, cpu.registers.D); return 8; }
        case 0x1B: { cpu.registers.E = rr(cpu, cpu.registers.E); return 8; }
        case 0x1C: { cpu.registers.H = rr(cpu, cpu.registers.H); return 8; }
        case 0x1D: { cpu.registers.L = rr(cpu, cpu.registers.L); return 8; }
        case 0x1E: { mmu.write(cpu.HL(), rr(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x1F: { cpu.registers.A = rr(cpu, cpu.registers.A); return 8; }
        
        // 0x20 - SLA operations
        case 0x20: { cpu.registers.B = sla(cpu, cpu.registers.B); return 8; }
        case 0x21: { cpu.registers.C = sla(cpu, cpu.registers.C); return 8; }
        case 0x22: { cpu.registers.D = sla(cpu, cpu.registers.D); return 8; }
        case 0x23: { cpu.registers.E = sla(cpu, cpu.registers.E); return 8; }
        case 0x24: { cpu.registers.H = sla(cpu, cpu.registers.H); return 8; }
        case 0x25: { cpu.registers.L = sla(cpu, cpu.registers.L); return 8; }
        case 0x26: { mmu.write(cpu.HL(), sla(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x27: { cpu.registers.A = sla(cpu, cpu.registers.A); return 8; }
        
        // 0x28 - SRA operations
        case 0x28: { cpu.registers.B = sra(cpu, cpu.registers.B); return 8; }
        case 0x29: { cpu.registers.C = sra(cpu, cpu.registers.C); return 8; }
        case 0x2A: { cpu.registers.D = sra(cpu, cpu.registers.D); return 8; }
        case 0x2B: { cpu.registers.E = sra(cpu, cpu.registers.E); return 8; }
        case 0x2C: { cpu.registers.H = sra(cpu, cpu.registers.H); return 8; }
        case 0x2D: { cpu.registers.L = sra(cpu, cpu.registers.L); return 8; }
        case 0x2E: { mmu.write(cpu.HL(), sra(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x2F: { cpu.registers.A = sra(cpu, cpu.registers.A); return 8; }
        
        // 0x30 - SWAP operations
        case 0x30: { cpu.registers.B = swap(cpu, cpu.registers.B); return 8; }
        case 0x31: { cpu.registers.C = swap(cpu, cpu.registers.C); return 8; }
        case 0x32: { cpu.registers.D = swap(cpu, cpu.registers.D); return 8; }
        case 0x33: { cpu.registers.E = swap(cpu, cpu.registers.E); return 8; }
        case 0x34: { cpu.registers.H = swap(cpu, cpu.registers.H); return 8; }
        case 0x35: { cpu.registers.L = swap(cpu, cpu.registers.L); return 8; }
        case 0x36: { mmu.write(cpu.HL(), swap(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x37: { cpu.registers.A = swap(cpu, cpu.registers.A); return 8; }
        
        // 0x38 - SRL operations
        case 0x38: { cpu.registers.B = srl(cpu, cpu.registers.B); return 8; }
        case 0x39: { cpu.registers.C = srl(cpu, cpu.registers.C); return 8; }
        case 0x3A: { cpu.registers.D = srl(cpu, cpu.registers.D); return 8; }
        case 0x3B: { cpu.registers.E = srl(cpu, cpu.registers.E); return 8; }
        case 0x3C: { cpu.registers.H = srl(cpu, cpu.registers.H); return 8; }
        case 0x3D: { cpu.registers.L = srl(cpu, cpu.registers.L); return 8; }
        case 0x3E: { mmu.write(cpu.HL(), srl(cpu, mmu.read(cpu.HL()))); return 16; }
        case 0x3F: { cpu.registers.A = srl(cpu, cpu.registers.A); return 8; }
        
        // 0x40 - BIT 0 operations
        case 0x40: { bit(cpu, cpu.registers.B, 0); return 8; }
        case 0x41: { bit(cpu, cpu.registers.C, 0); return 8; }
        case 0x42: { bit(cpu, cpu.registers.D, 0); return 8; }
        case 0x43: { bit(cpu, cpu.registers.E, 0); return 8; }
        case 0x44: { bit(cpu, cpu.registers.H, 0); return 8; }
        case 0x45: { bit(cpu, cpu.registers.L, 0); return 8; }
        case 0x46: { bit(cpu, mmu.read(cpu.HL()), 0); return 12; }
        case 0x47: { bit(cpu, cpu.registers.A, 0); return 8; }
        
        // 0x48 - BIT 1 operations
        case 0x48: { bit(cpu, cpu.registers.B, 1); return 8; }
        case 0x49: { bit(cpu, cpu.registers.C, 1); return 8; }
        case 0x4A: { bit(cpu, cpu.registers.D, 1); return 8; }
        case 0x4B: { bit(cpu, cpu.registers.E, 1); return 8; }
        case 0x4C: { bit(cpu, cpu.registers.H, 1); return 8; }
        case 0x4D: { bit(cpu, cpu.registers.L, 1); return 8; }
        case 0x4E: { bit(cpu, mmu.read(cpu.HL()), 1); return 12; }
        case 0x4F: { bit(cpu, cpu.registers.A, 1); return 8; }
        
        // 0x50 - BIT 2 operations
        case 0x50: { bit(cpu, cpu.registers.B, 2); return 8; }
        case 0x51: { bit(cpu, cpu.registers.C, 2); return 8; }
        case 0x52: { bit(cpu, cpu.registers.D, 2); return 8; }
        case 0x53: { bit(cpu, cpu.registers.E, 2); return 8; }
        case 0x54: { bit(cpu, cpu.registers.H, 2); return 8; }
        case 0x55: { bit(cpu, cpu.registers.L, 2); return 8; }
        case 0x56: { bit(cpu, mmu.read(cpu.HL()), 2); return 12; }
        case 0x57: { bit(cpu, cpu.registers.A, 2); return 8; }
        
        // 0x58 - BIT 3 operations
        case 0x58: { bit(cpu, cpu.registers.B, 3); return 8; }
        case 0x59: { bit(cpu, cpu.registers.C, 3); return 8; }
        case 0x5A: { bit(cpu, cpu.registers.D, 3); return 8; }
        case 0x5B: { bit(cpu, cpu.registers.E, 3); return 8; }
        case 0x5C: { bit(cpu, cpu.registers.H, 3); return 8; }
        case 0x5D: { bit(cpu, cpu.registers.L, 3); return 8; }
        case 0x5E: { bit(cpu, mmu.read(cpu.HL()), 3); return 12; }
        case 0x5F: { bit(cpu, cpu.registers.A, 3); return 8; }
        
        // 0x60 - BIT 4 operations
        case 0x60: { bit(cpu, cpu.registers.B, 4); return 8; }
        case 0x61: { bit(cpu, cpu.registers.C, 4); return 8; }
        case 0x62: { bit(cpu, cpu.registers.D, 4); return 8; }
        case 0x63: { bit(cpu, cpu.registers.E, 4); return 8; }
        case 0x64: { bit(cpu, cpu.registers.H, 4); return 8; }
        case 0x65: { bit(cpu, cpu.registers.L, 4); return 8; }
        case 0x66: { bit(cpu, mmu.read(cpu.HL()), 4); return 12; }
        case 0x67: { bit(cpu, cpu.registers.A, 4); return 8; }
        
        // 0x68 - BIT 5 operations
        case 0x68: { bit(cpu, cpu.registers.B, 5); return 8; }
        case 0x69: { bit(cpu, cpu.registers.C, 5); return 8; }
        case 0x6A: { bit(cpu, cpu.registers.D, 5); return 8; }
        case 0x6B: { bit(cpu, cpu.registers.E, 5); return 8; }
        case 0x6C: { bit(cpu, cpu.registers.H, 5); return 8; }
        case 0x6D: { bit(cpu, cpu.registers.L, 5); return 8; }
        case 0x6E: { bit(cpu, mmu.read(cpu.HL()), 5); return 12; }
        case 0x6F: { bit(cpu, cpu.registers.A, 5); return 8; }
        
        // 0x70 - BIT 6 operations
        case 0x70: { bit(cpu, cpu.registers.B, 6); return 8; }
        case 0x71: { bit(cpu, cpu.registers.C, 6); return 8; }
        case 0x72: { bit(cpu, cpu.registers.D, 6); return 8; }
        case 0x73: { bit(cpu, cpu.registers.E, 6); return 8; }
        case 0x74: { bit(cpu, cpu.registers.H, 6); return 8; }
        case 0x75: { bit(cpu, cpu.registers.L, 6); return 8; }
        case 0x76: { bit(cpu, mmu.read(cpu.HL()), 6); return 12; }
        case 0x77: { bit(cpu, cpu.registers.A, 6); return 8; }
        
        // 0x78 - BIT 7 operations
        case 0x78: { bit(cpu, cpu.registers.B, 7); return 8; }
        case 0x79: { bit(cpu, cpu.registers.C, 7); return 8; }
        case 0x7A: { bit(cpu, cpu.registers.D, 7); return 8; }
        case 0x7B: { bit(cpu, cpu.registers.E, 7); return 8; }
        case 0x7C: { bit(cpu, cpu.registers.H, 7); return 8; }
        case 0x7D: { bit(cpu, cpu.registers.L, 7); return 8; }
        case 0x7E: { bit(cpu, mmu.read(cpu.HL()), 7); return 12; }
        case 0x7F: { bit(cpu, cpu.registers.A, 7); return 8; }
        
        // 0x80 - RES 0 operations
        case 0x80: { cpu.registers.B = res(cpu.registers.B, 0); return 8; }
        case 0x81: { cpu.registers.C = res(cpu.registers.C, 0); return 8; }
        case 0x82: { cpu.registers.D = res(cpu.registers.D, 0); return 8; }
        case 0x83: { cpu.registers.E = res(cpu.registers.E, 0); return 8; }
        case 0x84: { cpu.registers.H = res(cpu.registers.H, 0); return 8; }
        case 0x85: { cpu.registers.L = res(cpu.registers.L, 0); return 8; }
        case 0x86: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 0)); return 16; }
        case 0x87: { cpu.registers.A = res(cpu.registers.A, 0); return 8; }
        
        // 0x88 - RES 1 operations
        case 0x88: { cpu.registers.B = res(cpu.registers.B, 1); return 8; }
        case 0x89: { cpu.registers.C = res(cpu.registers.C, 1); return 8; }
        case 0x8A: { cpu.registers.D = res(cpu.registers.D, 1); return 8; }
        case 0x8B: { cpu.registers.E = res(cpu.registers.E, 1); return 8; }
        case 0x8C: { cpu.registers.H = res(cpu.registers.H, 1); return 8; }
        case 0x8D: { cpu.registers.L = res(cpu.registers.L, 1); return 8; }
        case 0x8E: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 1)); return 16; }
        case 0x8F: { cpu.registers.A = res(cpu.registers.A, 1); return 8; }
        
        // 0x90 - RES 2 operations
        case 0x90: { cpu.registers.B = res(cpu.registers.B, 2); return 8; }
        case 0x91: { cpu.registers.C = res(cpu.registers.C, 2); return 8; }
        case 0x92: { cpu.registers.D = res(cpu.registers.D, 2); return 8; }
        case 0x93: { cpu.registers.E = res(cpu.registers.E, 2); return 8; }
        case 0x94: { cpu.registers.H = res(cpu.registers.H, 2); return 8; }
        case 0x95: { cpu.registers.L = res(cpu.registers.L, 2); return 8; }
        case 0x96: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 2)); return 16; }
        case 0x97: { cpu.registers.A = res(cpu.registers.A, 2); return 8; }
        
        // 0x98 - RES 3 operations
        case 0x98: { cpu.registers.B = res(cpu.registers.B, 3); return 8; }
        case 0x99: { cpu.registers.C = res(cpu.registers.C, 3); return 8; }
        case 0x9A: { cpu.registers.D = res(cpu.registers.D, 3); return 8; }
        case 0x9B: { cpu.registers.E = res(cpu.registers.E, 3); return 8; }
        case 0x9C: { cpu.registers.H = res(cpu.registers.H, 3); return 8; }
        case 0x9D: { cpu.registers.L = res(cpu.registers.L, 3); return 8; }
        case 0x9E: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 3)); return 16; }
        case 0x9F: { cpu.registers.A = res(cpu.registers.A, 3); return 8; }
        
        // 0xA0 - RES 4 operations
        case 0xA0: { cpu.registers.B = res(cpu.registers.B, 4); return 8; }
        case 0xA1: { cpu.registers.C = res(cpu.registers.C, 4); return 8; }
        case 0xA2: { cpu.registers.D = res(cpu.registers.D, 4); return 8; }
        case 0xA3: { cpu.registers.E = res(cpu.registers.E, 4); return 8; }
        case 0xA4: { cpu.registers.H = res(cpu.registers.H, 4); return 8; }
        case 0xA5: { cpu.registers.L = res(cpu.registers.L, 4); return 8; }
        case 0xA6: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 4)); return 16; }
        case 0xA7: { cpu.registers.A = res(cpu.registers.A, 4); return 8; }
        
        // 0xA8 - RES 5 operations
        case 0xA8: { cpu.registers.B = res(cpu.registers.B, 5); return 8; }
        case 0xA9: { cpu.registers.C = res(cpu.registers.C, 5); return 8; }
        case 0xAA: { cpu.registers.D = res(cpu.registers.D, 5); return 8; }
        case 0xAB: { cpu.registers.E = res(cpu.registers.E, 5); return 8; }
        case 0xAC: { cpu.registers.H = res(cpu.registers.H, 5); return 8; }
        case 0xAD: { cpu.registers.L = res(cpu.registers.L, 5); return 8; }
        case 0xAE: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 5)); return 16; }
        case 0xAF: { cpu.registers.A = res(cpu.registers.A, 5); return 8; }
        
        // 0xB0 - RES 6 operations
        case 0xB0: { cpu.registers.B = res(cpu.registers.B, 6); return 8; }
        case 0xB1: { cpu.registers.C = res(cpu.registers.C, 6); return 8; }
        case 0xB2: { cpu.registers.D = res(cpu.registers.D, 6); return 8; }
        case 0xB3: { cpu.registers.E = res(cpu.registers.E, 6); return 8; }
        case 0xB4: { cpu.registers.H = res(cpu.registers.H, 6); return 8; }
        case 0xB5: { cpu.registers.L = res(cpu.registers.L, 6); return 8; }
        case 0xB6: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 6)); return 16; }
        case 0xB7: { cpu.registers.A = res(cpu.registers.A, 6); return 8; }
        
        // 0xB8 - RES 7 operations
        case 0xB8: { cpu.registers.B = res(cpu.registers.B, 7); return 8; }
        case 0xB9: { cpu.registers.C = res(cpu.registers.C, 7); return 8; }
        case 0xBA: { cpu.registers.D = res(cpu.registers.D, 7); return 8; }
        case 0xBB: { cpu.registers.E = res(cpu.registers.E, 7); return 8; }
        case 0xBC: { cpu.registers.H = res(cpu.registers.H, 7); return 8; }
        case 0xBD: { cpu.registers.L = res(cpu.registers.L, 7); return 8; }
        case 0xBE: { mmu.write(cpu.HL(), res(mmu.read(cpu.HL()), 7)); return 16; }
        case 0xBF: { cpu.registers.A = res(cpu.registers.A, 7); return 8; }
        
        // 0xC0 - SET 0 operations
        case 0xC0: { cpu.registers.B = set(cpu.registers.B, 0); return 8; }
        case 0xC1: { cpu.registers.C = set(cpu.registers.C, 0); return 8; }
        case 0xC2: { cpu.registers.D = set(cpu.registers.D, 0); return 8; }
        case 0xC3: { cpu.registers.E = set(cpu.registers.E, 0); return 8; }
        case 0xC4: { cpu.registers.H = set(cpu.registers.H, 0); return 8; }
        case 0xC5: { cpu.registers.L = set(cpu.registers.L, 0); return 8; }
        case 0xC6: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 0)); return 16; }
        case 0xC7: { cpu.registers.A = set(cpu.registers.A, 0); return 8; }
        
        // 0xC8 - SET 1 operations
        case 0xC8: { cpu.registers.B = set(cpu.registers.B, 1); return 8; }
        case 0xC9: { cpu.registers.C = set(cpu.registers.C, 1); return 8; }
        case 0xCA: { cpu.registers.D = set(cpu.registers.D, 1); return 8; }
        case 0xCB: { cpu.registers.E = set(cpu.registers.E, 1); return 8; }
        case 0xCC: { cpu.registers.H = set(cpu.registers.H, 1); return 8; }
        case 0xCD: { cpu.registers.L = set(cpu.registers.L, 1); return 8; }
        case 0xCE: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 1)); return 16; }
        case 0xCF: { cpu.registers.A = set(cpu.registers.A, 1); return 8; }
        
        // 0xD0 - SET 2 operations
        case 0xD0: { cpu.registers.B = set(cpu.registers.B, 2); return 8; }
        case 0xD1: { cpu.registers.C = set(cpu.registers.C, 2); return 8; }
        case 0xD2: { cpu.registers.D = set(cpu.registers.D, 2); return 8; }
        case 0xD3: { cpu.registers.E = set(cpu.registers.E, 2); return 8; }
        case 0xD4: { cpu.registers.H = set(cpu.registers.H, 2); return 8; }
        case 0xD5: { cpu.registers.L = set(cpu.registers.L, 2); return 8; }
        case 0xD6: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 2)); return 16; }
        case 0xD7: { cpu.registers.A = set(cpu.registers.A, 2); return 8; }
        
        // 0xD8 - SET 3 operations
        case 0xD8: { cpu.registers.B = set(cpu.registers.B, 3); return 8; }
        case 0xD9: { cpu.registers.C = set(cpu.registers.C, 3); return 8; }
        case 0xDA: { cpu.registers.D = set(cpu.registers.D, 3); return 8; }
        case 0xDB: { cpu.registers.E = set(cpu.registers.E, 3); return 8; }
        case 0xDC: { cpu.registers.H = set(cpu.registers.H, 3); return 8; }
        case 0xDD: { cpu.registers.L = set(cpu.registers.L, 3); return 8; }
        case 0xDE: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 3)); return 16; }
        case 0xDF: { cpu.registers.A = set(cpu.registers.A, 3); return 8; }
        
        // 0xE0 - SET 4 operations
        case 0xE0: { cpu.registers.B = set(cpu.registers.B, 4); return 8; }
        case 0xE1: { cpu.registers.C = set(cpu.registers.C, 4); return 8; }
        case 0xE2: { cpu.registers.D = set(cpu.registers.D, 4); return 8; }
        case 0xE3: { cpu.registers.E = set(cpu.registers.E, 4); return 8; }
        case 0xE4: { cpu.registers.H = set(cpu.registers.H, 4); return 8; }
        case 0xE5: { cpu.registers.L = set(cpu.registers.L, 4); return 8; }
        case 0xE6: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 4)); return 16; }
        case 0xE7: { cpu.registers.A = set(cpu.registers.A, 4); return 8; }
        
        // 0xE8 - SET 5 operations
        case 0xE8: { cpu.registers.B = set(cpu.registers.B, 5); return 8; }
        case 0xE9: { cpu.registers.C = set(cpu.registers.C, 5); return 8; }
        case 0xEA: { cpu.registers.D = set(cpu.registers.D, 5); return 8; }
        case 0xEB: { cpu.registers.E = set(cpu.registers.E, 5); return 8; }
        case 0xEC: { cpu.registers.H = set(cpu.registers.H, 5); return 8; }
        case 0xED: { cpu.registers.L = set(cpu.registers.L, 5); return 8; }
        case 0xEE: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 5)); return 16; }
        case 0xEF: { cpu.registers.A = set(cpu.registers.A, 5); return 8; }
        
        // 0xF0 - SET 6 operations
        case 0xF0: { cpu.registers.B = set(cpu.registers.B, 6); return 8; }
        case 0xF1: { cpu.registers.C = set(cpu.registers.C, 6); return 8; }
        case 0xF2: { cpu.registers.D = set(cpu.registers.D, 6); return 8; }
        case 0xF3: { cpu.registers.E = set(cpu.registers.E, 6); return 8; }
        case 0xF4: { cpu.registers.H = set(cpu.registers.H, 6); return 8; }
        case 0xF5: { cpu.registers.L = set(cpu.registers.L, 6); return 8; }
        case 0xF6: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 6)); return 16; }
        case 0xF7: { cpu.registers.A = set(cpu.registers.A, 6); return 8; }
        
        // 0xF8 - SET 7 operations
        case 0xF8: { cpu.registers.B = set(cpu.registers.B, 7); return 8; }
        case 0xF9: { cpu.registers.C = set(cpu.registers.C, 7); return 8; }
        case 0xFA: { cpu.registers.D = set(cpu.registers.D, 7); return 8; }
        case 0xFB: { cpu.registers.E = set(cpu.registers.E, 7); return 8; }
        case 0xFC: { cpu.registers.H = set(cpu.registers.H, 7); return 8; }
        case 0xFD: { cpu.registers.L = set(cpu.registers.L, 7); return 8; }
        case 0xFE: { mmu.write(cpu.HL(), set(mmu.read(cpu.HL()), 7)); return 16; }
        case 0xFF: { cpu.registers.A = set(cpu.registers.A, 7); return 8; }
        
        default:
            return 8;
    }
}

} // namespace cboy::instructions::cb
