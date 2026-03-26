// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>

namespace cboy {
class CPU;
class MMU;
}

namespace cboy::instructions {

uint8_t execute(uint8_t opcode, cboy::CPU& cpu, cboy::MMU& mmu);

namespace cb {
    // Execute a CB-prefixed instruction
    uint8_t execute(uint8_t opcode, cboy::CPU& cpu, cboy::MMU& mmu);
} // namespace cb

} // namespace cboy::instructions
