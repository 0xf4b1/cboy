// SPDX-License-Identifier: GPL-3.0-only
// Test runner for Blargg cpu_instrs ROMs
// Exits 0 on PASSED, 1 on FAILED or timeout

use std::env;
use librsboy::Gameboy;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: rsboy-test <rom>");
        std::process::exit(1);
    }

    let mut gb = Gameboy::default();
    gb.load_rom(&args[1]);

    for _ in 0..2000 {
        gb.next_frame();

        // Check serial output for PASSED/FAILED
        for &b in &gb.mmu.serial_buf {
            if b == b'P' {
                println!("PASSED");
                std::process::exit(0);
            } else if b == b'F' {
                println!("FAILED");
                std::process::exit(1);
            }
        }
    }

    // Timeout
    std::process::exit(1);
}
