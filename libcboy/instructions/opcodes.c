// SPDX-License-Identifier: GPL-3.0-only

#include "cb.c"
#include "instructions.c"

unsigned char (*opcodes[0x100])() = {[0 ... 0xFF] = NOP};

unsigned char lenghts[0x100];

unsigned char op_length(unsigned char opcode) { return lenghts[opcode]; }

unsigned char (*ops[11])(unsigned char a, unsigned char b) = {ADD, ADC, SUB, SBC, AND, XOR, OR, CP};

unsigned char(*ops_cb[11]) = {RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL, BIT, RES, SET};
unsigned char(*registers[8]) = {&gameboy.cpu.B, &gameboy.cpu.C, &gameboy.cpu.D, &gameboy.cpu.E,
                                &gameboy.cpu.H, &gameboy.cpu.L, &gameboy.cpu.H, &gameboy.cpu.A};

unsigned short (*get_registers[8])() = {BC, DE, HL, SP, BC, DE, HL, AF};

void (*set_registers[8])(unsigned short) = {set_BC, set_DE, set_HL, set_SP, set_BC, set_DE, set_HL, set_AF};

unsigned char decode_and_execute_static(unsigned char opcode, unsigned short arg) {
    if (lenghts[opcode] == 1) {
        return opcodes[opcode]();
    } else if (lenghts[opcode] == 2) {
        return ((int (*)(unsigned char))opcodes[opcode])(arg);
    } else {
        return ((int (*)(unsigned short))opcodes[opcode])(arg);
    }
}

unsigned char decode_and_execute(unsigned char opcode, unsigned short arg) {
    if (opcode < 0x40) {
        if (opcode % 16 == 1) {
            // LD XX,d16
            set_registers[opcode / 16](arg);
            return 12;
        } else if (opcode % 16 == 3) {
            // INC XX
            set_registers[opcode / 16](get_registers[opcode / 16]() + 1);
            return 8;
        } else if (opcode % 16 == 0x9) {
            // ADD HL,XX
            set_HL(ADD_HL_n(HL(), get_registers[opcode / 16]()));
            return 8;
        } else if (opcode % 16 == 0xB) {
            // DEC XX
            set_registers[opcode / 16](get_registers[opcode / 16]() - 1);
            return 8;
        } else if (opcode % 8 == 4) {
            if (opcode == 0x34) {
                // INC (HL)
                write_mmu(HL(), INC(read_mmu(HL())));
                return 12;
            } else {
                // INC X
                *registers[opcode / 8] = INC(*registers[opcode / 8]);
                return 4;
            }
        } else if (opcode % 8 == 5) {
            if (opcode == 0x35) {
                // DEC (HL)
                write_mmu(HL(), DEC(read_mmu(HL())));
                return 12;
            } else {
                // DEC X
                *registers[opcode / 8] = DEC(*registers[opcode / 8]);
                return 4;
            }
        } else if (opcode % 8 == 6) {
            if (opcode == 0x36) {
                // LD (HL),d8
                write_mmu(HL(), arg);
                return 12;
            } else {
                // LD X,d8
                *registers[opcode / 8] = arg;
                return 8;
            }
        } else {
            return decode_and_execute_static(opcode, arg);
        }
    } else if (opcode < 0x80) {
        // LD
        if (opcode % 8 == 6) {
            if (opcode == 0x76) {
                // HALT
                return NOP();
            }
            // LD X,(HL)
            *registers[(opcode - 0x40) / 8] = read_mmu(HL());
            return 8;
        } else if (opcode / 8 == 0xE) {
            // LD (HL),X
            write_mmu(HL(), *registers[opcode % 8]);
            return 8;
        } else {
            // LD X,X
            *registers[(opcode - 0x40) / 8] = *registers[opcode % 8];
            return 4;
        }
    } else if (opcode < 0xC0) {
        // ADD, ADC, SUB, SBC, AND, XOR, OR, CP
        if (opcode % 8 == 6) {
            gameboy.cpu.A = ops[(opcode % 0x40) / 8](gameboy.cpu.A, read_mmu(HL()));
            return 8;
        } else {
            gameboy.cpu.A =
                ops[(opcode % 0x40) / 8](gameboy.cpu.A, opcode % 8 == 6 ? read_mmu(HL()) : *registers[opcode % 8]);
            return 4;
        }
    } else {
        if (opcode % 16 == 1) {
            // POP XX
            set_registers[(opcode % 0x40) / 16 + 4](POP());
            return 12;
        } else if (opcode % 16 == 5) {
            // PUSH XX
            PUSH(get_registers[(opcode % 0x40) / 16 + 4]());
            return 16;
        } else if (opcode % 8 == 6) {
            if (opcode == 0xFE) {
                // CP d8
                ops[(opcode % 0x40) / 8](gameboy.cpu.A, arg);
            } else {
                // XXX A,d8
                gameboy.cpu.A = ops[(opcode % 0x40) / 8](gameboy.cpu.A, arg);
            }
            return 8;
        } else if (opcode % 8 == 7) {
            // RST XXH
            RST(((opcode % 0x40) / 8) * 8);
            return 16;
        } else {
            return decode_and_execute_static(opcode, arg);
        }
    }
}

