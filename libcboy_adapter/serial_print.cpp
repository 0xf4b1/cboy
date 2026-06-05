// SPDX-License-Identifier: GPL-3.0-only
// Default serial_print for BACKEND=LIBCBOY.
// libcboy calls serial_print() as an external C symbol; this file satisfies
// that reference in the main emulator executable.
// Test binaries supply their own definition (main.c already does this).

#include <cstdio>

extern "C" void serial_print(char c) {
    std::putchar(c);
}
