// SPDX-License-Identifier: GPL-3.0-only

#include "cpu.h"
#include "gameboy.h"

#define WIDTH 160
#define HEIGHT 144

unsigned char background[256][256];
unsigned char window[256][256];

unsigned char scy[HEIGHT + 1] = {[0 ... HEIGHT] = 0};
unsigned char scx[HEIGHT + 1] = {[0 ... HEIGHT] = 0};
unsigned char wy[HEIGHT + 1] = {[0 ... HEIGHT] = 0};
unsigned char wx[HEIGHT + 1] = {[0 ... HEIGHT] = 0};

void set_params(unsigned char i) {
    scy[i] = read_mmu(0xFF42);
    scx[i] = read_mmu(0xFF43);
    wy[i] = read_mmu(0xFF4A);
    wx[i] = read_mmu(0xFF4B);
}

void draw_color(unsigned char x, unsigned char y, unsigned char color) {
    switch (color) {
        case 0:
            color = 255;
            break;
        case 1:
            color = 128;
            break;
        case 2:
            color = 64;
            break;
        case 3:
            color = 0;
            break;
    }
    gameboy.framebuffer.buffer[x][y] = color;
}

void draw_sprite(unsigned char offset_x, unsigned char offset_y, unsigned short tile_offset, bool x_flip, bool y_flip) {
    unsigned char palette = read_mmu(0xFF48);

    for (unsigned char y = 0; y < 8; y++) {
        for (unsigned char x = 0; x < 8; x++) {

            if (offset_x + x < 8 || offset_y + y < 16 || offset_x + x >= WIDTH + 8 || offset_y + y >= HEIGHT + 16) {
                continue;
            }

            unsigned char color = read_mmu((y_flip ? 7 - y : y) * 2 + tile_offset) >> (x_flip ? x : (7 - x)) & 1 |
                                  (read_mmu((y_flip ? 7 - y : y) * 2 + 1 + tile_offset) >> (x_flip ? x : (7 - x)) & 1)
                                      << 1;

            if (color == 0) {
                continue;
            }

            color = (palette >> (color * 2)) & 3;
            draw_color(offset_y + y - 16, offset_x + x - 8, color);
        }
    }
}

void draw_tile(unsigned char offset_x, unsigned char offset_y, unsigned short tile_addr, unsigned char palette,
               unsigned char buffer[256][256]) {

    for (unsigned char y = 0; y < 8; y++) {
        for (unsigned char x = 0; x < 8; x++) {
            unsigned char color = ((read_mmu(y * 2 + tile_addr) >> (7 - x)) & 1) |
                                  ((read_mmu(y * 2 + 1 + tile_addr) >> (7 - x)) & 1) << 1;

            color = (palette >> (color * 2)) & 3;
            buffer[offset_x + x][offset_y + y] = color;
        }
    }
}

void render_bg() {
    for (unsigned char y = 0; y < 32; y++) {
        for (unsigned char x = 0; x < 32; x++) {
            draw_tile(x * 8, y * 8, get_tile(x, y, false), read_mmu(0xFF47), background);
        }
    }

    for (unsigned char y = 0; y < HEIGHT; y++) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            draw_color(y, x, background[(x + scx[y]) % 256][(y + scy[y]) % 256]);
        }
    }
}

void render_window() {
    if (!window_display_enable()) {
        return;
    }

    for (unsigned char y = 0; y < 32; y++) {
        for (unsigned char x = 0; x < 32; x++) {
            draw_tile(x * 8, y * 8, get_tile(x, y, true), read_mmu(0xFF47), window);
        }
    }

    for (unsigned char y = 0; y < HEIGHT; y++) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            if (x + wx[y] >= 7 && x + wx[y] < WIDTH + 7 && y + wy[y] >= 0 && y + wy[y] < HEIGHT) {
                draw_color(y + wy[y], x + wx[y] - 7, window[x][y]);
            }
        }
    }
}

void render_sprites() {
    for (unsigned char i = 0; i < 0xA0; i += 4) {
        unsigned char y = read_mmu(0xFE00 + i);
        unsigned char x = read_mmu(0xFE00 + i + 1);
        unsigned char tile = read_mmu(0xFE00 + i + 2);
        unsigned char attr = read_mmu(0xFE00 + i + 3);
        bool x_flip = attr >> 5 & 1;
        bool y_flip = attr >> 6 & 1;

        if (obj_sprite_size() == 0) {
            // 8x8 sprite
            draw_sprite(x, y, 0x8000 + tile * 16, x_flip, y_flip);
        } else {
            // 8x16 sprite
            draw_sprite(x, y, 0x8000 + (tile & 0xFE) * 16, x_flip, y_flip);
            draw_sprite(x, y + 8, 0x8000 + (tile | 1) * 16, x_flip, y_flip);
        }
    }
}

void draw() {
    render_bg();
    render_window();
    render_sprites();
}