// SPDX-License-Identifier: GPL-3.0-only

#include "../gameboy.h"

/*
 * NOP
 * Description:
 * No operation.
 */
unsigned char NOP() { return 4; }

/*
 * PUSH nn
 * Description:
 * Push register pair nn onto stack.
 * Decrement Stack Pointer (SP) twice.
 * Use with:
 * nn = AF,BC,DE,HL
 */
void PUSH(unsigned short value) {
    write_mmu(gameboy.cpu.SP - 1, value >> 8);
    write_mmu(gameboy.cpu.SP - 2, value & 0xFF);
    gameboy.cpu.SP -= 2;
}

/*
 * POP nn
 * Description:
 * Pop two bytes off stack into register pair nn.
 * Increment Stack Pointer (SP) twice.
 * Use with:
 * nn = AF,BC,DE,HL
 */
unsigned short POP() {
    unsigned short value = (read_mmu(gameboy.cpu.SP + 1) << 8 | read_mmu(gameboy.cpu.SP)) & 0xFFFF;
    gameboy.cpu.SP += 2;
    return value;
}

/*
 * JP nn
 * Description:
 * Jump to address nn.
 * Use with:
 * nn = two byte immediate value. (LS byte first.)
 */
unsigned char JP(unsigned short addr) {
    gameboy.cpu.PC = addr;
    return 16;
}

unsigned char JP_NZ_a16(unsigned short value) {
    if (!flag_Z()) {
        gameboy.cpu.PC = value;
        return 16;
    }
    return 12;
}

unsigned char JP_NC_a16(unsigned short value) {
    if (!flag_C()) {
        gameboy.cpu.PC = value;
        return 16;
    }
    return 12;
}

unsigned char JP_C_a16(unsigned short value) {
    if (flag_C()) {
        gameboy.cpu.PC = value;
        return 16;
    }
    return 12;
}

unsigned char JP_Z_a16(unsigned short value) {
    if (flag_Z()) {
        gameboy.cpu.PC = value;
        return 16;
    }
    return 12;
}

/*
 * JP (HL)
 * Description:
 * Jump to address contained in HL.
 */
unsigned char JP_HL() {
    gameboy.cpu.PC = HL();
    return 4;
}

/*
 * JR n
 * Description:
 * Add n to current address and jump to it.
 * Use with:
 * n = one byte signed immediate value
 */
void JR(unsigned char value) { gameboy.cpu.PC += (value ^ 0x80) - 0x80; }

unsigned char JR_C_r8(unsigned char value) {
    if (flag_C()) {
        JR(value);
        return 12;
    }
    return 8;
}

unsigned char JR_NC_r8(unsigned char value) {
    if (flag_C() == 0) {
        JR(value);
        return 12;
    }
    return 8;
}

unsigned char JR_NZ_r8(unsigned char value) {
    if (flag_Z() == 0) {
        JR(value);
        return 12;
    }
    return 8;
}

unsigned char JR_Z_r8(unsigned char value) {
    if (flag_Z()) {
        JR(value);
        return 12;
    }
    return 8;
}

unsigned char JR_r8(unsigned char value) {
    JR(value);
    return 12;
}

/*
 * CALL nn
 * Description:
 * Push address of next instruction onto stack and then
 * jump to address nn.
 * Use with:
 * nn = two byte immediate value. (LS byte first.)
 */
unsigned char CALL_a16(unsigned short addr) {
    PUSH(gameboy.cpu.PC);
    gameboy.cpu.PC = addr;
    return 24;
}

unsigned char CALL_Z_a16(unsigned short addr) {
    if (flag_Z()) {
        return CALL_a16(addr);
    }
    return 12;
}

unsigned char CALL_NZ_a16(unsigned short addr) {
    if (!flag_Z()) {
        return CALL_a16(addr);
    }
    return 12;
}

unsigned char CALL_NC_a16(unsigned short addr) {
    if (!flag_C()) {
        return CALL_a16(addr);
    }
    return 12;
}

unsigned char CALL_C_a16(unsigned short addr) {
    if (flag_C()) {
        return CALL_a16(addr);
    }
    return 12;
}

/*
 * RET
 * Description:
 * Pop two bytes from stack & jump to that address.
 */
unsigned char RET() {
    gameboy.cpu.PC = POP();
    return 16;
}

unsigned char RET_C() {
    if (flag_C()) {
        gameboy.cpu.PC = POP();
        return 20;
    }
    return 8;
}

unsigned char RET_NC() {
    if (!flag_C()) {
        gameboy.cpu.PC = POP();
        return 20;
    }
    return 8;
}

unsigned char RET_Z() {
    if (flag_Z()) {
        gameboy.cpu.PC = POP();
        return 20;
    }
    return 8;
}

