// SPDX-License-Identifier: GPL-3.0-only

#include "display.h"
#include "gameboy.h"
#include "mmu.h"
#include "timer.h"
#include "instructions/cb.h"
#include "instructions/instructions.h"

Cpu cpu =  {.SP = 0xFFFF,
            .ime = true,
            .halt = false};

static unsigned char (*opcodes[0x100])() = {(void*) NOP, (void*) LD_BC_d16, (void*) LD_BC_A, (void*) INC_BC, (void*) INC_B, (void*) DEC_B, (void*) LD_B_d8, (void*) RLCA, (void*) LD_a16_SP, (void*) ADD_HL_BC, (void*) LD_A_BC, (void*) DEC_BC, (void*) INC_C, (void*) DEC_C, (void*) LD_C_d8, (void*) RRCA, (void*)
        NOP, (void*) LD_DE_d16, (void*) LD_DE_A, (void*) INC_DE, (void*) INC_D, (void*) DEC_D, (void*) LD_D_d8, (void*) RLA, (void*) JR_r8, (void*) ADD_HL_DE, (void*) LD_A_DE, (void*) DEC_DE, (void*) INC_E, (void*) DEC_E, (void*) LD_E_d8, (void*) RRA, (void*)
        JR_NZ_r8, (void*) LD_HL_d16, (void*) LDI_HL_A, (void*) INC_HL, (void*) INC_H, (void*) DEC_H, (void*) LD_H_d8, (void*) DAA, (void*) JR_Z_r8, (void*) ADD_HL_HL, (void*) LDI_A_HL, (void*) DEC_HL, (void*) INC_L, (void*) DEC_L, (void*) LD_L_d8, (void*) CPL, (void*)
        JR_NC_r8, (void*) LD_SP_d16, (void*) LDD_HL_A, (void*) INC_SP, (void*) INC_HLp, (void*) DEC_HLp, (void*) LD_HLp_d8, (void*) SCF, (void*) JR_C_r8, (void*) ADD_HL_SP, (void*) LDD_A_HL, (void*) DEC_SP, (void*) INC_A, (void*) DEC_A, (void*) LD_A_d8, (void*) CCF, (void*)
        LD_B_B, (void*) LD_B_C, (void*) LD_B_D, (void*) LD_B_E, (void*) LD_B_H, (void*) LD_B_L, (void*) LD_B_HLp, (void*) LD_B_A, (void*) LD_C_B, (void*) LD_C_C, (void*) LD_C_D, (void*) LD_C_E, (void*) LD_C_H, (void*) LD_C_L, (void*) LD_C_HLp, (void*) LD_C_A, (void*)
        LD_D_B, (void*) LD_D_C, (void*) LD_D_D, (void*) LD_D_E, (void*) LD_D_H, (void*) LD_D_L, (void*) LD_D_HLp, (void*) LD_D_A, (void*) LD_E_B, (void*) LD_E_C, (void*) LD_E_D, (void*) LD_E_E, (void*) LD_E_H, (void*) LD_E_L, (void*) LD_E_HLp, (void*) LD_E_A, (void*)
        LD_H_B, (void*) LD_H_C, (void*) LD_H_D, (void*) LD_H_E, (void*) LD_H_H, (void*) LD_H_L, (void*) LD_H_HLp, (void*) LD_H_A, (void*) LD_L_B, (void*) LD_L_C, (void*) LD_L_D, (void*) LD_L_E, (void*) LD_L_H, (void*) LD_L_L, (void*) LD_L_HLp, (void*) LD_L_A, (void*)
        LD_HLp_B, (void*) LD_HLp_C, (void*) LD_HLp_D, (void*) LD_HLp_E, (void*) LD_HLp_H, (void*) LD_HLp_L, (void*) HALT, (void*) LD_HLp_A, (void*) LD_A_B, (void*) LD_A_C, (void*) LD_A_D, (void*) LD_A_E, (void*) LD_A_H, (void*) LD_A_L, (void*) LD_A_HLp, (void*) LD_A_A, (void*)
        ADD_B, (void*) ADD_C, (void*) ADD_D, (void*) ADD_E, (void*) ADD_H, (void*) ADD_L, (void*) ADD_HLp, (void*) ADD_A, (void*) ADC_B, (void*) ADC_C, (void*) ADC_D, (void*) ADC_E, (void*) ADC_H, (void*) ADC_L, (void*) ADC_HLp, (void*) ADC_A, (void*)
        SUB_B, (void*) SUB_C, (void*) SUB_D, (void*) SUB_E, (void*) SUB_H, (void*) SUB_L, (void*) SUB_HLp, (void*) SUB_A, (void*) SBC_B, (void*) SBC_C, (void*) SBC_D, (void*) SBC_E, (void*) SBC_H, (void*) SBC_L, (void*) SBC_HLp, (void*) SBC_A, (void*)
        AND_B, (void*) AND_C, (void*) AND_D, (void*) AND_E, (void*) AND_H, (void*) AND_L, (void*) AND_HLp, (void*) AND_A, (void*) XOR_B, (void*) XOR_C, (void*) XOR_D, (void*) XOR_E, (void*) XOR_H, (void*) XOR_L, (void*) XOR_HLp, (void*) XOR_A, (void*)
        OR_B, (void*) OR_C, (void*) OR_D, (void*) OR_E, (void*) OR_H, (void*) OR_L, (void*) OR_HLp, (void*) OR_A, (void*) CP_B, (void*) CP_C, (void*) CP_D, (void*) CP_E, (void*) CP_H, (void*) CP_L, (void*) CP_HLp, (void*) CP_A, (void*)
        RET_NZ, (void*) POP_BC, (void*) JP_NZ_a16, (void*) JP, (void*) CALL_NZ_a16, (void*) PUSH_BC, (void*) ADD_d8, (void*) RST_0x0, (void*) RET_Z, (void*) RET, (void*) JP_Z_a16, (void*) NOP, (void*) CALL_Z_a16, (void*) CALL_a16, (void*) ADC_d8, (void*) RST_0x8, (void*)
        RET_NC, (void*) POP_DE, (void*) JP_NC_a16, (void*) NOP, (void*) CALL_NC_a16, (void*) PUSH_DE, (void*) SUB_d8, (void*) RST_0x10, (void*) RET_C, (void*) RETI, (void*) JP_C_a16, (void*) NOP, (void*) CALL_C_a16, (void*) NOP, (void*) SBC_d8, (void*) RST_0x18, (void*)
        LDH_n_A, (void*) POP_HL, (void*) LD_Cp_A, (void*) NOP, (void*) NOP, (void*) PUSH_HL, (void*) AND_d8, (void*) RST_0x20, (void*) ADD_SP_r8, (void*) JP_HL, (void*) LD_a16_A, (void*) NOP, (void*) NOP, (void*) NOP, (void*) XOR_d8, (void*) RST_0x28, (void*)
        LDH_A_n, (void*) POP_AF, (void*) LD_A_Cp, (void*) DI, (void*) NOP, (void*) PUSH_AF, (void*) OR_d8, (void*) RST_0x30, (void*) LD_HL_SP_r8, (void*) LD_SP_HL, (void*) LD_A_a16, (void*) EI, (void*) NOP, (void*) NOP, (void*) CP_d8, (void*) RST_0x38};

static void (*cb[0x100])() = {RLC_B, RLC_C, RLC_D, RLC_E, RLC_H, RLC_L, RLC_HL, RLC_A, RRC_B, RRC_C, RRC_D, RRC_E, RRC_H, RRC_L, RRC_HL, RRC_A,
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
    unsigned char value = read_mmu(cpu.PC);
    cpu.PC += 1;
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

            if (cpu.halt)
                cpu.halt = false;

            if (!cpu.ime)
                return;

            // reset corresponding bit
            write_mmu(0xFF0F, read_mmu(0xFF0F) & ~(1 << i));

            // disable IME
            cpu.ime = false;

            // push PC to stack
            write_mmu(cpu.SP - 1, cpu.PC >> 8);
            write_mmu(cpu.SP - 2, cpu.PC & 0xFF);
            cpu.SP -= 2;

            // call corresponding interrupt address
            cpu.PC = 0x40 + i * 8;
            cpu.halt = false;
        }
    }
}

static unsigned char next_instruction() {

    check_interrupt();

    if (cpu.halt)
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