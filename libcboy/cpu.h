// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_CPU_H
#define LIBCBOY_CPU_H

#include "display.h"
#include <stdbool.h>

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

Frame next_frame();

unsigned short AF();

unsigned short BC();

unsigned short DE();

unsigned short HL();

unsigned short SP();

void set_AF(unsigned short value);

void set_BC(unsigned short value);

void set_DE(unsigned short value);

void set_HL(unsigned short value);

void set_SP(unsigned short value);

bool flag_Z();

bool flag_N();

bool flag_H();

bool flag_C();

void set_flag_Z(bool set);

void set_flag_N(bool set);

void set_flag_H(bool set);

void set_flag_C(bool set);

#ifdef __cplusplus
}
#endif

#endif // LIBCBOY_CPU_H