unsigned char RET_NZ() {
    if (!flag_Z()) {
        gameboy.cpu.PC = POP();
        return 20;
    }
    return 8;
}

/*
 * RETI
 * Description:
 * Pop two bytes from stack & jump to that address then
 * enable interrupts.
 */
unsigned char RETI() {
    gameboy.cpu.PC = POP();
    gameboy.cpu.ime = true;
    return 16;
}

unsigned char RST(unsigned char addr) {
    PUSH(gameboy.cpu.PC);
    gameboy.cpu.PC = addr;
    return 16;
}

/*
 * ADD A,n
 * Description:
 * Add n to A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
unsigned char ADD(unsigned char a, unsigned char b) {
    unsigned char res = (a + b) & 0xFF;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H((a & 0xF) + (b & 0xF) > 0xF);
    set_flag_C(a + b > 0xFF);
    return res;
}

/*
 * ADD HL,n
 * Description:
 * Add n to HL.
 * Use with:
 * n = BC,DE,HL,SP
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Set if carry from bit 11.
 * C - Set if carry from bit 15.
 */
unsigned short ADD_HL_n(unsigned short a, unsigned short b) {
    unsigned short res = a + b & 0xFFFF;
    set_flag_N(false);
    set_flag_H((a & 0xFFF) + (b & 0xFFF) > 0xFFF);
    set_flag_C(a + b > 0xFFFF);
    return res;
}

/*
 * ADD SP,n
 * Description:
 * Add n to Stack Pointer (SP).
 * Use with:
 * n = one byte signed immediate value (#).
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Set or reset according to operation.
 * C - Set or reset according to operation.
 */
unsigned char ADD_SP_r8(char value) {
    unsigned short res = (gameboy.cpu.SP + value) & 0xFFFF;
    set_flag_Z(false);
    set_flag_N(false);
    set_flag_H((gameboy.cpu.SP & 0xF) + (value & 0xF) > 0xF);
    set_flag_C((gameboy.cpu.SP & 0xFF) + (value & 0xFF) > 0xFF);
    gameboy.cpu.SP = res;
    return 16;
}

/*
 * ADC A,n
 * Description:
 * Add n + Carry flag to A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Set if carry from bit 7.
 */
unsigned char ADC(unsigned char a, unsigned char b) {
    unsigned char res = (a + b + flag_C()) & 0xFF;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H((a & 0xF) + (b & 0xF) + flag_C() > 0xF);
    set_flag_C(a + b + flag_C() > 0xFF);
    return res;
}

/*
 * SUB n
 * Description:
 * Subtract n from A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
unsigned char SUB(unsigned char a, unsigned char b) {
    unsigned char res = (a - b) & 0xFF;
    set_flag_Z(res == 0);
    set_flag_N(true);
    set_flag_H((a & 0xF) < (b & 0xF));
    set_flag_C(a < b);
    return res;
}

/*
 * SBC A,n
 * Description:
 * Subtract n + Carry flag from A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set if no borrow.
 */
unsigned char SBC(unsigned char a, unsigned char b) {
    unsigned char res = (a - b - flag_C()) & 0xFF;
    set_flag_Z(res == 0);
    set_flag_N(true);
    set_flag_H((a & 0xF) < (b & 0xF) + flag_C());
    set_flag_C(a < b + flag_C());
    return res;
}

/*
 * AND n
 * Description:
 * Logically AND n with A, result in A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set.
 * C - Reset.
 */
unsigned char AND(unsigned char a, unsigned char b) {
    unsigned char res = a & b;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H(true);
    set_flag_C(false);
    return res;
}

/*
 * INC n
 * Description:
 * Increment register n.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Set if carry from bit 3.
 * C - Not affected.
 */
unsigned char INC(unsigned char reg) {
    set_flag_H((reg & 0xF) + 1 & 0x10);
    reg = (reg + 1) & 0xFF;
    set_flag_Z(reg == 0);
    set_flag_N(false);
    return reg;
}

/*
 * DEC n
 * Description:
 * Decrement register n.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Not affected.
 */
unsigned char DEC(unsigned char reg) {
    set_flag_H((reg & 0xF) - 1 < 0);
    reg = (reg - 1) & 0xFF;
    set_flag_Z(reg == 0);
    set_flag_N(true);
    return reg;
}

