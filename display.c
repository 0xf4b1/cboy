// SPDX-License-Identifier: GPL-3.0-only

#include <GL/gl.h>
#include <GL/glut.h>

#include "controls.h"
#include "cpu.h"
#include "gameboy.h"

#define WIDTH 160
#define HEIGHT 144

int window_width = WIDTH;
int window_height = HEIGHT;

bool fullscreen = false;

unsigned char background[256][256];

unsigned char scy[256] = {[0 ... 0xFF] = 0};
unsigned char scx[256] = {[0 ... 0xFF] = 0};

void set_params(unsigned char i) {
    scy[i] = read_mmu(0xFF42);
    scx[i] = read_mmu(0xFF43);
}

void draw_pixel(unsigned char x, unsigned char y, unsigned char r, unsigned char g, unsigned char b) {
    if (y > 160 || x > 144) {
        return;
    }

    glPointSize(window_width / (float)WIDTH);
    glBegin(GL_POINTS);
    glColor3f((float)r / 255, (float)g / 255, (float)b / 255);
    glVertex2i(y, x);
    glEnd();
}

void draw_color(unsigned char x, unsigned char y, unsigned char color) {
    if (color == 0) {
        draw_pixel(x, y, 255, 255, 255);
    } else if (color == 1) {
        draw_pixel(x, y, 128, 128, 128);
    } else if (color == 2) {
        draw_pixel(x, y, 64, 64, 64);
    } else if (color == 3) {
        draw_pixel(x, y, 0, 0, 0);
    }
}

void draw_tile(unsigned char offset_x, unsigned char offset_y, unsigned short tile_offset, bool x_flip, bool y_flip) {
    unsigned char palette = read_mmu(0xFF48);

    for (unsigned char y = 0; y < 8; y++) {
        for (unsigned char x = 0; x < 8; x++) {
            unsigned char color =
                ((read_mmu((y_flip ? 7 - y : y) * 2 + tile_offset) >> (x_flip ? x : (7 - x))) & 1) << 1 |
                ((read_mmu((y_flip ? 7 - y : y) * 2 + 1 + tile_offset) >> (x_flip ? x : (7 - x))) & 1);

            if (color == 0) {
                continue;
            }

            color = (palette >> (color * 2)) & 3;
            draw_color(offset_y + y, offset_x + x, color);
        }
    }
}

void draw_bg_tile(unsigned char offset_x, unsigned char offset_y, unsigned short tile_addr) {
    unsigned char palette = read_mmu(0xFF47);

    for (unsigned char y = 0; y < 8; y++) {
        for (unsigned char x = 0; x < 8; x++) {
            unsigned char color = ((read_mmu(y * 2 + tile_addr) >> (7 - x)) & 1) << 1 |
                                  ((read_mmu(y * 2 + 1 + tile_addr) >> (7 - x)) & 1);

            color = (palette >> (color * 2)) & 3;
            background[offset_x + x][offset_y + y] = color;
        }
    }
}

void draw_visible_bg_area() {
    for (unsigned char y = 0; y < HEIGHT; y++) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            draw_color(y, x, background[(x + scx[y]) % 256][(y + scy[y]) % 256]);
        }
    }
}

void render_bg() {
    for (unsigned char y = 0; y < 32; y++) {
        for (unsigned char x = 0; x < 32; x++) {
            draw_bg_tile(x * 8, y * 8, get_bg_tile(x, y));
        }
    }

    draw_visible_bg_area();
}

void render_sprites() {
    for (unsigned char i = 0; i < 0xA0; i += 4) {
        unsigned char y = read_mmu(0xFE00 + i) - 16;
        unsigned char x = read_mmu(0xFE00 + i + 1) - 8;
        unsigned char tile = read_mmu(0xFE00 + i + 2);
        unsigned char attr = read_mmu(0xFE00 + i + 3);
        bool x_flip = attr >> 5 & 1;
        bool y_flip = attr >> 6 & 1;

        // hidden sprites
        if (y < 0 || y > 144 || x < 0 || x > 160) {
            continue;
        }

        draw_tile(x, y, 0x8000 + tile * 16, x_flip, y_flip);
    }
}

void draw() { glutPostRedisplay(); }

void display() {
    window_width = glutGet(GLUT_WINDOW_WIDTH);
    window_height = glutGet(GLUT_WINDOW_HEIGHT);

    glLoadIdentity();
    glViewport((window_width - (window_height / HEIGHT) * WIDTH) / 2, 0, ((float)window_height / HEIGHT) * WIDTH,
               window_height);
    glOrtho(0.0f, WIDTH, HEIGHT, 0.0f, 0.0f, 1.0f);

    render_bg();
    render_sprites();

    glutSwapBuffers();
}

void display_loop() {
    glutInitDisplayMode(GL_DOUBLE);
    glutInitWindowSize(WIDTH * 4, HEIGHT * 4);
    glutCreateWindow("cboy");
    glutDisplayFunc(display);
    glutIdleFunc(next_frame);
    glutSpecialFunc(special_key_handler);
    glutSpecialUpFunc(special_key_up_handler);
    glutKeyboardFunc(normal_key_handler);
    glutKeyboardUpFunc(normal_key_up_handler);
    glutMainLoop();
}

void toggle_fullscreen() {
    if (!fullscreen) {
        glutFullScreen();
    } else {
        glutPositionWindow(0, 0);
        glutReshapeWindow(WIDTH * 4, HEIGHT * 4);
    }
    fullscreen = !fullscreen;
}