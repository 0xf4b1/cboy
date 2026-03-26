// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>

namespace cboy {

class MMU;
void timer(MMU& mmu, uint8_t cycles);

} // namespace cboy
