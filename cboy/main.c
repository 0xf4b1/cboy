// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "gameboy.h"

#ifdef linux
#include "joystick.h"
#endif

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("No rom file specified");
        exit(1);
    }

#ifdef linux
    init_joystick();
#endif
    load_rom(argv[1]);
    display_loop();
}

void serial_print(char c) {
    printf("%c", c);
}