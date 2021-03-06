// SPDX-License-Identifier: GPL-3.0-only

#ifndef CBOY_KEYBOARD_H
#define CBOY_KEYBOARD_H

void special_key_handler(int key, int x, int y);

void special_key_up_handler(int key, int x, int y);

void normal_key_handler(unsigned char key, int x, int y);

void normal_key_up_handler(unsigned char key, int x, int y);

#endif // CBOY_KEYBOARD_H
