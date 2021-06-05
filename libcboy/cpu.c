// SPDX-License-Identifier: GPL-3.0-only

#include "display.h"
#include "gameboy.h"
#include "timer.h"
#include "instructions/cb.h"
#include "instructions/instructions.h"

static const unsigned char (*opcodes[0x100])() = {NOP, LD_BC_d16, LD_BC_A, INC_BC, INC_B, DEC_B, LD_B_d8, RLCA, LD_a16_SP, ADD_HL_BC, LD_A_BC, DEC_BC, INC_C, DEC_C, LD_C_d8, RRCA,
        NOP, LD_DE_d16, LD_DE_A, INC_DE, INC_D, DEC_D, LD_D_d8, RLA, JR_r8, ADD_HL_DE, LD_A_DE, DEC_DE, INC_E, DEC_E, LD_E_d8, RRA,
        JR_NZ_r8, LD_HL_d16, LDI_HL_A, INC_HL, INC_H, DEC_H, LD_H_d8, DAA, JR_Z_r8, ADD_HL_HL, LDI_A_HL, DEC_HL, INC_L, DEC_L, LD_L_d8, CPL,
        JR_NC_r8, LD_SP_d16, LDD_HL_A, INC_SP, INC_HLp, DEC_HLp, LD_HLp_d8, SCF, JR_C_r8, ADD_HL_SP, LDD_A_HL, DEC_SP, INC_A, DEC_A, LD_A_d8, CCF,
        LD_B_B, LD_B_C, LD_B_D, LD_B_E, LD_B_H, LD_B_L, LD_B_HLp, LD_B_A, LD_C_B, LD_C_C, LD_C_D, LD_C_E, LD_C_H, LD_C_L, LD_C_HLp, LD_C_A,
        LD_D_B, LD_D_C, LD_D_D, LD_D_E, LD_D_H, LD_D_L, LD_D_HLp, LD_D_A, LD_E_B, LD_E_C, LD_E_D, LD_E_E, LD_E_H, LD_E_L, LD_E_HLp, LD_E_A,
        LD_H_B, LD_H_C, LD_H_D, LD_H_E, LD_H_H, LD_H_L, LD_H_HLp, LD_H_A, LD_L_B, LD_L_C, LD_L_D, LD_L_E, LD_L_H, LD_L_L, LD_L_HLp, LD_L_A,
        LD_HLp_B, LD_HLp_C, LD_HLp_D, LD_HLp_E, LD_HLp_H, LD_HLp_L, HALT, LD_HLp_A, LD_A_B, LD_A_C, LD_A_D, LD_A_E, LD_A_H, LD_A_L, LD_A_HLp, LD_A_A,
        ADD_B, ADD_C, ADD_D, ADD_E, ADD_H, ADD_L, ADD_HLp, ADD_A, ADC_B, ADC_C, ADC_D, ADC_E, ADC_H, ADC_L, ADC_HLp, ADC_A,
        SUB_B, SUB_C, SUB_D, SUB_E, SUB_H, SUB_L, SUB_HLp, SUB_A, SBC_B, SBC_C, SBC_D, SBC_E, SBC_H, SBC_L, SBC_HLp, SBC_A,
        AND_B, AND_C, AND_D, AND_E, AND_H, AND_L, AND_HLp, AND_A, XOR_B, XOR_C, XOR_D, XOR_E, XOR_H, XOR_L, XOR_HLp, XOR_A,
        OR_B, OR_C, OR_D, OR_E, OR_H, OR_L, OR_HLp, OR_A, CP_B, CP_C, CP_D, CP_E, CP_H, CP_L, CP_HLp, CP_A,
        RET_NZ, POP_BC, JP_NZ_a16, JP, CALL_NZ_a16, PUSH_BC, ADD_d8, RST_0x0, RET_Z, RET, JP_Z_a16, NOP, CALL_Z_a16, CALL_a16, ADC_d8, RST_0x8,
        RET_NC, POP_DE, JP_NC_a16, NOP, CALL_NC_a16, PUSH_DE, SUB_d8, RST_0x10, RET_C, RETI, JP_C_a16, NOP, CALL_C_a16, NOP, SBC_d8, RST_0x18,
        LDH_n_A, POP_HL, LD_Cp_A, NOP, NOP, PUSH_HL, AND_d8, RST_0x20, ADD_SP_r8, JP_HL, LD_a16_A, NOP, NOP, NOP, XOR_d8, RST_0x28,
        LDH_A_n, POP_AF, LD_A_Cp, DI, NOP, PUSH_AF, OR_d8, RST_0x30, LD_HL_SP_r8, LD_SP_HL, LD_A_a16, EI, NOP, NOP, CP_d8, RST_0x38};

