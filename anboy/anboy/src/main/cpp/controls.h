// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANBOY_CONTROLS_H
#define ANBOY_CONTROLS_H

#include <android_native_app_glue.h>

int32_t engine_handle_input(struct android_app *app, AInputEvent *event);

void release_button();

#endif // ANBOY_CONTROLS_H