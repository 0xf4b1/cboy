// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_DISPLAY_H
#define LIBCBOY_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned short buffer[144][160];
} Frame;

void draw();

void set_params(unsigned char i);

void toggle_fullscreen();

#ifdef __cplusplus
}
#endif

#endif // LIBCBOY_DISPLAY_H