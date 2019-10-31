// SPDX-License-Identifier: GPL-3.0-only

#ifndef CPU_H
#define CPU_H

#include <stdbool.h>

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
} Cpu;

void next_frame();

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

#endif