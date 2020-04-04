// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>

#include "gameboy.h"
#include "joystick.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("No rom file specified");
        exit(1);
    }

    init_joystick();
    load_rom(argv[1]);
    display_loop();
}

void serial_print(char c) {
    printf("%c", c);
}