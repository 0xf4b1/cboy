// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

// Forward declare — each renderer_*.cpp includes the full gameboy.hpp itself.
namespace cboy {
class Gameboy;
namespace renderer {

// Each renderer implements this interface.
// run() opens a window, presents frames, and blocks until the user closes it.
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void run(Gameboy &gameboy) = 0;
};

// Factory function — implemented by whichever renderer_*.cpp is compiled in.
std::unique_ptr<IRenderer> create();

} // namespace renderer
} // namespace cboy
