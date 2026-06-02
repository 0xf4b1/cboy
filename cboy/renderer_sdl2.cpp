// SPDX-License-Identifier: GPL-3.0-only
// SDL2 renderer — cross-platform, zero extra GPU API dependencies.
//
// Pipeline:
//   SDL_CreateTexture (SDL_PIXELFORMAT_ARGB8888, STREAMING)
//   SDL_LockTexture / fill / SDL_UnlockTexture  each frame
//   SDL_RenderCopy with a letterboxed destination rect
//
// SDL_PIXELFORMAT_ARGB8888 = 0xAARRGGBB in native byte order,
// which maps to the same layout as Direct3D BGRA on little-endian.

#include "gameboy.hpp"
#include "renderer_sdl2.hpp"
#include "controls.hpp"
#include "frame_pacer.hpp"

#include <SDL2/SDL.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace cboy {
namespace renderer {

static const int GB_W  = 160;
static const int GB_H  = 144;
static const int SCALE = 4;

// GB RGB555 → SDL ARGB8888 (0xFFRRGGBB)
static inline uint32_t rgb555_to_argb(uint16_t c) {
    uint8_t r = static_cast<uint8_t>(((c >>  0) & 0x1F) * 255 / 31);
    uint8_t g = static_cast<uint8_t>(((c >>  5) & 0x1F) * 255 / 31);
    uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
    return (0xFFu << 24) | (static_cast<uint32_t>(r) << 16)
                         | (static_cast<uint32_t>(g) <<  8)
                         |  static_cast<uint32_t>(b);
}

static Button sdl_key_to_button(SDL_Keycode k, bool &found) {
    found = true;
    switch (k) {
    case SDLK_RIGHT: return Button::RIGHT;
    case SDLK_LEFT:  return Button::LEFT;
    case SDLK_UP:    return Button::UP;
    case SDLK_DOWN:  return Button::DOWN;
    case SDLK_a:     return Button::A;
    case SDLK_s:     return Button::B;
    case SDLK_q:     return Button::START;
    case SDLK_w:     return Button::SELECT;
    default: found = false; return Button::A;
    }
}

// Compute a centered letterbox/pillarbox SDL_Rect at 160:144 aspect ratio.
static SDL_Rect letterbox(int win_w, int win_h) {
    SDL_Rect r;
    if (win_w * GB_H <= win_h * GB_W) {
        r.w = win_w;
        r.h = win_w * GB_H / GB_W;
    } else {
        r.h = win_h;
        r.w = win_h * GB_W / GB_H;
    }
    r.x = (win_w - r.w) / 2;
    r.y = (win_h - r.h) / 2;
    return r;
}

void SDL2Renderer::run(Gameboy &gameboy) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

    SDL_Window *window = SDL_CreateWindow(
        "cboy — SDL2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GB_W * SCALE, GB_H * SCALE,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    // Hardware-accelerated renderer — no vsync, FramePacer controls rate
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        // Fall back to software renderer if hardware isn't available
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer)
            throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    // Scale quality: nearest-neighbour for sharp pixel art
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    // Black bars on letterbox/pillarbox
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Streaming texture — we write into it every frame
    SDL_Texture *tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        GB_W, GB_H);
    if (!tex)
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());

    bool running = true;
    FramePacer pacer;
    while (running) {
        // --- Events ---
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                SDL_Keycode k = ev.key.keysym.sym;
                if (ev.type == SDL_KEYUP) {
                    if (k == SDLK_F5) { gameboy.load_state(); break; }
                    if (k == SDLK_F6) { gameboy.save_state(); break; }
                }
                bool found;
                Button btn = sdl_key_to_button(k, found);
                if (found) {
                    if (ev.type == SDL_KEYDOWN) gameboy.controls().press(btn);
                    else                        gameboy.controls().release(btn);
                }
                break;
            }
            default: break;
            }
        }
        if (!running) break;

        // --- Emulate one frame ---
        const display::Frame &frame = gameboy.run_frame();

        // --- Upload pixels ---
        void    *pixels;
        int      pitch;
        SDL_LockTexture(tex, nullptr, &pixels, &pitch);
        auto *dst = static_cast<uint32_t *>(pixels);
        int row_words = pitch / 4;
        for (int y = 0; y < GB_H; ++y)
            for (int x = 0; x < GB_W; ++x)
                dst[y * row_words + x] = rgb555_to_argb(frame[y][x]);
        SDL_UnlockTexture(tex);

        // --- Render ---
        SDL_RenderClear(renderer);  // fills with black (letterbox bars)

        int win_w, win_h;
        SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
        SDL_Rect dst_rect = letterbox(win_w, win_h);
        SDL_RenderCopy(renderer, tex, nullptr, &dst_rect);

        SDL_RenderPresent(renderer);
        pacer.wait();
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

std::unique_ptr<IRenderer> create() {
    return std::make_unique<SDL2Renderer>();
}

} // namespace renderer
} // namespace cboy
