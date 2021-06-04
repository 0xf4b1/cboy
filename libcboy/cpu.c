// SPDX-License-Identifier: GPL-3.0-only

#include "display.h"
#include "gameboy.h"
#include "timer.h"
#include "instructions/opcodes.h"

unsigned char fetch_instruction() {
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
void check_interrupt() {

    if (!gameboy.cpu.ime) {
        return;
    }

    unsigned char interrupt_enable = read_mmu(0xFFFF);
    unsigned char interrupt_flag = read_mmu(0xFF0F);

    for (unsigned char i = 0; i < 5; i++) {
        // check for interrupt enable and interrupt request being set
        if (interrupt_enable >> i & 1 && interrupt_flag >> i & 1) {
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
        }
    }
}

unsigned char next_instruction() {

    check_interrupt();

    unsigned char inst = fetch_instruction();

    if (inst == 0xCB) {
        inst = fetch_instruction();
        decode_and_execute_cb(inst);
        return 8;
    }

    unsigned char length = op_length(inst);

    if (length == 1) {
        return decode_and_execute(inst, 0);

    } else if (length == 2) {
        unsigned char param = fetch_instruction();

        return decode_and_execute(inst, param);

    } else {
        unsigned char v1 = fetch_instruction();
        unsigned char v2 = fetch_instruction();

        unsigned short param = (v2 << 8) + v1;

        return decode_and_execute(inst, param);
    }
}

void next_instructions(int cycles) {
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