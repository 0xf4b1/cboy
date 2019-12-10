// SPDX-License-Identifier: GPL-3.0-only

#include <GL/glut.h>
#include <controls.h>
#include <display.h>
#include <gameboy.h>

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
    else if (key == GLUT_KEY_F11)
        toggle_fullscreen();
    else if (key == GLUT_KEY_F5)
        load_state();
    else if (key == GLUT_KEY_F6)
        save_state();
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