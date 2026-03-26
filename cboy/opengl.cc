// SPDX-License-Identifier: GPL-3.0-only

#include <stdbool.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/OpenGL.h>
#else
#include <GL/glut.h>
#endif

#include "gameboy.hpp"
#include <cpu.hpp>
#include <display.hpp>
#include "app_context.hpp"
#include "keyboard.h"
#include <iostream>
#include <chrono>
#include <thread>

#define WIDTH 160
#define HEIGHT 144
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)  // ~16.67ms per frame

int window_width = WIDTH;
int window_height = HEIGHT;

bool fullscreen = false;
static auto last_frame_time = std::chrono::high_resolution_clock::now();
static int fps_counter = 0;
static long fps_accumulator_ms = 0;
static int current_fps = 0;

static cboy::Gameboy* s_gameboy = nullptr;
static cboy::display::Renderer* s_renderer = nullptr;

#include "renderer_opengl.hpp"
using cboy::renderer::OpenGLRenderer;

static void display() {
    if (s_renderer) {
        s_renderer->render();
    }
    glutSwapBuffers();
}

static void idle_func() {
    if (!s_gameboy) return;
    
    // Framerate limiting: target 60 FPS
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame_time).count();
    
    if (elapsed_ms < FRAME_TIME_MS) {
        // Sleep to maintain target FPS
        std::chrono::milliseconds sleep_time(FRAME_TIME_MS - elapsed_ms);
        std::this_thread::sleep_for(sleep_time);
        return;
    }
    
    last_frame_time = now;
    
    if (s_renderer) {
        s_renderer->present(s_gameboy->run_frame());
    }
    
    // Update FPS counter
    fps_counter++;
    fps_accumulator_ms += elapsed_ms;
    
    if (fps_accumulator_ms >= 1000) {
        current_fps = fps_counter;
        fps_counter = 0;
        fps_accumulator_ms = 0;
        
        // Update window title with FPS
        char title[64];
        snprintf(title, sizeof(title), "cboy - %d FPS", current_fps);
        glutSetWindowTitle(title);
    }
    
    glutPostRedisplay();
}

namespace cboy {

void set_app_context(Gameboy* gb, display::Renderer* r) {
    s_gameboy = gb;
    s_renderer = r;
}

Gameboy* get_app_gameboy() {
    return s_gameboy;
}

display::Renderer* get_app_renderer() {
    return s_renderer;
}

void display_loop(Gameboy& gameboy, display::Renderer& renderer) {
    set_app_context(&gameboy, &renderer);

    int argc = 0;
    glutInit(&argc, 0);
    glutInitDisplayMode(GL_DOUBLE);
    glutInitWindowSize(WIDTH * 4, HEIGHT * 4);
    glutCreateWindow("cboy");

#if defined(FREEGLUT)
    glutSwapInterval(0);
#elif defined(__APPLE__)
    {
        GLint vsync = 0;
        CGLContextObj ctx = CGLGetCurrentContext();
        if (ctx) {
            CGLSetParameter(ctx, kCGLCPSwapInterval, &vsync);
        }
    }
#endif

    // Initialize projection matrix once
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, WIDTH, HEIGHT, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, WIDTH * 4, HEIGHT * 4);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);

    glutDisplayFunc(::display);
    glutIdleFunc(idle_func);
    glutSpecialFunc(special_key_handler);
    glutSpecialUpFunc(special_key_up_handler);
    glutKeyboardFunc(normal_key_handler);
    glutKeyboardUpFunc(normal_key_up_handler);
    glutMainLoop();
}

} // namespace cboy

void toggle_fullscreen() {
    if (!fullscreen) {
        glutFullScreen();
    } else {
        glutPositionWindow(0, 0);
        glutReshapeWindow(WIDTH * 4, HEIGHT * 4);
    }
    fullscreen = !fullscreen;
}