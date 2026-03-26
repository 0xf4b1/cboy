// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "gameboy.hpp"
#include "display.hpp"

namespace cboy {

void set_app_context(Gameboy* gb, display::Renderer* r);
Gameboy* get_app_gameboy();
display::Renderer* get_app_renderer();
void display_loop(Gameboy& gameboy, display::Renderer& renderer);

} // namespace cboy
