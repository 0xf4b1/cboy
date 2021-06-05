// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_INSTRUCTIONS_H
#define LIBCBOY_INSTRUCTIONS_H

unsigned char NOP();
unsigned char HALT();
unsigned char JP(unsigned short addr);
unsigned char JP_NZ_a16(unsigned short value);
unsigned char JP_NC_a16(unsigned short value);
unsigned char JP_C_a16(unsigned short value);
unsigned char JP_Z_a16(unsigned short value);
unsigned char JP_HL();
unsigned char JR_C_r8(unsigned char value);
unsigned char JR_NC_r8(unsigned char value);
unsigned char JR_NZ_r8(unsigned char value);
unsigned char JR_Z_r8(unsigned char value);
unsigned char JR_r8(unsigned char value);
unsigned char CALL_a16(unsigned short addr);
unsigned char CALL_Z_a16(unsigned short addr);
unsigned char CALL_NZ_a16(unsigned short addr);
unsigned char CALL_NC_a16(unsigned short addr);
unsigned char CALL_C_a16(unsigned short addr);
unsigned char RET();
unsigned char RET_C();
unsigned char RET_NC();
unsigned char RET_Z();
unsigned char RET_NZ();
unsigned char RETI();
unsigned char ADD_SP_r8(char value);
unsigned char RLCA();
unsigned char RRCA();
unsigned char RLA();
unsigned char RRA();
unsigned char DAA();
unsigned char CPL();
unsigned char SCF();
unsigned char CCF();
unsigned char LD_A_BC();
unsigned char LD_BC_A();
unsigned char LD_A_DE();
unsigned char LD_DE_A();
unsigned char LDI_HL_A();
unsigned char LDI_A_HL();
unsigned char LDD_HL_A();
unsigned char LDD_A_HL();
unsigned char LDH_n_A(unsigned char addr);
unsigned char LDH_A_n(unsigned char addr);
unsigned char LD_A_Cp();
unsigned char LD_Cp_A();
unsigned char LD_a16_SP(unsigned short addr);
unsigned char LD_a16_A(unsigned short value);
unsigned char LD_A_a16(unsigned short value);
unsigned char LD_HL_SP_r8(char value);
unsigned char LD_SP_HL();
unsigned char DI();
unsigned char EI();
unsigned char INC_HLp();
unsigned char DEC_HLp();
unsigned char LD_HLp_d8();

#define DEFINE_OP_REG(OP, REG) \
        unsigned char OP ## _ ## REG();

#define DEFINE_OP_REG1_REG2(OP, REG1, REG2) \
        unsigned char OP ## _ ## REG1 ## _ ## REG2();

#define DEFINE_r16(REG) \
        DEFINE_OP_REG(INC, REG) \
        DEFINE_OP_REG(DEC, REG) \
        DEFINE_OP_REG1_REG2(ADD, HL, REG) \
        DEFINE_OP_REG1_REG2(LD, REG, d16)

DEFINE_r16(BC)
DEFINE_r16(DE)
DEFINE_r16(HL)
DEFINE_r16(SP)

#define DEFINE_STACK(REG) \
        DEFINE_OP_REG(POP, REG) \
        DEFINE_OP_REG(PUSH, REG)

DEFINE_STACK(BC)
DEFINE_STACK(DE)
DEFINE_STACK(HL)
DEFINE_STACK(AF)

#define DEFINE_r8(REG) \
        DEFINE_OP_REG(INC, REG) \
        DEFINE_OP_REG(DEC, REG) \
        DEFINE_OP_REG1_REG2(LD, REG, d8) \
        DEFINE_OP_REG1_REG2(LD, REG, B) \
        DEFINE_OP_REG1_REG2(LD, REG, C) \
        DEFINE_OP_REG1_REG2(LD, REG, D) \
        DEFINE_OP_REG1_REG2(LD, REG, E) \
        DEFINE_OP_REG1_REG2(LD, REG, H) \
        DEFINE_OP_REG1_REG2(LD, REG, L) \
        DEFINE_OP_REG1_REG2(LD, REG, HLp) \
        DEFINE_OP_REG1_REG2(LD, HLp, REG) \
        DEFINE_OP_REG1_REG2(LD, REG, A) \
        DEFINE_OP_REG(ADD, REG) \
        DEFINE_OP_REG(ADC, REG) \
        DEFINE_OP_REG(SUB, REG) \
        DEFINE_OP_REG(SBC, REG) \
        DEFINE_OP_REG(AND, REG) \
        DEFINE_OP_REG(XOR, REG) \
        DEFINE_OP_REG(OR, REG) \
        DEFINE_OP_REG(CP, REG)

DEFINE_r8(B)
DEFINE_r8(C)
DEFINE_r8(D)
DEFINE_r8(E)
DEFINE_r8(H)
DEFINE_r8(L)
DEFINE_r8(A)

#define DEFINE_OP(OP) \
        DEFINE_OP_REG(OP, HLp) \
        DEFINE_OP_REG(OP, d8)

DEFINE_OP(ADD)
DEFINE_OP(ADC)
DEFINE_OP(SUB)
DEFINE_OP(SBC)
DEFINE_OP(AND)
DEFINE_OP(XOR)
DEFINE_OP(OR)
DEFINE_OP(CP)

DEFINE_OP_REG(RST, 0x0)
DEFINE_OP_REG(RST, 0x8)
DEFINE_OP_REG(RST, 0x10)
DEFINE_OP_REG(RST, 0x18)
DEFINE_OP_REG(RST, 0x20)
DEFINE_OP_REG(RST, 0x28)
DEFINE_OP_REG(RST, 0x30)
DEFINE_OP_REG(RST, 0x38)

#endif // LIBCBOY_INSTRUCTIONS_H