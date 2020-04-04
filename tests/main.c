// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>

#include "gameboy.h"

void serial_print(char c) {
    if (c == 'P') {
        // begin of PASSED, test was successful
        exit(0);
    } else if (c == 'F') {
        // begin of FAILED, test was not successful
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("No rom file specified");
        exit(1);
    }

    load_rom(argv[1]);

    // run for some frames and fail when there is no result
    for (int i = 0; i < 2000; i++) {
        next_frame();
    }

    exit(1);
}