void decode_and_execute_cb(unsigned char opcode) {

    unsigned char result;
    unsigned char *src = registers[opcode % 8];

    if (opcode < 0x40) {
        unsigned char (*op)(unsigned char) = ops_cb[opcode / 8];
        if (opcode % 8 == 6) {
            result = op(read_mmu(HL()));
        } else {
            result = op(*src);
        }
    } else {
        unsigned char (*op)(unsigned char, unsigned char) = ops_cb[opcode / 0x40 + 7];
        if (opcode % 8 == 6) {
            result = op(read_mmu(HL()), (opcode % 0x40) / 8);
        } else {
            result = op(*src, (opcode % 0x40) / 8);
        }
    }

    if (opcode % 8 == 6) {
        write_mmu(HL(), result);
    } else {
        *src = result;
    }
}

void init_ops() {
    opcodes[0x0] = NOP;
    opcodes[0x2] = LD_BC_A;
    opcodes[0x7] = RLCA;
    opcodes[0x8] = LD_a16_SP;
    opcodes[0xA] = LD_A_BC;
    opcodes[0xF] = RRCA;
    opcodes[0x10] = NOP;
    opcodes[0x12] = LD_DE_A;
    opcodes[0x17] = RLA;
    opcodes[0x18] = JR_r8;
    opcodes[0x1A] = LD_A_DE;
    opcodes[0x20] = JR_NZ_r8;
    opcodes[0x1F] = RRA;
    opcodes[0x22] = LDI_HL_A;
    opcodes[0x27] = DAA;
    opcodes[0x2F] = CPL;
    opcodes[0x28] = JR_Z_r8;
    opcodes[0x2A] = LDI_A_HL;
    opcodes[0x30] = JR_NC_r8;
    opcodes[0x32] = LDD_HL_A;
    opcodes[0x37] = SCF;
    opcodes[0x38] = JR_C_r8;
    opcodes[0x3A] = LDD_A_HL;
    opcodes[0x3F] = CCF;
    opcodes[0xC0] = RET_NZ;
    opcodes[0xC2] = JP_NZ_a16;
    opcodes[0xC3] = JP;
    opcodes[0xC4] = CALL_NZ_a16;
    opcodes[0xC8] = RET_Z;
    opcodes[0xC9] = RET;
    opcodes[0xCA] = JP_Z_a16;
    opcodes[0xCC] = CALL_Z_a16;
    opcodes[0xCD] = CALL_a16;
    opcodes[0xD0] = RET_NC;
    opcodes[0xD2] = JP_NC_a16;
    opcodes[0xD4] = CALL_NC_a16;
    opcodes[0xD8] = RET_C;
    opcodes[0xD9] = RETI;
    opcodes[0xDA] = JP_C_a16;
    opcodes[0xDC] = CALL_C_a16;
    opcodes[0xE0] = LDH_n_A;
    opcodes[0xE2] = LD_C_A;
    opcodes[0xE3] = NOP;
    opcodes[0xE8] = ADD_SP_r8;
    opcodes[0xE9] = JP_HL;
    opcodes[0xEA] = LD_a16_A;
    opcodes[0xEC] = NOP;
    opcodes[0xED] = NOP;
    opcodes[0xF0] = LDH_A_n;
    opcodes[0xF2] = LD_A_C;
    opcodes[0xF3] = DI;
    opcodes[0xF4] = NOP;
    opcodes[0xF8] = LD_HL_SP_r8;
    opcodes[0xF9] = LD_SP_HL;
    opcodes[0xFA] = LD_A_a16;
    opcodes[0xFB] = EI;
    opcodes[0xFD] = NOP;

    for (unsigned char i = 0; i < 0x40; i++) {
        if (i % 16 == 1) {
            lenghts[i] = 3;
        } else if (i % 8 == 3) {
            lenghts[i] = 1;
        } else if (i % 16 == 9) {
            lenghts[i] = 1;
        } else if (i % 8 == 4) {
            lenghts[i] = 1;
        } else if (i % 8 == 5) {
            lenghts[i] = 1;
        } else if (i % 8 == 6) {
            lenghts[i] = 2;
        }
    }

    for (unsigned char i = 0x40; i < 0xC0; i++) {
        lenghts[i] = 1;
    }

    for (unsigned short i = 0xC0; i <= 0xFF; i++) {
        if (i % 16 == 1) {
            lenghts[i] = 1;
        } else if (i % 16 == 5) {
            lenghts[i] = 1;
        } else if (i % 8 == 6) {
            lenghts[i] = 2;
        } else if (i % 8 == 7) {
            lenghts[i] = 1;
        }
    }

    lenghts[0x0] = 1;
    lenghts[0x2] = 1;
    lenghts[0x7] = 1;
    lenghts[0x8] = 3;
    lenghts[0xA] = 1;
    lenghts[0xF] = 1;
    lenghts[0x10] = 1;
    lenghts[0x12] = 1;
    lenghts[0x17] = 1;
    lenghts[0x18] = 2;
    lenghts[0x1A] = 1;
    lenghts[0x1F] = 1;
    lenghts[0x20] = 2;
    lenghts[0x22] = 1;
    lenghts[0x27] = 1;
    lenghts[0x28] = 2;
    lenghts[0x2A] = 1;
    lenghts[0x2F] = 1;
    lenghts[0x30] = 2;
    lenghts[0x32] = 1;
    lenghts[0x37] = 1;
    lenghts[0x38] = 2;
    lenghts[0x3A] = 1;
    lenghts[0x3F] = 1;
    lenghts[0xC0] = 1;
    lenghts[0xC2] = 3;
    lenghts[0xC3] = 3;
    lenghts[0xC4] = 3;
    lenghts[0xC8] = 1;
    lenghts[0xC9] = 1;
    lenghts[0xCA] = 3;
    lenghts[0xCC] = 3;
    lenghts[0xCD] = 3;
    lenghts[0xD0] = 1;
    lenghts[0xD2] = 3;
    lenghts[0xD4] = 3;
    lenghts[0xD8] = 1;
    lenghts[0xD9] = 1;
    lenghts[0xDA] = 3;
    lenghts[0xDC] = 3;
    lenghts[0xE0] = 2;
    lenghts[0xE2] = 1;
    lenghts[0xE3] = 1;
    lenghts[0xEC] = 1;
    lenghts[0xE8] = 2;
    lenghts[0xE9] = 1;
    lenghts[0xEA] = 3;
    lenghts[0xED] = 1;
    lenghts[0xF0] = 2;
    lenghts[0xF2] = 2;
    lenghts[0xF3] = 1;
    lenghts[0xF4] = 1;
    lenghts[0xF8] = 2;
    lenghts[0xF9] = 1;
    lenghts[0xFA] = 3;
    lenghts[0xFB] = 1;
    lenghts[0xFD] = 1;
}