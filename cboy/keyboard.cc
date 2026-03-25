// SPDX-License-Identifier: GPL-3.0-only

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <controls.h>
#include <display.h>
#include <gameboy.h>

static void handle_key(int key, void (*function)(unsigned char)) {
    switch (key) {
        case GLUT_KEY_RIGHT:
            function(RIGHT);
            break;
        case GLUT_KEY_LEFT:
            function(LEFT);
            break;
        case GLUT_KEY_UP:
            function(UP);
            break;
        case GLUT_KEY_DOWN:
            function(DOWN);
            break;
        case 'a':
            function(CBOY_KEY_A);
            break;
        case 's':
            function(CBOY_KEY_B);
            break;
        case 'q':
            function(START);
            break;
        case 'w':
            function(SELECT);
            break;
    }
}

void special_key_handler(int key, int x, int y) {
    handle_key(key, press);
}

void special_key_up_handler(int key, int x, int y) {
    if (key == GLUT_KEY_F5)
        load_state();
    else if (key == GLUT_KEY_F6)
        save_state();
    else if (key == GLUT_KEY_F11)
        toggle_fullscreen();
    else
        handle_key(key, release);
}

void normal_key_handler(unsigned char key, int x, int y) {
    handle_key(key, press);
}

void normal_key_up_handler(unsigned char key, int x, int y) {
    handle_key(key, release);
}