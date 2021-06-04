// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_INSTRUCTIONS_H
#define LIBCBOY_INSTRUCTIONS_H

unsigned char NOP();
unsigned char HALT();
void PUSH(unsigned short value);
unsigned short POP();
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
unsigned char RST(unsigned char addr);
unsigned char ADD(unsigned char a, unsigned char b);
unsigned short ADD_HL_n(unsigned short a, unsigned short b);
unsigned char ADD_SP_r8(char value);
unsigned char ADC(unsigned char a, unsigned char b);
unsigned char SUB(unsigned char a, unsigned char b);
unsigned char SBC(unsigned char a, unsigned char b);
unsigned char AND(unsigned char a, unsigned char b);
unsigned char INC(unsigned char reg);
unsigned char DEC(unsigned char reg);
unsigned char OR(unsigned char a, unsigned char b);
unsigned char XOR(unsigned char a, unsigned char b);
unsigned char CP(unsigned char a, unsigned char b);
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
unsigned char LD_A_C();
unsigned char LD_C_A();
unsigned char LD_a16_SP(unsigned short addr);
unsigned char LD_a16_A(unsigned short value);
unsigned char LD_A_a16(unsigned short value);
unsigned char LD_HL_SP_r8(char value);
unsigned char LD_SP_HL();
unsigned char DI();
unsigned char EI();

#endif // LIBCBOY_INSTRUCTIONS_H