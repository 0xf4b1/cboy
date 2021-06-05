// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_CPU_H
#define LIBCBOY_CPU_H

#include <stdbool.h>

#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // 8 bit registers
    unsigned char A;
    unsigned char F;
    unsigned char B;
    unsigned char C;
    unsigned char D;
    unsigned char E;
    unsigned char H;
    unsigned char L;

    unsigned short SP;
    unsigned short PC;

    bool ime;
    bool halt;
} Cpu;

extern Cpu cpu;

Frame next_frame();

inline unsigned short AF() { return (cpu.A << 8) + cpu.F; }
inline unsigned short BC() { return (cpu.B << 8) + cpu.C; }
inline unsigned short DE() { return (cpu.D << 8) + cpu.E; }
inline unsigned short HL() { return (cpu.H << 8) + cpu.L; }
inline unsigned short SP() { return cpu.SP; }

inline void set_AF(unsigned short value) {
    cpu.A = value >> 8 & 0xFF;
    cpu.F = value & 0xF0;
}

inline void set_BC(unsigned short value) {
    cpu.B = value >> 8 & 0xFF;
    cpu.C = value & 0xFF;
}

inline void set_DE(unsigned short value) {
    cpu.D = value >> 8 & 0xFF;
    cpu.E = value & 0xFF;
}

inline void set_HL(unsigned short value) {
    cpu.H = value >> 8 & 0xFF;
    cpu.L = value & 0xFF;
}

inline void set_SP(unsigned short value) { cpu.SP = value; }

inline bool get_bit(unsigned char i) { return cpu.F >> i & 1; }

inline void set_bit(unsigned char i, bool set) {
    if (set) {
        cpu.F |= 1 << i;
    } else {
        cpu.F &= ~(1 << i);
    }
}

inline bool flag_Z() { return get_bit(7); }
inline bool flag_N() { return get_bit(6); }
inline bool flag_H() { return get_bit(5); }
inline bool flag_C() { return get_bit(4); }

inline void set_flag_Z(bool set) { set_bit(7, set); }
inline void set_flag_N(bool set) { set_bit(6, set); }
inline void set_flag_H(bool set) { set_bit(5, set); }
inline void set_flag_C(bool set) { set_bit(4, set); }

#ifdef __cplusplus
}
#endif

#endif // LIBCBOY_CPU_H