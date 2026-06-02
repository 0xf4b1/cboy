// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cboy {
class MMU;
namespace display {

class Frame {
public:
	using Row = std::array<uint16_t, 160>;
	Frame() = default;
	Row& operator[](size_t y) { return m_data[y]; }
	const Row& operator[](size_t y) const { return m_data[y]; }
	Row& at(size_t y) { return m_data.at(y); }
	const Row& at(size_t y) const { return m_data.at(y); }

private:
	std::array<Row, 144> m_data{};
};

struct Renderer {
	virtual void present(const Frame& frame) = 0;
    // Called from the frontend's display callback to render current frame
    virtual void render() = 0;
    virtual ~Renderer() = default;
};

void set_params(const MMU& mmu, uint8_t scanline);
void draw(const MMU& mmu, Frame& framebuffer, bool is_cgb);
void toggle_fullscreen();

} // namespace cboy::display
} // namespace cboy
