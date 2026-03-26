// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <display.hpp>
#include <vector>
#include <cstdint>

namespace cboy {
namespace renderer {

class OpenGLRenderer : public display::Renderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override;
    void present(const display::Frame& frame) override;
    void render() override;

private:
    unsigned int tex_id = 0;
    std::vector<uint8_t> pixels;
    bool initialized = false;
    bool dirty = false;
    int tex_width = 160;
    int tex_height = 144;
};

} // namespace renderer
} // namespace cboy