/*
 * OR n
 * Description:
 * Logical OR n with register A, result in A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
unsigned char OR(unsigned char a, unsigned char b) {
    unsigned char res = a | b;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(false);
    return res;
}

/*
 * XOR n
 * Description:
 * Logical exclusive OR n with register A, result in A.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
unsigned char XOR(unsigned char a, unsigned char b) {
    unsigned char res = a ^ b;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(false);
    return res;
}

/*
 * CP n
 * Description:
 * Compare A with n. This is basically an A - n
 * subtraction instruction but the results are thrown
 * away.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL),#
 * Flags affected:
 * Z - Set if result is zero. (Set if A = n.)
 * N - Set.
 * H - Set if no borrow from bit 4.
 * C - Set for no borrow. (Set if A < n.)
 */
unsigned char CP(unsigned char a, unsigned char b) {
    unsigned char res = a - b;
    set_flag_Z(res == 0);
    set_flag_N(true);
    set_flag_H((a & 0xF) < (b & 0xF));
    set_flag_C(a < b);
    return a;
}

/*
 * RLCA
 * Description:
 * Rotate A left. Old bit 7 to Carry flag.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
unsigned char RLCA() {
    bool c = gameboy.cpu.A >> 7 & 1;
    gameboy.cpu.A = (gameboy.cpu.A << 1 | c) & 0xFF;
    set_flag_Z(false);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return 4;
}

/*
 * RRCA
 * Description:
 * Rotate A right. Old bit 0 to Carry flag.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
unsigned char RRCA() {
    bool c = gameboy.cpu.A & 1;
    gameboy.cpu.A = ((gameboy.cpu.A >> 1) | c << 7) & 0xFF;
    set_flag_Z(false);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return 4;
}

/*
 * RLA
 * Description:
 * Rotate A left through Carry flag.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
unsigned char RLA() {
    bool c = (gameboy.cpu.A >> 7) & 1;
    gameboy.cpu.A = ((gameboy.cpu.A << 1) | flag_C()) & 0xFF;
    set_flag_Z(false);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return 4;
}

/*
 * RRA
 * Description:
 * Rotate A right through Carry flag.
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
unsigned char RRA() {
    unsigned char c = gameboy.cpu.A & 1;
    gameboy.cpu.A = ((gameboy.cpu.A >> 1) | (flag_C() << 7)) & 0xFF;
    set_flag_Z(false);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return 4;
}

/*
 * DAA
 * Description:
 * Decimal adjust register A.
 * This instruction adjusts register A so that the
 * correct representation of Binary Coded Decimal (BCD)
 * is obtained.
 * Flags affected:
 * Z - Set if register A is zero.
 * N - Not affected.
 * H - Reset.
 * C - Set or reset according to operation.
 */
unsigned char DAA() {
    unsigned char t = gameboy.cpu.A;
    unsigned char corr = 0;
    if (flag_H())
        corr |= 0x06;
    if (flag_C())
        corr |= 0x60;
    if (flag_N())
        t -= corr;
    else {
        if ((t & 0x0F) > 0x09)
            corr |= 0x06;
        if (t > 0x99)
            corr |= 0x60;
        t += corr;
    }
    set_flag_Z((t & 0xFF) == 0);
    set_flag_H(false);
    set_flag_C((corr & 0x60) != 0);
    gameboy.cpu.A = t & 0xFF;
    return 4;
}

/*
 * CPL
 * Description:
 * Complement A register. (Flip all bits.)
 * Flags affected:
 * Z - Not affected.
 * N - Set.
 * H - Set.
 * C - Not affected.
 */
unsigned char CPL() {
    gameboy.cpu.A = ~gameboy.cpu.A & 0xFF;
    set_flag_N(true);
    set_flag_H(true);
    return 4;
}

/*
 * SCF
 * Description:
 * Set Carry flag.
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Reset.
 * C - Set.
 */
unsigned char SCF() {
    set_flag_C(true);
    set_flag_N(false);
    set_flag_H(false);
    return 4;
}

/*
 * CCF
 * Description:
 * Complement carry flag.
 * If C flag is set, then reset it.
 * If C flag is reset, then set it.
 * Flags affected:
 * Z - Not affected.
 * N - Reset.
 * H - Reset.
 * C - Complemented.
 */
unsigned char CCF() {
    set_flag_C(!flag_C());
    set_flag_N(false);
    set_flag_H(false);
    return 4;
}

unsigned char LD_A_BC() {
    gameboy.cpu.A = read_mmu(BC());
    return 8;
}

unsigned char LD_BC_A() {
    write_mmu(BC(), gameboy.cpu.A);
    return 8;
}

unsigned char LD_A_DE() {
    gameboy.cpu.A = read_mmu(DE());
    return 8;
}

unsigned char LD_DE_A() {
    write_mmu(DE(), gameboy.cpu.A);
    return 8;
}

/*
 * LDI (HL),A
 * Description:
 * Put A into memory address HL. Increment HL.
 * Same as: LD (HL),A - INC HL
 */
