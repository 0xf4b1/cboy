// SPDX-License-Identifier: GPL-3.0-only

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "gameboy.hpp"
#include "renderer.hpp"

int cboy_main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: cboy <rom.gb>\n";
        return 1;
    }

    cboy::Gameboy gameboy;
    gameboy.load_rom(argv[1]);

    auto renderer = cboy::renderer::create();
    renderer->run(gameboy);

    return 0;
}

// Platform entry point
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    // Re-assemble argc/argv from the process command line so cboy_main
    // can use standard argument handling on all platforms.
    int argc = __argc;
    char **argv = __argv;
    return cboy_main(argc, argv);
}
#else
int main(int argc, char **argv) {
    return cboy_main(argc, argv);
}
#endif
