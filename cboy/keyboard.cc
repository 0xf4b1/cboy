// SPDX-License-Identifier: GPL-3.0-only

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <controls.hpp>
#include "app_context.hpp"
#include <gameboy.hpp>

void normal_key_handler(unsigned char key, int /*x*/, int /*y*/) {
    auto* gameboy = cboy::get_app_gameboy();
    if (!gameboy) return;
    auto& controls = gameboy->controls();
    switch (key) {
        case GLUT_KEY_RIGHT:
            controls.press(cboy::Button::RIGHT);
            break;
        case GLUT_KEY_LEFT:
            controls.press(cboy::Button::LEFT);
            break;
        case GLUT_KEY_UP:
            controls.press(cboy::Button::UP);
            break;
        case GLUT_KEY_DOWN:
            controls.press(cboy::Button::DOWN);
            break;
        case 'a':
            controls.press(cboy::Button::A);
            break;
        case 's':
            controls.press(cboy::Button::B);
            break;
        case 'q':
            controls.press(cboy::Button::START);
            break;
        case 'w':
            controls.press(cboy::Button::SELECT);
            break;
    }
}

void normal_key_up_handler(unsigned char key, int /*x*/, int /*y*/) {
    auto* gameboy = cboy::get_app_gameboy();
    if (!gameboy) return;
    auto& controls = gameboy->controls();
    switch (key) {
        case GLUT_KEY_RIGHT:
            controls.release(cboy::Button::RIGHT);
            break;
        case GLUT_KEY_LEFT:
            controls.release(cboy::Button::LEFT);
            break;
        case GLUT_KEY_UP:
            controls.release(cboy::Button::UP);
            break;
        case GLUT_KEY_DOWN:
            controls.release(cboy::Button::DOWN);
            break;
        case 'a':
            controls.release(cboy::Button::A);
            break;
        case 's':
            controls.release(cboy::Button::B);
            break;
        case 'q':
            controls.release(cboy::Button::START);
            break;
        case 'w':
            controls.release(cboy::Button::SELECT);
            break;
    }
}

void special_key_handler(int key, int x, int y) {
    normal_key_handler(key, x, y);
}

void special_key_up_handler(int key, int x, int y) {
    normal_key_up_handler(key, x, y);
}