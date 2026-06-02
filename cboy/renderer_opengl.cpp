// SPDX-License-Identifier: GPL-3.0-only
// OpenGL renderer — uses GLFW for windowing (cross-platform).

#include "gameboy.hpp"
#include "renderer_opengl.hpp"
#include "controls.hpp"
#include "frame_pacer.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace cboy {
namespace renderer {

static const int GB_W  = 160;
static const int GB_H  = 144;
static const int SCALE = 4;

// Convert a GLFW key to a GB Button.
static bool glfw_key_to_button(int key, Button &out) {
    switch (key) {
    case GLFW_KEY_RIGHT: out = Button::RIGHT;  return true;
    case GLFW_KEY_LEFT:  out = Button::LEFT;   return true;
    case GLFW_KEY_UP:    out = Button::UP;     return true;
    case GLFW_KEY_DOWN:  out = Button::DOWN;   return true;
    case GLFW_KEY_A:     out = Button::A;      return true;
    case GLFW_KEY_S:     out = Button::B;      return true;
    case GLFW_KEY_Q:     out = Button::START;  return true;
    case GLFW_KEY_W:     out = Button::SELECT; return true;
    default:             return false;
    }
}

static void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto *gb = static_cast<Gameboy *>(glfwGetWindowUserPointer(window));
    if (!gb || action == GLFW_REPEAT) return;

    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_F5) { gb->load_state(); return; }
        if (key == GLFW_KEY_F6) { gb->save_state(); return; }
    }

    Button btn;
    if (glfw_key_to_button(key, btn)) {
        if (action == GLFW_PRESS)   gb->controls().press(btn);
        else                        gb->controls().release(btn);
    }
}

// Convert GB RGB555 frame to RGBA8 pixel buffer.
static void frame_to_rgba(const display::Frame &frame, std::vector<uint8_t> &out) {
    out.resize(GB_W * GB_H * 4);
    for (int y = 0; y < GB_H; ++y) {
        for (int x = 0; x < GB_W; ++x) {
            uint16_t c = frame[y][x];
            uint8_t r = static_cast<uint8_t>(((c >>  0) & 0x1F) * 255 / 31);
            uint8_t g = static_cast<uint8_t>(((c >>  5) & 0x1F) * 255 / 31);
            uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
            size_t i = static_cast<size_t>((y * GB_W + x) * 4);
            out[i]   = r;
            out[i+1] = g;
            out[i+2] = b;
            out[i+3] = 0xFF;
        }
    }
}

void OpenGLRenderer::run(Gameboy &gameboy) {
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");

    // Request legacy OpenGL 2.1 — available everywhere and enough for a textured quad.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow *window = glfwCreateWindow(GB_W * SCALE, GB_H * SCALE,
                                          "cboy — OpenGL", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwSetWindowUserPointer(window, &gameboy);
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // disable vsync — FramePacer controls rate

    // --- Texture setup ---
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GB_W, GB_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Ortho projection matching the GB native resolution
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, GB_W, GB_H, 0, -1, 1); // y-down to match GB convention
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    std::vector<uint8_t> pixels;
    FramePacer pacer;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const display::Frame &frame = gameboy.run_frame();
        frame_to_rgba(frame, pixels);

        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GB_W, GB_H,
                        GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // Fit the texture to the current window size
        int win_w, win_h;
        glfwGetFramebufferSize(window, &win_w, &win_h);
        glViewport(0, 0, win_w, win_h);

        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(0,    0   );
            glTexCoord2f(1, 0); glVertex2f(GB_W, 0   );
            glTexCoord2f(1, 1); glVertex2f(GB_W, GB_H);
            glTexCoord2f(0, 1); glVertex2f(0,    GB_H);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);
        pacer.wait();
    }

    glDeleteTextures(1, &tex);
    glfwDestroyWindow(window);
    glfwTerminate();
}

std::unique_ptr<IRenderer> create() {
    return std::make_unique<OpenGLRenderer>();
}

} // namespace renderer
} // namespace cboy
