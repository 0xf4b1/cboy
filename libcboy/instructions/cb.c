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
static inline unsigned char RLC(unsigned char value) {
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
static inline unsigned char RRC(unsigned char value) {
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
static inline unsigned char RL(unsigned char value) {
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
static inline unsigned char RR(unsigned char value) {
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
static inline unsigned char SLA(unsigned char value) {
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
static inline unsigned char SRA(unsigned char value) {
    bool c = value & 1;
    value = (value >> 1 | (value & (1 << 7))) & 0xFF;
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
static inline unsigned char SWAP(unsigned char value) {
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
static inline unsigned char SRL(unsigned char value) {
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
static inline unsigned char BIT(unsigned char value, unsigned char i) {
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
static inline unsigned char RES(unsigned char value, unsigned char i) { return value & ~(1 << i); }

/*
 * SET b,r
 * Description:
 * Set bit b in register r.
 * Use with:
 * b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
 * Flags affected:
 * None.
 */
static inline unsigned char SET(unsigned char value, unsigned char i) { return value | 1 << i; }

#define DEFINE_OP_REG(OP, REG) \
        void OP ## _ ## REG() { \
            gameboy.cpu.REG = OP(gameboy.cpu.REG); \
        }

#define DEFINE_CB_OPS(REG) \
        DEFINE_OP_REG(RLC, REG) \
        DEFINE_OP_REG(RRC, REG) \
        DEFINE_OP_REG(RL, REG) \
        DEFINE_OP_REG(RR, REG) \
        DEFINE_OP_REG(SLA, REG) \
        DEFINE_OP_REG(SRA, REG) \
        DEFINE_OP_REG(SWAP, REG) \
        DEFINE_OP_REG(SRL, REG)

#define DEFINE_BIT_N_REG(N, REG) \
        void BIT_ ## N ## _ ## REG() { \
            gameboy.cpu.REG = BIT(gameboy.cpu.REG, N); \
        } \
        \
        void RES_ ## N ## _ ## REG() { \
            gameboy.cpu.REG = RES(gameboy.cpu.REG, N); \
        } \
        \
        void SET_ ## N ## _ ## REG() { \
            gameboy.cpu.REG = SET(gameboy.cpu.REG, N); \
        }

#define DEFINE_BIT_REG(REG) \
        DEFINE_BIT_N_REG(0, REG) \
        DEFINE_BIT_N_REG(1, REG) \
        DEFINE_BIT_N_REG(2, REG) \
        DEFINE_BIT_N_REG(3, REG) \
        DEFINE_BIT_N_REG(4, REG) \
        DEFINE_BIT_N_REG(5, REG) \
        DEFINE_BIT_N_REG(6, REG) \
        DEFINE_BIT_N_REG(7, REG)

#define DEFINE_CB(REG) \
        DEFINE_CB_OPS(REG) \
        DEFINE_BIT_REG(REG)

DEFINE_CB(B)
DEFINE_CB(C)
DEFINE_CB(D)
DEFINE_CB(E)
DEFINE_CB(H)
DEFINE_CB(L)
DEFINE_CB(A)

#define DEFINE_OP_HL(OP) \
        void OP ## _HL() { \
            write_mmu(HL(), OP(read_mmu(HL()))); \
        }

DEFINE_OP_HL(RLC)
DEFINE_OP_HL(RRC)
DEFINE_OP_HL(RL)
DEFINE_OP_HL(RR)
DEFINE_OP_HL(SLA)
DEFINE_OP_HL(SRA)
DEFINE_OP_HL(SWAP)
DEFINE_OP_HL(SRL)

#define DEFINE_BIT_N_HL(N) \
        void BIT_ ## N ## _HL() { \
            write_mmu(HL(), BIT(read_mmu(HL()), N)); \
        } \
        \
        void RES_ ## N ## _HL() { \
            write_mmu(HL(), RES(read_mmu(HL()), N)); \
        } \
        \
        void SET_ ## N ## _HL() { \
            write_mmu(HL(), SET(read_mmu(HL()), N)); \
        }

DEFINE_BIT_N_HL(0)
DEFINE_BIT_N_HL(1)
DEFINE_BIT_N_HL(2)
DEFINE_BIT_N_HL(3)
DEFINE_BIT_N_HL(4)
DEFINE_BIT_N_HL(5)
DEFINE_BIT_N_HL(6)
DEFINE_BIT_N_HL(7)