static const void (*cb[0x100])() = {RLC_B, RLC_C, RLC_D, RLC_E, RLC_H, RLC_L, RLC_HL, RLC_A, RRC_B, RRC_C, RRC_D, RRC_E, RRC_H, RRC_L, RRC_HL, RRC_A,
        RL_B, RL_C, RL_D, RL_E, RL_H, RL_L, RL_HL, RL_A, RR_B, RR_C, RR_D, RR_E, RR_H, RR_L, RR_HL, RR_A,
        SLA_B, SLA_C, SLA_D, SLA_E, SLA_H, SLA_L, SLA_HL, SLA_A, SRA_B, SRA_C, SRA_D, SRA_E, SRA_H, SRA_L, SRA_HL, SRA_A,
        SWAP_B, SWAP_C, SWAP_D, SWAP_E, SWAP_H, SWAP_L, SWAP_HL, SWAP_A, SRL_B, SRL_C, SRL_D, SRL_E, SRL_H, SRL_L, SRL_HL, SRL_A,
        BIT_0_B, BIT_0_C, BIT_0_D, BIT_0_E, BIT_0_H, BIT_0_L, BIT_0_HL, BIT_0_A, BIT_1_B, BIT_1_C, BIT_1_D, BIT_1_E, BIT_1_H, BIT_1_L, BIT_1_HL, BIT_1_A,
        BIT_2_B, BIT_2_C, BIT_2_D, BIT_2_E, BIT_2_H, BIT_2_L, BIT_2_HL, BIT_2_A, BIT_3_B, BIT_3_C, BIT_3_D, BIT_3_E, BIT_3_H, BIT_3_L, BIT_3_HL, BIT_3_A,
        BIT_4_B, BIT_4_C, BIT_4_D, BIT_4_E, BIT_4_H, BIT_4_L, BIT_4_HL, BIT_4_A, BIT_5_B, BIT_5_C, BIT_5_D, BIT_5_E, BIT_5_H, BIT_5_L, BIT_5_HL, BIT_5_A,
        BIT_6_B, BIT_6_C, BIT_6_D, BIT_6_E, BIT_6_H, BIT_6_L, BIT_6_HL, BIT_6_A, BIT_7_B, BIT_7_C, BIT_7_D, BIT_7_E, BIT_7_H, BIT_7_L, BIT_7_HL, BIT_7_A,
        RES_0_B, RES_0_C, RES_0_D, RES_0_E, RES_0_H, RES_0_L, RES_0_HL, RES_0_A, RES_1_B, RES_1_C, RES_1_D, RES_1_E, RES_1_H, RES_1_L, RES_1_HL, RES_1_A,
        RES_2_B, RES_2_C, RES_2_D, RES_2_E, RES_2_H, RES_2_L, RES_2_HL, RES_2_A, RES_3_B, RES_3_C, RES_3_D, RES_3_E, RES_3_H, RES_3_L, RES_3_HL, RES_3_A,
        RES_4_B, RES_4_C, RES_4_D, RES_4_E, RES_4_H, RES_4_L, RES_4_HL, RES_4_A, RES_5_B, RES_5_C, RES_5_D, RES_5_E, RES_5_H, RES_5_L, RES_5_HL, RES_5_A,
        RES_6_B, RES_6_C, RES_6_D, RES_6_E, RES_6_H, RES_6_L, RES_6_HL, RES_6_A, RES_7_B, RES_7_C, RES_7_D, RES_7_E, RES_7_H, RES_7_L, RES_7_HL, RES_7_A,
        SET_0_B, SET_0_C, SET_0_D, SET_0_E, SET_0_H, SET_0_L, SET_0_HL, SET_0_A, SET_1_B, SET_1_C, SET_1_D, SET_1_E, SET_1_H, SET_1_L, SET_1_HL, SET_1_A,
        SET_2_B, SET_2_C, SET_2_D, SET_2_E, SET_2_H, SET_2_L, SET_2_HL, SET_2_A, SET_3_B, SET_3_C, SET_3_D, SET_3_E, SET_3_H, SET_3_L, SET_3_HL, SET_3_A,
        SET_4_B, SET_4_C, SET_4_D, SET_4_E, SET_4_H, SET_4_L, SET_4_HL, SET_4_A, SET_5_B, SET_5_C, SET_5_D, SET_5_E, SET_5_H, SET_5_L, SET_5_HL, SET_5_A,
        SET_6_B, SET_6_C, SET_6_D, SET_6_E, SET_6_H, SET_6_L, SET_6_HL, SET_6_A, SET_7_B, SET_7_C, SET_7_D, SET_7_E, SET_7_H, SET_7_L, SET_7_HL, SET_7_A};

