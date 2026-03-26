// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "gameboy.hpp"
#include "renderer_opengl.hpp"
#include "app_context.hpp"

#ifdef linux
#include "joystick.h"
#endif

#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "No rom file specified";
        exit(1);
    }

#ifdef linux
    init_joystick();
#endif
    cout << "Loading rom ..." << endl;
    cboy::Gameboy gameboy;
    gameboy.load_rom(argv[1]);
    cboy::renderer::OpenGLRenderer renderer;
    cboy::display_loop(gameboy, renderer);
}

void serial_print(char c) {
    printf("%c", c);
}