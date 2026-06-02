// SPDX-License-Identifier: GPL-3.0-only
// GDI renderer — blits pixels to the window via StretchDIBits.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>   // timeBeginPeriod / timeEndPeriod

#include <cstdint>
#include <memory>

#include "gameboy.hpp"
#include "renderer_gdi.hpp"
#include "controls.hpp"

namespace cboy {
namespace renderer {

static const int GB_W  = 160;
static const int GB_H  = 144;
static const int SCALE = 4;

static bool vkey_to_button(int vkey, Button &out) {
    switch (vkey) {
    case VK_RIGHT: out = Button::RIGHT;  return true;
    case VK_LEFT:  out = Button::LEFT;   return true;
    case VK_UP:    out = Button::UP;     return true;
    case VK_DOWN:  out = Button::DOWN;   return true;
    case 'A':      out = Button::A;      return true;
    case 'S':      out = Button::B;      return true;
    case 'Q':      out = Button::START;  return true;
    case 'W':      out = Button::SELECT; return true;
    default:       return false;
    }
}

struct WindowState {
    Gameboy *gameboy = nullptr;
    bool     running = true;
};

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto *s = reinterpret_cast<WindowState *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (s && s->gameboy) {
            if (wp == VK_F5 && msg == WM_KEYUP) { s->gameboy->load_state(); return 0; }
            if (wp == VK_F6 && msg == WM_KEYUP) { s->gameboy->save_state(); return 0; }
            Button btn;
            if (vkey_to_button(static_cast<int>(wp), btn)) {
                if (msg == WM_KEYDOWN) s->gameboy->controls().press(btn);
                else                   s->gameboy->controls().release(btn);
            }
        }
        return 0;
    case WM_DESTROY:
        if (s) s->running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// GB RGB555 → GDI RGBX8888 (0x00RRGGBB stored little-endian = BBGGRR00 in memory,
// but StretchDIBits BI_RGB 32bpp expects 0x00RRGGBB in the DWORD value).
static inline uint32_t rgb555_to_rgbx(uint16_t c) {
    uint8_t r = static_cast<uint8_t>(((c >>  0) & 0x1F) * 255 / 31);
    uint8_t g = static_cast<uint8_t>(((c >>  5) & 0x1F) * 255 / 31);
    uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
    // BI_RGB DIBs: blue in bits 0-7, green 8-15, red 16-23 (little-endian DWORD)
    return (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) <<  8) |
            static_cast<uint32_t>(b);
}

void GDIRenderer::run(Gameboy &gameboy) {
    // Raise timer resolution so Sleep(1) is accurate enough for pacing
    timeBeginPeriod(1);

    const wchar_t CLASS[] = L"cboy_gdi";
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = wnd_proc;
    wc.lpszClassName = CLASS;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    RECT r = {0, 0, GB_W * SCALE, GB_H * SCALE};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    WindowState state{&gameboy, true};
    HWND hwnd = CreateWindowExW(0, CLASS, L"cboy — GDI",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                r.right - r.left, r.bottom - r.top,
                                nullptr, nullptr, nullptr, nullptr);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    HDC hdc = GetDC(hwnd);

    // DIB header — 32bpp top-down, BI_RGB (blue in low byte of each DWORD)
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = GB_W;
    bmi.bmiHeader.biHeight      = -GB_H; // negative = top-down scanlines
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    auto pixels = std::make_unique<uint32_t[]>(static_cast<size_t>(GB_W * GB_H));

    // Target ~60 fps using QueryPerformanceCounter for accurate pacing.
    LARGE_INTEGER freq, last_time;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last_time);
    const double frame_ns = 1.0e9 / 60.0; // nanoseconds per frame

    MSG msg = {};
    while (state.running) {
        // Drain message queue
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { state.running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!state.running) break;

        // Run one GB frame
        const display::Frame &frame = gameboy.run_frame();

        // Convert RGB555 → GDI RGBX
        for (int y = 0; y < GB_H; ++y)
            for (int x = 0; x < GB_W; ++x)
                pixels[y * GB_W + x] = rgb555_to_rgbx(frame[y][x]);

        // Letterbox / pillarbox: find the largest centered rect at 160:144
        RECT cr; GetClientRect(hwnd, &cr);
        int win_w = cr.right;
        int win_h = cr.bottom;
        int dst_w, dst_h, dst_x, dst_y;
        if (win_w * GB_H <= win_h * GB_W) {
            dst_w = win_w;
            dst_h = win_w * GB_H / GB_W;
        } else {
            dst_h = win_h;
            dst_w = win_h * GB_W / GB_H;
        }
        dst_x = (win_w - dst_w) / 2;
        dst_y = (win_h - dst_h) / 2;

        // Fill bars with black if the window is larger than the game rect
        if (dst_x > 0 || dst_y > 0) {
            HBRUSH black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
            if (dst_y > 0) {
                RECT top    = {0,            0,     win_w, dst_y};
                RECT bottom = {0, dst_y + dst_h, win_w, win_h};
                FillRect(hdc, &top,    black);
                FillRect(hdc, &bottom, black);
            }
            if (dst_x > 0) {
                RECT left  = {0,            dst_y,       dst_x, dst_y + dst_h};
                RECT right = {dst_x + dst_w, dst_y, win_w, dst_y + dst_h};
                FillRect(hdc, &left,  black);
                FillRect(hdc, &right, black);
            }
        }

        StretchDIBits(hdc,
                      dst_x, dst_y, dst_w, dst_h,
                      0,     0,     GB_W,  GB_H,
                      pixels.get(), &bmi, DIB_RGB_COLORS, SRCCOPY);

        // Pace to ~60 fps: spin-wait the remaining time in the frame budget
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double elapsed_ns = static_cast<double>(now.QuadPart - last_time.QuadPart)
                            * 1.0e9 / static_cast<double>(freq.QuadPart);
        double remaining_ns = frame_ns - elapsed_ns;
        if (remaining_ns > 2.0e6) {
            // Sleep for most of the wait (leave 2 ms for spin)
            Sleep(static_cast<DWORD>((remaining_ns - 2.0e6) / 1.0e6));
        }
        // Spin for the last couple of milliseconds for precision
        do {
            QueryPerformanceCounter(&now);
            elapsed_ns = static_cast<double>(now.QuadPart - last_time.QuadPart)
                         * 1.0e9 / static_cast<double>(freq.QuadPart);
        } while (elapsed_ns < frame_ns);
        last_time = now;
    }

    timeEndPeriod(1);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    UnregisterClassW(CLASS, nullptr);
}

std::unique_ptr<IRenderer> create() {
    return std::make_unique<GDIRenderer>();
}

} // namespace renderer
} // namespace cboy
