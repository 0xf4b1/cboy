// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// See also libnx display/framebuffer.h.

#include <controls.h>
#include <cpu.h>
#include <gameboy.h>

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH 1280
#define FB_HEIGHT 720

#define WIDTH 160
#define HEIGHT 144

#define SCALE FB_HEIGHT / HEIGHT
#define X_OFFSET 50

// Main program entrypoint
int main(int argc, char *argv[]) {
    // Retrieve the default window
    NWindow *win = nwindowGetDefault();

    // Create a linear double-buffered framebuffer
    Framebuffer fb;
    framebufferCreate(&fb, win, FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    Frame buffer;
    load_rom("/switch/rom.gb");

    // Main loop
    while (appletMainLoop()) {
        // Scan all the inputs. This should be done once for each frame
        hidScanInput();

        // hidKeysDown returns information about which buttons have been
        // just pressed in this frame compared to the previous one
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS)
            break; // break in order to return to hbmenu

        release_all();

        if (kDown & KEY_A)
            press(CBOY_KEY_A);

        if (kDown & KEY_B)
            press(CBOY_KEY_B);

        if ((kDown & KEY_UP) || (kHeld & KEY_UP))
            press(UP);

        if ((kDown & KEY_DOWN) || (kHeld & KEY_DOWN))
            press(DOWN);

        if ((kDown & KEY_LEFT) || (kHeld & KEY_LEFT))
            press(LEFT);

        if ((kDown & KEY_RIGHT) || (kHeld & KEY_RIGHT))
            press(RIGHT);

        if (kDown & KEY_Y)
            press(SELECT);

        if (kDown & KEY_X)
            press(START);

        if (kDown & KEY_L)
            load_state();

        if (kDown & KEY_R)
            save_state();

        // Retrieve the framebuffer
        u32 stride;
        u32 *framebuffer = (u32 *)framebufferBegin(&fb, &stride);

        buffer = next_frame();

        for (unsigned char y = 0; y < HEIGHT; y++) {
            for (unsigned char x = 0; x < WIDTH; x++) {
                u32 color = buffer.buffer[y][x];
                u32 pos = y * stride / sizeof(u32) + x + X_OFFSET;

                for (unsigned char py = 0; py < SCALE; py++) {
                    for (unsigned char px = 0; px < SCALE; px++) {
                        framebuffer[pos * SCALE + py * stride / sizeof(u32) + px] = RGBA8_MAXALPHA(color, color, color);
                    }
                }
            }
        }

        // We're done rendering, so we end the frame here.
        framebufferEnd(&fb);
    }

    framebufferClose(&fb);
    return 0;
}

void serial_print(char c) {
    // no output
}