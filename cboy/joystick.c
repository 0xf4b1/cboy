// SPDX-License-Identifier: GPL-3.0-only

#include <fcntl.h>
#include <gameboy.h>
#include <linux/joystick.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "controls.h"

int js_dev = -1;

int read_event(int fd, struct js_event *event) {
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    return -1;
}

void *joystick_thread() {
    struct js_event event;

    while (read_event(js_dev, &event) == 0) {
        if (event.type == JS_EVENT_BUTTON) {
            void (*fun)(unsigned char) = event.value ? press : release;
            switch (event.number) {
                case 12:
                    fun(RIGHT);
                    break;
                case 11:
                    fun(LEFT);
                    break;
                case 13:
                    fun(UP);
                    break;
                case 14:
                    fun(DOWN);
                    break;
                case 0:
                    fun(CBOY_KEY_A);
                    break;
                case 1:
                    fun(CBOY_KEY_B);
                    break;
                case 6:
                    fun(SELECT);
                    break;
                case 7:
                    fun(START);
                    break;
                case 4:
                    load_state();
                    break;
                case 5:
                    save_state();
                    break;
            }
        }
    }

    return NULL;
}

void init_joystick() {
    js_dev = open("/dev/input/js0", O_RDONLY);

    if (js_dev == -1)
        perror("Could not open joystick");

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, joystick_thread, NULL);
}