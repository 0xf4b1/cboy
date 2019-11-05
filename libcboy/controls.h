// SPDX-License-Identifier: GPL-3.0-only

#ifndef LIBCBOY_CONTROLS_H
#define LIBCBOY_CONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif

void press(unsigned char i);

void release(unsigned char i);

void release_all();

#ifdef __cplusplus
}
#endif

#endif // LIBCBOY_CONTROLS_H
