// SPDX-License-Identifier: GPL-3.0-only

#include "renderer_opengl.hpp"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cstring>

namespace cboy {
namespace renderer {

OpenGLRenderer::OpenGLRenderer() : tex_id(0), initialized(false), dirty(false) {
    pixels.reserve(160 * 144 * 3);
}
OpenGLRenderer::~OpenGLRenderer() {
    if (initialized && tex_id) {
        unsigned int id = tex_id;
        glDeleteTextures(1, &id);
    }
}

static void convert_frame_to_rgb(const display::Frame& frame, std::vector<uint8_t>& out) {
    if (out.size() != 160 * 144 * 3) {
        out.resize(160 * 144 * 3);
    }
    size_t idx = 0;
    for (int y = 0; y < 144; ++y) {
        const auto& row = frame[y];  // Use operator[] to avoid bounds checking
        for (int x = 0; x < 160; ++x) {
            uint16_t c = row[x];  // Direct access to pixel
            // 5-5-5 format assumed
            uint8_t r = static_cast<uint8_t>(((c >> 0) & 0x1F) * 255 / 31);
            uint8_t g = static_cast<uint8_t>(((c >> 5) & 0x1F) * 255 / 31);
            uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
            out[idx++] = r;
            out[idx++] = g;
            out[idx++] = b;
        }
    }
}

void OpenGLRenderer::present(const display::Frame& frame) {
    convert_frame_to_rgb(frame, pixels);
    dirty = true;
}

void OpenGLRenderer::render() {
    if (!initialized) {
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        initialized = true;
    }

    if (dirty) {
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        dirty = false;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(160.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(160.0f, 144.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 144.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

} // namespace renderer
} // namespace cboy
