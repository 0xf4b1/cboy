// SPDX-License-Identifier: GPL-3.0-only

#include "controls.hpp"

namespace cboy {

void Controls::press(Button button) {
    m_state &= ~(1 << static_cast<uint8_t>(button));
}

void Controls::release(Button button) {
    m_state |= (1 << static_cast<uint8_t>(button));
}

void Controls::release_all() {
    m_state = 0xFF;
}

} // namespace cboy
