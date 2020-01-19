// SPDX-License-Identifier: GPL-3.0-only

#include <cmath>

#include "display.h"

#include <controls.h>
#include <gameboy.h>

float down_x, down_y = 0;
bool pressed_direction, pressed_button = false;

/**
 * Process the next input event.
 */
int32_t engine_handle_input(struct android_app *app, AInputEvent *event) {
    int32_t eventType = AInputEvent_getType(event);
    if (eventType == AINPUT_EVENT_TYPE_MOTION && AInputEvent_getSource(event) == AINPUT_SOURCE_TOUCHSCREEN) {
        int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
            down_x = AMotionEvent_getX(event, 0);
            down_y = AMotionEvent_getY(event, 0);
            break;
        case AMOTION_EVENT_ACTION_UP:
            // releasing key
            if (!pressed_direction) {
                if (down_y > (float)get_display_height() / 2) {
                    // A button
                    press(CBOY_KEY_A);
                } else if (down_y > (float)get_display_height() / 4) {
                    if (down_x < (float)get_display_width() / 2) {
                        // select
                        press(SELECT);
                    } else {
                        // start
                        press(START);
                    }
                } else {
                    if (down_x < (float)get_display_width() / 2) {
                        load_state();
                    } else {
                        save_state();
                    }
                }
                pressed_button = true;
            } else {
                release_all();
                pressed_direction = false;
            }
            break;
        case AMOTION_EVENT_ACTION_MOVE:

            if (pressed_direction)
                break;

            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);
            float delta_x = x - down_x;
            float delta_y = y - down_y;

            if (abs(delta_x - delta_y) < 10) {
                break;
            } else if (abs(delta_x) > abs(delta_y)) {
                if (delta_x > 0) {
                    press(RIGHT);
                } else {
                    // left
                    press(LEFT);
                }
            } else {
                if (delta_y > 0) {
                    // down
                    press(DOWN);
                } else {
                    // up
                    press(UP);
                }
            }
            pressed_direction = true;
            break;
        }
    }
    return 0;
}

void release_button() {
    if (pressed_button) {
        pressed_button = false;
        release(CBOY_KEY_A);
        release(SELECT);
        release(START);
    }
}