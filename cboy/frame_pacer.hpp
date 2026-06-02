// SPDX-License-Identifier: GPL-3.0-only
//
// FramePacer — portable, high-resolution frame rate limiter.
//
// Usage:
//   FramePacer pacer;          // defaults to GB_FPS (~59.7275 Hz)
//   while (running) {
//       run_one_frame();
//       pacer.wait();          // sleeps the remaining frame budget
//   }
//
// On Windows the system timer is raised to 1 ms precision automatically.
// The implementation uses a sleep-then-spin strategy: sleep for most of the
// budget, then busy-wait the last 500 µs for sub-millisecond accuracy without
// burning the whole CPU time.

#pragma once

#include <chrono>
#include <thread>

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <timeapi.h>
#endif

namespace cboy {

class FramePacer {
public:
    // The real Game Boy runs at 4,194,304 Hz / 70,224 cycles = 59.7275... fps.
    static constexpr double GB_FPS = 4194304.0 / 70224.0;

    explicit FramePacer(double target_fps = GB_FPS)
        : m_frame_duration(std::chrono::duration<double>(1.0 / target_fps))
        , m_next_frame(clock::now())
    {
#if defined(_WIN32)
        timeBeginPeriod(1);
#endif
    }

    ~FramePacer() {
#if defined(_WIN32)
        timeEndPeriod(1);
#endif
    }

    // Call once per frame after running the emulator step.
    // Returns the actual elapsed time for this frame (useful for stats).
    double wait() {
        using namespace std::chrono;

        // Convert the target frame duration to the clock's native duration
        auto frame_ticks = duration_cast<clock::duration>(m_frame_duration);
        auto wake        = m_next_frame + frame_ticks;

        // Sleep for the bulk of the remaining time
        auto sleep_until = wake - microseconds(500);
        auto now         = clock::now();
        if (sleep_until > now)
            std::this_thread::sleep_until(sleep_until);

        // Spin-wait the last 500 µs for precision
        while (clock::now() < wake) {}

        auto actual_now = clock::now();
        duration<double> elapsed = actual_now - m_next_frame;
        m_next_frame = wake; // advance by fixed step to prevent drift

        return elapsed.count();
    }

    // Reset the pacer (e.g. after a long pause or window resize).
    void reset() { m_next_frame = clock::now(); }

    void set_fps(double fps) {
        m_frame_duration = std::chrono::duration<double>(1.0 / fps);
        reset();
    }

private:
    using clock = std::chrono::steady_clock;

    std::chrono::duration<double> m_frame_duration;
    clock::time_point             m_next_frame;
};

} // namespace cboy
