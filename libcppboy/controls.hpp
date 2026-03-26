// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>

namespace cboy {

enum class Button : uint8_t {
    RIGHT = 0,
    LEFT = 1,
    UP = 2,
    DOWN = 3,
    A = 4,
    B = 5,
    SELECT = 6,
    START = 7
};

class Controls {
public:
    void press(Button button);
    void release(Button button);
    void release_all();
    
    uint8_t get_state() const { return m_state; }
    
private:
    uint8_t m_state = 0xFF;
};

} // namespace cboy
