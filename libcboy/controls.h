// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_CONTROLS_H
#define LIBCBOY_CONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RIGHT 0
#define LEFT 1
#define UP 2
#define DOWN 3

#define CBOY_KEY_A 4
#define CBOY_KEY_B 5
#define SELECT 6
#define START 7

void press(unsigned char i);

void release(unsigned char i);

void release_all();

#ifdef __cplusplus
}
#endif

#endif // LIBCBOY_CONTROLS_H
