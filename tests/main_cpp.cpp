// SPDX-License-Identifier: GPL-3.0-only
// Test runner for BACKEND=LIBCPPBOY.
// Mirrors main.c but uses the cboy::Gameboy C++ API.

#include <cstdio>
#include <cstdlib>

#include "gameboy.hpp"

// The GB test ROMs write 'P' (PASSED) or 'F' (FAILED) via the serial port.
// We override serial_print in the Gameboy class by subclassing it.

class TestGameboy : public cboy::Gameboy {
public:
    void serial_print(char c) override {
        if (c == 'P') std::exit(0);
        if (c == 'F') std::exit(1);
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::puts("No rom file specified");
        return 1;
    }

    TestGameboy gb;
    gb.load_rom(argv[1]);

    // run for up to 2000 frames and fail when there is no result
    for (int i = 0; i < 2000; ++i)
        gb.run_frame();

    return 1;
}
