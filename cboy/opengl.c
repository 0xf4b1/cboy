// SPDX-License-Identifier: GPL-3.0-only

#include <stdbool.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cpu.h>

#include "keyboard.h"

#define WIDTH 160
#define HEIGHT 144

int window_width = WIDTH;
int window_height = HEIGHT;

bool fullscreen = false;

Frame buffer;


static void draw_pixel(unsigned char x, unsigned char y, unsigned short color) {
    glPointSize(window_width / (float)WIDTH);
    glBegin(GL_POINTS);
    glColor3f((float)(color & 0x1f) / 32, (float)((color >> 5) & 0x1f) / 32, (float)((color >> 10) & 0x1f) / 32);
    glVertex2i(x, y);
    glEnd();
}

static void display() {
    window_width = glutGet(GLUT_WINDOW_WIDTH);
    window_height = glutGet(GLUT_WINDOW_HEIGHT);

    glLoadIdentity();
    glViewport((window_width - (window_height / HEIGHT) * WIDTH) / 2, 0, ((float)window_height / HEIGHT) * WIDTH,
               window_height);
    glOrtho(0.0f, WIDTH, HEIGHT, 0.0f, 0.0f, 1.0f);

    for (unsigned char y = 0; y < HEIGHT; y++) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            draw_pixel(x, y, buffer.buffer[y][x]);
        }
    }

    glutSwapBuffers();
}

static void idle_func() {
    buffer = next_frame();
    glutPostRedisplay();
}

void display_loop() {
    int argc = 0;
    glutInit(&argc, 0);
    glutInitDisplayMode(GL_DOUBLE);
    glutInitWindowSize(WIDTH * 4, HEIGHT * 4);
    glutCreateWindow("cboy");
    glutDisplayFunc(display);
    glutIdleFunc(idle_func);
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