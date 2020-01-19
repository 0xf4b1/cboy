// SPDX-License-Identifier: GPL-3.0-only

#include <cstring>

#include "controls.h"
#include "display.h"

#include <controls.h>
#include <cpu.h>
#include <gameboy.h>

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app *app, int32_t cmd) {
    struct engine *engine = (struct engine *)app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        if (engine->app->window != NULL) {
            engine_init_display(engine);
        }
        break;
    case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        engine_term_display(engine);
        break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app *state) {
    struct engine engine;

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    jobject object = state->activity->clazz;

    JNIEnv *env;
    state->activity->vm->AttachCurrentThread(&env, 0);

    jclass clazz = env->GetObjectClass(object);
    jmethodID getIntentId = env->GetMethodID(clazz, "getIntent", "()Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(object, getIntentId);
    jclass intentClass = env->GetObjectClass(intent);
    jmethodID getDataStringId = env->GetMethodID(intentClass, "getDataString", "()Ljava/lang/String;");

    auto data = static_cast<jstring>(env->CallObjectMethod(intent, getDataStringId));
    const char *path = env->GetStringUTFChars(data, 0);
    env->ReleaseStringUTFChars(data, path);

    LOGI("starting rom: %s", path);
    load_rom(const_cast<char *>(path));

    while (true) {
        // Read all pending events.
        int events;
        struct android_poll_source *source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        Frame buffer = next_frame();
        engine_draw_frame(&engine, buffer.buffer);

        release_button();
    }
}