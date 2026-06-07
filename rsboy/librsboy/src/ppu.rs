// SPDX-License-Identifier: GPL-3.0-only

use crate::gameboy::Gameboy;
use crate::display::{WIDTH, HEIGHT};

impl Gameboy {
    pub fn set_params(&mut self, i: usize) {
        self.scy[i] = self.read_mmu(0xFF42);
        self.scx[i] = self.read_mmu(0xFF43);
        self.wy[i]  = self.read_mmu(0xFF4A);
        self.wx[i]  = self.read_mmu(0xFF4B);
    }

    fn draw_greyscale(&mut self, x: usize, y: usize, color: u8) {
        self.framebuffer.buffer[x][y] = match color {
            0 => 0xFFFF,
            1 => 0x4210,
            2 => 0x2108,
            _ => 0x0000,
        };
    }

    fn draw_color(&mut self, x: usize, y: usize, color: u16) {
        self.framebuffer.buffer[x][y] = color;
    }

    fn get_color_palette_bg(&self, palette_num: u8, color_idx: u8) -> u16 {
        let base = (palette_num as usize) * 8 + (color_idx as usize) * 2;
        if base + 1 < self.mmu.bg_palette.len() {
            (self.mmu.bg_palette[base] as u16) | ((self.mmu.bg_palette[base + 1] as u16) << 8)
        } else {
            0
        }
    }

    fn get_color_palette_sprite(&self, palette_num: u8, color_idx: u8) -> u16 {
        let base = (palette_num as usize) * 8 + (color_idx as usize) * 2;
        if base + 1 < self.mmu.sprite_palette.len() {
            (self.mmu.sprite_palette[base] as u16) | ((self.mmu.sprite_palette[base + 1] as u16) << 8)
        } else {
            0
        }
    }

    fn draw_sprite(&mut self, offset_x: u8, offset_y: u8, tile_offset: u16, attr: u8) {
        let palette_number = attr & 3;
        let vram_bank = attr >> 3 & 1 != 0;
        let obp1 = attr >> 4 & 1 != 0;
        let x_flip = attr >> 5 & 1 != 0;
        let y_flip = attr >> 6 & 1 != 0;

        let palette = if !self.mmu.cgb() {
            self.read_mmu(if obp1 { 0xFF49 } else { 0xFF48 })
        } else { 0 };

        for py in 0u8..8 {
            for px in 0u8..8 {
                let screen_x = offset_x as i16 + px as i16 - 8;
                let screen_y = offset_y as i16 + py as i16 - 16;

                if screen_x < 0 || screen_y < 0 || screen_x >= WIDTH as i16 || screen_y >= HEIGHT as i16 {
                    continue;
                }

                let ty = if y_flip { 7 - py } else { py };
                let tx = if x_flip { px } else { 7 - px };

                let offset = (ty as u16) * 2 + tile_offset - 0x8000;
                let (byte0, byte1) = if !vram_bank {
                    (self.mmu.ram[offset as usize], self.mmu.ram[offset as usize + 1])
                } else {
                    (self.mmu.vram_bank[offset as usize], self.mmu.vram_bank[offset as usize + 1])
                };

                let color_idx = ((byte0 >> tx) & 1) | (((byte1 >> tx) & 1) << 1);

                if color_idx == 0 { continue; } // transparent

                let sx = screen_y as usize;
                let sy = screen_x as usize;

                if self.mmu.cgb() {
                    let c = self.get_color_palette_sprite(palette_number, color_idx);
                    self.draw_color(sx, sy, c);
                } else {
                    let c = (palette >> (color_idx * 2)) & 3;
                    self.draw_greyscale(sx, sy, c);
                }
            }
        }
    }

