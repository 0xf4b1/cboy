// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANBOY_DISPLAY_H
#define ANBOY_DISPLAY_H

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#define WIDTH 160
#define HEIGHT 144

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "anboy", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "anboy", __VA_ARGS__))

int32_t get_display_width();

int32_t get_display_height();

int engine_init_display(struct engine *engine);

void engine_draw_frame(struct engine *engine, unsigned char buffer[HEIGHT][WIDTH]);

void engine_term_display(struct engine *engine);

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app *app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
};

#endif // ANBOY_DISPLAY_H