unsigned char LDI_HL_A() {
    write_mmu(HL(), gameboy.cpu.A);
    set_HL(HL() + 1 & 0xFFFF);
    return 8;
}

/*
 * LDI A,(HL)
 * Description:
 * Put value at address HL into A. Increment HL.
 * Same as: LD A,(HL) - INC HL
 */
unsigned char LDI_A_HL() {
    gameboy.cpu.A = read_mmu(HL());
    set_HL(HL() + 1 & 0xFFFF);
    return 8;
}

/*
 * LDD (HL),A
 * Description:
 * Put A into memory address HL. Decrement HL.
 * Same as: LD (HL),A - DEC HL
 */
unsigned char LDD_HL_A() {
    write_mmu(HL(), gameboy.cpu.A);
    set_HL(HL() - 1);
    return 8;
}

/*
 * LDD A,(HL)
 * Description:
 * Put value at address HL into A. Decrement HL.
 * Same as: LD A,(HL) - DEC HL
 */
unsigned char LDD_A_HL() {
    gameboy.cpu.A = read_mmu(HL());
    set_HL(HL() - 1 & 0xFFFF);
    return 8;
}

/*
 * LDH (n),A
 * Description:
 * Put A into memory address $FF00+n.
 * Use with:
 * n = one byte immediate value.
 */
unsigned char LDH_n_A(unsigned char addr) {
    write_mmu(0xFF00 + addr, gameboy.cpu.A);
    return 12;
}

/*
 * LDH A,(n)
 * Description:
 * Put memory address $FF00+n into A.
 * Use with:
 * n = one byte immediate value.
 */
unsigned char LDH_A_n(unsigned char addr) {
    gameboy.cpu.A = read_mmu(0xFF00 + addr);
    return 12;
}

/*
 * LD A,(C)
 * Description:
 * Put value at address $FF00 + register C into A.
 * Same as: LD A,($FF00+C)
 */
unsigned char LD_A_C() {
    gameboy.cpu.A = read_mmu(0xFF00 + gameboy.cpu.C);
    return 4;
}

/*
 * LD (C),A
 * Description:
 * Put A into address $FF00 + register C.
 */
unsigned char LD_C_A() {
    write_mmu(0xFF00 + gameboy.cpu.C, gameboy.cpu.A);
    return 8;
}

/*
 * LD (nn),SP
 * Description:
 * Put Stack Pointer (SP) at address n.
 * Use with:
 * nn = two byte immediate address.
 */
unsigned char LD_a16_SP(unsigned short addr) {
    write_mmu(addr, gameboy.cpu.SP & 0xFF);
    write_mmu(addr + 1, gameboy.cpu.SP >> 8 & 0xFF);
    return 20;
}

/*
 * LD n,A
 * Description:
 * Put value A into n.
 * Use with:
 * nn = two byte immediate value. (LS byte first.)
 */
unsigned char LD_a16_A(unsigned short value) {
    write_mmu(value, gameboy.cpu.A);
    return 16;
}

/*
 * LD A,n
 * Description:
 * Put value n into A.
 * Use with:
 * nn = two byte immediate value. (LS byte first.)
 */
unsigned char LD_A_a16(unsigned short value) {
    gameboy.cpu.A = read_mmu(value);
    return 16;
}

/*
 * LDHL SP,n
 * Description:
 * Put SP + n effective address into HL.
 * Use with:
 * n = one byte signed immediate value.
 * Flags affected:
 * Z - Reset.
 * N - Reset.
 * H - Set or reset according to operation.
 * C - Set or reset according to operation.
 */
unsigned char LD_HL_SP_r8(char value) {
    unsigned short res = gameboy.cpu.SP + value;
    set_flag_Z(false);
    set_flag_N(false);
    set_flag_H((gameboy.cpu.SP & 0xF) + (value & 0xF) > 0xF);
    set_flag_C((gameboy.cpu.SP & 0xFF) + (value & 0xFF) > 0xFF);
    set_HL(res & 0xFFFF);
    return 12;
}

/*
 * LD SP,HL
 * Description:
 * Put HL into Stack Pointer (SP).
 */
unsigned char LD_SP_HL() {
    gameboy.cpu.SP = HL();
    return 8;
}

/*
 * DI
 * Description:
 * This instruction disables interrupts but not
 * immediately. Interrupts are disabled after
 * instruction after DI is executed.
 * Flags affected:
 * None.
 */
unsigned char DI() {
    gameboy.cpu.ime = false;
    return 4;
}

/*
 * EI
 * Description:
 * Enable interrupts. This intruction enables interrupts
 * but not immediately. Interrupts are enabled after
 * instruction after EI is executed.
 * Flags affected:
 * None.
 */
unsigned char EI() {
    gameboy.cpu.ime = true;
    return 4;
}