static const unsigned char lengths[0x100] = {1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1,
                                            1, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
                                            2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
                                            2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, -1, 3, 3, 2, 1,
                                            1, 1, 3, -1, 3, 1, 2, 1, 1, 1, 3, -1, 3, -1, 2, 1,
                                            2, 1, 1, -1, -1, 1, 2, 1, 2, 1, 3, -1, -1, -1, 2, 1,
                                            2, 1, 1, 1, -1, 1, 2, 1, 2, 1, 3, 1, -1, -1, 2, 1};

static unsigned char fetch() {
    unsigned char value = read_mmu(gameboy.cpu.PC);
    gameboy.cpu.PC += 1;
    return value;
}

/*
 * FFFF - IE - Interrupt Enable (R/W)
 *   Bit 0: V-Blank  Interrupt Enable  (INT 40h)  (1=Enable)
 *   Bit 1: LCD STAT Interrupt Enable  (INT 48h)  (1=Enable)
 *   Bit 2: Timer    Interrupt Enable  (INT 50h)  (1=Enable)
 *   Bit 3: Serial   Interrupt Enable  (INT 58h)  (1=Enable)
 *   Bit 4: Joypad   Interrupt Enable  (INT 60h)  (1=Enable)
 *
 * FF0F - IF - Interrupt Flag (R/W)
 *   Bit 0: V-Blank  Interrupt Request (INT 40h)  (1=Request)
 *   Bit 1: LCD STAT Interrupt Request (INT 48h)  (1=Request)
 *   Bit 2: Timer    Interrupt Request (INT 50h)  (1=Request)
 *   Bit 3: Serial   Interrupt Request (INT 58h)  (1=Request)
 *   Bit 4: Joypad   Interrupt Request (INT 60h)  (1=Request)
 */
static void check_interrupt() {

    unsigned char interrupt_enable = read_mmu(0xFFFF);
    unsigned char interrupt_flag = read_mmu(0xFF0F);

    for (unsigned char i = 0; i < 5; i++) {
        // check for interrupt enable and interrupt request being set
        if (interrupt_enable >> i & 1 && interrupt_flag >> i & 1) {

            if (gameboy.cpu.halt)
                gameboy.cpu.halt = false;

            if (!gameboy.cpu.ime)
                return;

            // reset corresponding bit
            write_mmu(0xFF0F, read_mmu(0xFF0F) & ~(1 << i));

            // disable IME
            gameboy.cpu.ime = false;

            // push PC to stack
            write_mmu(gameboy.cpu.SP - 1, gameboy.cpu.PC >> 8);
            write_mmu(gameboy.cpu.SP - 2, gameboy.cpu.PC & 0xFF);
            gameboy.cpu.SP -= 2;

            // call corresponding interrupt address
            gameboy.cpu.PC = 0x40 + i * 8;
            gameboy.cpu.halt = false;
        }
    }
}

