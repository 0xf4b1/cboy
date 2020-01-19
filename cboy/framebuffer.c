// SPDX-License-Identifier: GPL-3.0-only

#include <fcntl.h>
#include <inttypes.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <cpu.h>
#include <display.h>

#define WIDTH 160
#define HEIGHT 144

struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
uint8_t *fbp;

unsigned char scale = 1;
int x_offset = 0;

void draw_pixel_rgb(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    uint32_t pixel =
        (r << vinfo.red.offset) | (g << vinfo.green.offset) | (b << vinfo.blue.offset) | (a << vinfo.transp.offset);
    uint32_t location = x * vinfo.bits_per_pixel / 8 + y * finfo.line_length;
    *((uint32_t *)(fbp + location)) = pixel;
}

void draw_pixel(unsigned char x, unsigned char y, unsigned char color) {
    for (unsigned char px = 0; px < scale; px++) {
        for (unsigned char py = 0; py < scale; py++) {
            draw_pixel_rgb(x * scale + px + x_offset, y * scale + py, color, color, color, 0);
        }
    }
}

void display_loop() {
    int fd = open("/dev/fb0", O_RDWR);
    ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(fd, FBIOGET_FSCREENINFO, &finfo);

    size_t size = vinfo.yres * finfo.line_length;

    scale = vinfo.yres / HEIGHT;
    x_offset = (vinfo.xres - WIDTH * scale) / 2;

    fbp = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    Frame buffer;

    int frames = 60;
    int throttle = 0;
    time_t last_time = 0;

    while (true) {

        buffer = next_frame();

        for (unsigned char y = 0; y < HEIGHT; y++) {
            for (unsigned char x = 0; x < WIDTH; x++) {
                draw_pixel(x, y, buffer.buffer[y][x]);
            }
        }

        frames++;
        if (time(NULL) > last_time) {
            time(&last_time);
            if (throttle == 0) {
                throttle = frames;
            } else {
                throttle = throttle * ((float)frames / 60);
            }
            frames = 0;
        }
        if (throttle > 60) {
            usleep(1000000 * ((1 - (float)60 / throttle)) / 60);
        }
    }
}