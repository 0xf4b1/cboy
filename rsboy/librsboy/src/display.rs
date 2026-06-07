// SPDX-License-Identifier: GPL-3.0-only

pub const WIDTH: usize = 160;
pub const HEIGHT: usize = 144;

#[derive(Clone, Copy)]
pub struct Frame {
    pub buffer: [[u16; WIDTH]; HEIGHT],
}

impl Default for Frame {
    fn default() -> Self {
        Self {
            buffer: [[0u16; WIDTH]; HEIGHT],
        }
    }
}