static unsigned char next_instruction() {

    check_interrupt();

    if (gameboy.cpu.halt)
        return 12;

    unsigned char opcode = fetch();

    if (opcode == 0xCB) {
        opcode = fetch();
        cb[opcode]();
        return 8;
    }

    if (lengths[opcode] == 1)
        return opcodes[opcode]();

    unsigned char v1 = fetch();

    if (lengths[opcode] == 2)
        return ((int (*)(unsigned char))opcodes[opcode])(v1);

    unsigned char v2 = fetch();
    unsigned short arg = (v2 << 8) + v1;

    return ((int (*)(unsigned short))opcodes[opcode])(arg);
}

static void next_instructions(int cycles) {
    unsigned char cur_cycles;
    while (cycles > 0) {
        cur_cycles = next_instruction();
        timer(cur_cycles);
        cycles -= cur_cycles;
    }
}

Frame next_frame() {
    while (lcd_display_enable() == false) {
        set_mode(0);
        write_mmu(0xFF44, 0);
        for (unsigned char i = 0; i < 154; i++) {
            next_instructions(456);
        }
    }

    // Resolution - 160x144 (20x18 tiles)
    // 144 vertical lines
    for (unsigned char i = 0; i < 144; i++) {
        set_ly(i);

        // MODE 2
        // 77-83 clks
        set_mode(2);
        next_instructions(80);

        // MODE 3
        // 169-175 clks
        set_mode(3);
        next_instructions(172);
        set_params(i);

        // MODE 0
        // 201-207 clks
        set_mode(0);
        next_instructions(204);
    }

    set_vblank();
    draw();

    for (unsigned char i = 144; i <= 154; i++) {
        set_ly(i);
        // MODE 1
        // 4560 clks
        set_mode(1);
        next_instructions(456);
    }

    return gameboy.framebuffer;
}

unsigned short AF() { return (gameboy.cpu.A << 8) + gameboy.cpu.F; }

void set_AF(unsigned short value) {
    gameboy.cpu.A = value >> 8 & 0xFF;
    gameboy.cpu.F = value & 0xF0;
}

unsigned short BC() { return (gameboy.cpu.B << 8) + gameboy.cpu.C; }

void set_BC(unsigned short value) {
    gameboy.cpu.B = value >> 8 & 0xFF;
    gameboy.cpu.C = value & 0xFF;
}

unsigned short DE() { return (gameboy.cpu.D << 8) + gameboy.cpu.E; }

void set_DE(unsigned short value) {
    gameboy.cpu.D = value >> 8 & 0xFF;
    gameboy.cpu.E = value & 0xFF;
}

unsigned short HL() { return (gameboy.cpu.H << 8) + gameboy.cpu.L; }

void set_HL(unsigned short value) {
    gameboy.cpu.H = value >> 8 & 0xFF;
    gameboy.cpu.L = value & 0xFF;
}

unsigned short SP() { return gameboy.cpu.SP; }

void set_SP(unsigned short value) { gameboy.cpu.SP = value; }

bool get_bit(unsigned char i) { return gameboy.cpu.F >> i & 1; }

void set_bit(unsigned char i, bool set) {
    if (set) {
        gameboy.cpu.F |= 1 << i;
    } else {
        gameboy.cpu.F &= ~(1 << i);
    }
}

bool flag_Z() { return get_bit(7); }

void set_flag_Z(bool set) { set_bit(7, set); }

bool flag_N() { return get_bit(6); }

void set_flag_N(bool set) { set_bit(6, set); }

bool flag_H() { return get_bit(5); }

void set_flag_H(bool set) { set_bit(5, set); }

bool flag_C() { return get_bit(4); }

void set_flag_C(bool set) { set_bit(4, set); }