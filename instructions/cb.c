// SPDX-License-Identifier: GPL-3.0-only

#include "../gameboy.h"

/*
 * RLC n
 * Description:
 * Rotate n left. Old bit 7 to Carry flag.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
unsigned char RLC(unsigned char value) {
    bool c = (value >> 7) & 1;
    value = ((value << 1) | c) & 0xFF;
    set_flag_Z(value == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return value;
}

/*
 * RRC n
 * Description:
 * Rotate n right. Old bit 0 to Carry flag.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data
 */
unsigned char RRC(unsigned char value) {
    bool c = value & 1;
    value = ((value >> 1) | (c << 7)) & 0xFF;
    set_flag_Z(value == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return value;
}

/*
 * RL n
 * Description:
 * Rotate n left through Carry flag.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
unsigned char RL(unsigned char value) {
    bool c = (value >> 7) & 1;
    value = ((value << 1) | flag_C()) & 0xFF;
    set_flag_Z(value == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return value;
}

/*
 * RR n
 * Description:
 * Rotate n right through Carry flag.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
unsigned char RR(unsigned char value) {
    bool c = value & 1;
    value = ((value >> 1) | (flag_C() << 7)) & 0xFF;
    set_flag_Z(value == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return value;
}

/*
 * SLA n
 * Description:
 * Shift n left into Carry. LSB of n set to 0.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 7 data.
 */
unsigned char SLA(unsigned char value) {
    bool c = value >> 7 & 1;
    value = value << 1 & 0xFF;
    set_flag_Z(value == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return value;
}

/*
 * SRA n
 * Description:
 * Shift n right into Carry. MSB doesn't change.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data.
 */
unsigned char SRA(unsigned char value) {
    bool c = value & 1;
    value = (value >> 1 | value & (1 << 7)) & 0xFF;
    set_flag_Z(value == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(c);
    return value;
}

/*
 * SWAP n
 * Description:
 * Swap upper & lower nibles of n.
 * Use with:
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Reset.
 */
unsigned char SWAP(unsigned char value) {
    unsigned char res = (value << 4 | value >> 4) & 0xFF;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(false);
    return res;
}

/*
 * SRL n
 * Description {
 * Shift n right into Carry. MSB set to 0.
 * Use with {
 * n = A,B,C,D,E,H,L,(HL)
 * Flags affected {
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Contains old bit 0 data
 */
unsigned char SRL(unsigned char value) {
    unsigned char res = value >> 1;
    set_flag_Z(res == 0);
    set_flag_N(false);
    set_flag_H(false);
    set_flag_C(value & 1);
    return res;
}

/*
 * BIT b,r
 * Description:
 * Test bit b in register r.
 * Use with:
 * b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * Z - Set if bit b of register r is 0.
 * N - Reset.
 * H - Set.
 * C - Not affected.
 */
unsigned char BIT(unsigned char value, unsigned char i) {
    set_flag_Z((value >> i & 1) == 0);
    set_flag_N(false);
    set_flag_H(true);
    return value;
}

/*
 * RES b,r
 * Description:
 * Reset bit b in register r.
 * Use with:
 * b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * None.
 */
unsigned char RES(unsigned char value, unsigned char i) { return value & ~(1 << i); }

/*
 * SET b,r
 * Description:
 * Set bit b in register r.
 * Use with:
 * b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * None.
 */
unsigned char SET(unsigned char value, unsigned char i) { return value | 1 << i; }