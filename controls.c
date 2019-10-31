// SPDX-License-Identifier: GPL-3.0-only

#include "gameboy.h"
#include <GL/glut.h>

void press(unsigned char i) { gameboy.controls &= ~(1 << i); }

void release(unsigned char i) { gameboy.controls |= (1 << i); }

void special_key_handler(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT)
        press(0);
    else if (key == GLUT_KEY_LEFT)
        press(1);
    else if (key == GLUT_KEY_UP)
        press(2);
    else if (key == GLUT_KEY_DOWN)
        press(3);
}

void special_key_up_handler(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT)
        release(0);
    else if (key == GLUT_KEY_LEFT)
        release(1);
    else if (key == GLUT_KEY_UP)
        release(2);
    else if (key == GLUT_KEY_DOWN)
        release(3);
}

void normal_key_handler(unsigned char key, int x, int y) {
    if (key == 'a')
        press(4);
    else if (key == 's')
        press(5);
    else if (key == 'w')
        press(6);
    else if (key == 'q')
        press(7);
}

void normal_key_up_handler(unsigned char key, int x, int y) {
    if (key == 'a')
        release(4);
    else if (key == 's')
        release(5);
    else if (key == 'w')
        release(6);
    else if (key == 'q')
        release(7);
}