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
            if (event.number == 12) {
                fun(0);
            } else if (event.number == 11) {
                fun(1);
            } else if (event.number == 13) {
                fun(2);
            } else if (event.number == 14) {
                fun(3);
            } else if (event.number == 0) {
                fun(4);
            } else if (event.number == 1) {
                fun(5);
            } else if (event.number == 6) {
                fun(6);
            } else if (event.number == 7) {
                fun(7);
            } else if (event.number == 4) {
                load_state();
            } else if (event.number == 5) {
                save_state();
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