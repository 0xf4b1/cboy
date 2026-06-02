// SPDX-License-Identifier: GPL-3.0-only
//
// Adapter that exposes libcboy's C API through the same cboy::Gameboy /
// cboy::Controls / cboy::display::Frame interface that the renderers use
// when built against libcppboy.
//
// Active when BACKEND=LIBCBOY.

#pragma once

#include <cstdint>
#include <cstring>
#include <string>

// libcboy headers — each has its own extern "C" guard.
#include <gameboy.h>   // ::load_rom, ::load_state, ::save_state, ::Gameboy global
#include <controls.h>  // ::press, ::release, ::release_all — plus macros RIGHT/LEFT/…
#include <cpu.h>       // ::next_frame(), ::Frame (via display.h)
#include <display.h>   // ::Frame typedef

// ---- Capture macro values before undefining them -------------------------
// controls.h defines RIGHT=0, LEFT=1, … as plain integer macros.  We need to
// capture those values into named constants before undefining the macros, so
// the scoped Button enum can reference them without the macro expansion
// turning the enumerator names into integer literals mid-parse.

namespace cboy_adapter_detail {
    static const int k_RIGHT  = RIGHT;
    static const int k_LEFT   = LEFT;
    static const int k_UP     = UP;
    static const int k_DOWN   = DOWN;
    static const int k_A      = CBOY_KEY_A;
    static const int k_B      = CBOY_KEY_B;
    static const int k_SELECT = SELECT;
    static const int k_START  = START;
} // namespace cboy_adapter_detail

#undef RIGHT
#undef LEFT
#undef UP
#undef DOWN
#undef CBOY_KEY_A
#undef CBOY_KEY_B
#undef SELECT
#undef START

// ---- cboy namespace types ------------------------------------------------

namespace cboy {

// ---- cboy::Button --------------------------------------------------------

enum class Button : uint8_t {
    RIGHT  = cboy_adapter_detail::k_RIGHT,
    LEFT   = cboy_adapter_detail::k_LEFT,
    UP     = cboy_adapter_detail::k_UP,
    DOWN   = cboy_adapter_detail::k_DOWN,
    A      = cboy_adapter_detail::k_A,
    B      = cboy_adapter_detail::k_B,
    SELECT = cboy_adapter_detail::k_SELECT,
    START  = cboy_adapter_detail::k_START,
};

// ---- cboy::display::Frame ------------------------------------------------
// Owns pixel data and exposes frame[y][x] (uint16_t) like libcppboy's Frame.

namespace display {

class Frame {
public:
    Frame() { std::memset(&m_data, 0, sizeof(m_data)); }

    void update(const ::Frame &src) { m_data = src; }

    const uint16_t *operator[](int y) const {
        return reinterpret_cast<const uint16_t *>(m_data.buffer[y]);
    }
    uint16_t *operator[](int y) {
        return reinterpret_cast<uint16_t *>(m_data.buffer[y]);
    }

private:
    ::Frame m_data{};
};

} // namespace display

// ---- cboy::Controls ------------------------------------------------------

class Controls {
public:
    void press(Button b)   { ::press(static_cast<unsigned char>(b));   }
    void release(Button b) { ::release(static_cast<unsigned char>(b)); }
    void release_all()     { ::release_all(); }
};

// ---- cboy::Gameboy --------------------------------------------------------

class Gameboy {
public:
    Gameboy() = default;
    Gameboy(const Gameboy &) = delete;
    Gameboy &operator=(const Gameboy &) = delete;

    void load_rom(const std::string &path) {
        ::load_rom(const_cast<char *>(path.c_str()));
    }
    void load_state() { ::load_state(); }
    void save_state() { ::save_state(); }

    const display::Frame &run_frame() {
        ::Frame c = ::next_frame();
        m_frame.update(c);
        return m_frame;
    }

    Controls &controls() { return m_controls; }

private:
    Controls       m_controls;
    display::Frame m_frame;
};

} // namespace cboy
