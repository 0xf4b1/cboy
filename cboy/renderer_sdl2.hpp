// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "renderer.hpp"

namespace cboy {
namespace renderer {

class SDL2Renderer final : public IRenderer {
public:
    void run(Gameboy &gameboy) override;
};

} // namespace renderer
} // namespace cboy
