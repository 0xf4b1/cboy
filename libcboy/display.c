// SPDX-License-Identifier: GPL-3.0-only

#include "cpu.h"
#include "gameboy.h"

#define WIDTH 160
#define HEIGHT 144

static unsigned short background[256][256];
static unsigned short window[256][256];

static unsigned char scy[HEIGHT + 1] = {[0 ... HEIGHT] = 0};
static unsigned char scx[HEIGHT + 1] = {[0 ... HEIGHT] = 0};
static unsigned char wy[HEIGHT + 1] = {[0 ... HEIGHT] = 0};
static unsigned char wx[HEIGHT + 1] = {[0 ... HEIGHT] = 0};

void set_params(unsigned char i) {
    scy[i] = read_mmu(0xFF42);
    scx[i] = read_mmu(0xFF43);
    wy[i] = read_mmu(0xFF4A);
    wx[i] = read_mmu(0xFF4B);
}

static void draw_greyscale(unsigned char x, unsigned char y, unsigned char color) {
    switch (color) {
        case 0:
            gameboy.framebuffer.buffer[x][y] = 0xffff;
            break;
        case 1:
            gameboy.framebuffer.buffer[x][y] = 0x4210;
            break;
        case 2:
            gameboy.framebuffer.buffer[x][y] = 0x2108;
            break;
        case 3:
            gameboy.framebuffer.buffer[x][y] = 0x0;
            break;
    }
}

static void draw_color(unsigned char x, unsigned char y, unsigned short color) {
    gameboy.framebuffer.buffer[x][y] = color;
}

static void draw_sprite(unsigned char offset_x, unsigned char offset_y, unsigned short tile_offset, unsigned char attr) {
    unsigned char palette_number = attr & 3;
    bool vram_bank = attr >> 3 & 1;
    bool obp1 = attr >> 4 & 1;
    bool x_flip = attr >> 5 & 1;
    bool y_flip = attr >> 6 & 1;

    unsigned long color_palette;
    unsigned char palette;

    if (gameboy.cgb)
        color_palette = *(unsigned long *)(gameboy.mmu.sprite_palette + palette_number * sizeof(long));
    else
        palette = read_mmu(obp1 ? 0xFF49 : 0xFF48);

    for (unsigned char y = 0; y < 8; y++) {
        for (unsigned char x = 0; x < 8; x++) {

            if (offset_x + x < 8 || offset_y + y < 16 || offset_x + x >= WIDTH + 8 || offset_y + y >= HEIGHT + 16)
                continue;

            unsigned char color;
            unsigned short offset = (y_flip ? 7 - y : y) * 2 + tile_offset - 0x8000;
            if (!vram_bank)
                color = gameboy.mmu.ram[offset];
            else
                color = gameboy.mmu.vram_bank[offset];

            color = ((color >> (x_flip ? x : (7 - x))) & 1) | (((gameboy.mmu.ram[offset + 1] >> (x_flip ? x : (7 - x))) & 1) << 1);

            if (color == 0)
                continue;

            if (gameboy.cgb)
                draw_color(offset_y + y - 16, offset_x + x - 8, (color_palette >> (16 * color)) & 0xffff);
            else
                draw_greyscale(offset_y + y - 16, offset_x + x - 8, (palette >> (color * 2)) & 3);
        }
    }
}

static void draw_tile(unsigned char offset_x, unsigned char offset_y, bool window, unsigned short buffer[256][256]) {
    unsigned short tile_addr = get_tile(offset_x, offset_y, window);
    unsigned char first, second, attr, color, palette;
    unsigned long color_palette;

    bool map_display_select = window ? window_tile_map_display_select() : bg_tile_map_display_select();
    attr = gameboy.mmu.vram_bank[(map_display_select ? 0x9C00 : 0x9800) + offset_y * 32 + offset_x - 0x8000];
    for (unsigned char y = 0; y < 8; y++) {

        unsigned short offset = y * 2 + tile_addr - 0x8000;
        if (attr >> 3 & 1) {
            first = gameboy.mmu.vram_bank[offset];
            second = gameboy.mmu.vram_bank[offset + 1];
        } else {
            first = gameboy.mmu.ram[offset];
            second = gameboy.mmu.ram[offset + 1];
        }

        if (gameboy.cgb)
            color_palette = *(unsigned long *)(gameboy.mmu.bg_palette + (attr & 7) * sizeof(long));
        else
            palette = read_mmu(0xFF47);

        for (unsigned char x = 0; x < 8; x++) {
            color = ((first >> (7 - x)) & 1) | ((second >> (7 - x)) & 1) << 1;

            if (gameboy.cgb)
                buffer[offset_x * 8 + x][offset_y * 8 + y] = (color_palette >> (16 * color)) & 0xffff;
            else
                buffer[offset_x * 8 + x][offset_y * 8 + y] = (unsigned short)(palette >> (color * 2)) & 3;
        }
    }
}

static void render_bg() {
    for (unsigned char y = 0; y < 32; y++) {
        for (unsigned char x = 0; x < 32; x++) {
            draw_tile(x, y, false, background);
        }
    }

    for (unsigned char y = 0; y < HEIGHT; y++) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            if (gameboy.cgb)
                draw_color(y, x, background[(x + scx[y]) % 256][(y + scy[y]) % 256]);
            else
                draw_greyscale(y, x, background[(x + scx[y]) % 256][(y + scy[y]) % 256]);
        }
    }
}

static void render_window() {
    if (!window_display_enable())
        return;

    for (unsigned char y = 0; y < 32; y++) {
        for (unsigned char x = 0; x < 32; x++) {
            draw_tile(x, y, true, window);
        }
    }

    for (unsigned char y = 0; y < HEIGHT; y++) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            if (x + wx[y] >= 7 && x + wx[y] < WIDTH + 7 && y + wy[y] >= 0 && y + wy[y] < HEIGHT) {
                if (gameboy.cgb)
                    draw_color(y + wy[y], x + wx[y] - 7, window[x][y]);
                else
                    draw_greyscale(y + wy[y], x + wx[y] - 7, window[x][y]);
            }
        }
    }
}

static void render_sprites() {
    for (unsigned char i = 0; i < 0xA0; i += 4) {
        unsigned char y = read_mmu(0xFE00 + i);
        unsigned char x = read_mmu(0xFE00 + i + 1);
        unsigned char tile = read_mmu(0xFE00 + i + 2);
        unsigned char attr = read_mmu(0xFE00 + i + 3);

        if (obj_sprite_size() == 0) {
            // 8x8 sprite
            draw_sprite(x, y, 0x8000 + tile * 16, attr);
        } else {
            // 8x16 sprite
            draw_sprite(x, y, 0x8000 + (tile & 0xFE) * 16, attr);
            draw_sprite(x, y + 8, 0x8000 + (tile | 1) * 16, attr);
        }
    }
}

void draw() {
    render_bg();
    render_window();
    render_sprites();
}