    fn draw_tile(&self, offset_x: u8, offset_y: u8, window: bool, buffer: &mut [[u16; 256]; 256]) {
        let tile_addr = self.get_tile(offset_x, offset_y, window);
        let map_select = if window { self.window_tile_map_display_select() } else { self.bg_tile_map_display_select() };
        let attr_addr = (if map_select { 0x9C00u16 } else { 0x9800u16 }) + offset_y as u16 * 32 + offset_x as u16;
        let attr = self.mmu.vram_bank[(attr_addr - 0x8000) as usize];

        let palette_num = attr & 7;
        let use_vram_bank = attr >> 3 & 1 != 0;
        let x_flip = attr >> 5 & 1 != 0;
        let y_flip = attr >> 6 & 1 != 0;

        let palette = if !self.mmu.cgb() { self.read_mmu(0xFF47) } else { 0 };

        for ty in 0u8..8 {
            let real_ty = if y_flip { 7 - ty } else { ty };
            let offset = (real_ty as u16) * 2 + tile_addr - 0x8000;

            let (byte0, byte1) = if use_vram_bank && self.mmu.cgb() {
                (self.mmu.vram_bank[offset as usize], self.mmu.vram_bank[offset as usize + 1])
            } else {
                (self.mmu.ram[offset as usize], self.mmu.ram[offset as usize + 1])
            };

            for tx in 0u8..8 {
                let bit = if x_flip { tx } else { 7 - tx };
                let color_idx = ((byte0 >> bit) & 1) | (((byte1 >> bit) & 1) << 1);

                let bx = offset_x as usize * 8 + tx as usize;
                let by = offset_y as usize * 8 + ty as usize;

                if self.mmu.cgb() {
                    let c = self.get_color_palette_bg(palette_num, color_idx);
                    buffer[bx][by] = c;
                } else {
                    buffer[bx][by] = ((palette >> (color_idx * 2)) & 3) as u16;
                }
            }
        }
    }

    pub fn draw(&mut self) {
        let mut background = Box::new([[0u16; 256]; 256]);
        let mut window_buf = Box::new([[0u16; 256]; 256]);

        // Render background tiles
        for ty in 0u8..32 {
            for tx in 0u8..32 {
                self.draw_tile(tx, ty, false, &mut background);
            }
        }

        // Apply background to framebuffer
        for y in 0..HEIGHT {
            for x in 0..WIDTH {
                let bx = (x as u16 + self.scx[y] as u16) as usize % 256;
                let by = (y as u16 + self.scy[y] as u16) as usize % 256;
                if self.mmu.cgb() {
                    let c = background[bx][by];
                    self.draw_color(y, x, c);
                } else {
                    self.draw_greyscale(y, x, background[bx][by] as u8);
                }
            }
        }

        // Render window
        if self.window_display_enable() {
            for ty in 0u8..32 {
                for tx in 0u8..32 {
                    self.draw_tile(tx, ty, true, &mut window_buf);
                }
            }

            for y in 0..HEIGHT {
                for x in 0..WIDTH {
                    let wx = self.wx[y];
                    let wy = self.wy[y];
                    if (x as u16 + wx as u16) >= 7
                        && (x as u16 + wx as u16) < (WIDTH as u16 + 7)
                        && (y as u16 + wy as u16) < HEIGHT as u16
                    {
                        let dest_x = x + wx as usize - 7;
                        let dest_y = y + wy as usize;
                        if dest_x < WIDTH && dest_y < HEIGHT {
                            if self.mmu.cgb() {
                                let c = window_buf[x][y];
                                self.draw_color(dest_y, dest_x, c);
                            } else {
                                self.draw_greyscale(dest_y, dest_x, window_buf[x][y] as u8);
                            }
                        }
                    }
                }
            }
        }

        // Render sprites
        for i in (0..0xA0usize).step_by(4) {
            let sy = self.read_mmu(0xFE00 + i as u16);
            let sx = self.read_mmu(0xFE01 + i as u16);
            let tile = self.read_mmu(0xFE02 + i as u16);
            let attr = self.read_mmu(0xFE03 + i as u16);

            if !self.obj_sprite_size() {
                self.draw_sprite(sx, sy, 0x8000 + tile as u16 * 16, attr);
            } else {
                self.draw_sprite(sx, sy, 0x8000 + (tile & 0xFE) as u16 * 16, attr);
                self.draw_sprite(sx, sy.wrapping_add(8), 0x8000 + (tile | 1) as u16 * 16, attr);
            }
        }
    }

    pub fn next_frame(&mut self) -> crate::display::Frame {
        while !self.lcd_display_enable() {
            self.set_mode(0);
            self.write_mmu(0xFF44, 0);
            for _ in 0..154 {
                self.next_instructions(456);
            }
        }

        for i in 0..144u8 {
            self.set_ly(i);
            self.set_mode(2);
            self.next_instructions(80);
            self.set_mode(3);
            self.next_instructions(172);
            self.set_params(i as usize);
            self.set_mode(0);
            self.next_instructions(204);
        }

        self.set_vblank();
        self.draw();

        for i in 144..=154u8 {
            self.set_ly(i);
            self.set_mode(1);
            self.next_instructions(456);
        }

        self.framebuffer
    }
}
