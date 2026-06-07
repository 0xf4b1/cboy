// SPDX-License-Identifier: GPL-3.0-only

use std::env;
use librsboy::{Gameboy, controls};
use winit::{
    event::{Event, WindowEvent, KeyboardInput, ElementState, VirtualKeyCode},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};
use pixels::{Pixels, SurfaceTexture};

const WIDTH: u32 = 160;
const HEIGHT: u32 = 144;
const SCALE: u32 = 4;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: rsboy <rom>");
        std::process::exit(1);
    }

    let mut gb = Box::new(Gameboy::default());
    gb.load_rom(&args[1]);

    let event_loop = EventLoop::new();
    let window = WindowBuilder::new()
        .with_title("rsboy")
        .with_inner_size(winit::dpi::LogicalSize::new(WIDTH * SCALE, HEIGHT * SCALE))
        .build(&event_loop)
        .unwrap();

    let window_size = window.inner_size();
    let surface_texture = SurfaceTexture::new(window_size.width, window_size.height, &window);
    let mut pixels = Pixels::new(WIDTH, HEIGHT, surface_texture).expect("Failed to create pixels");

    event_loop.run(move |event, _, control_flow| {
        *control_flow = ControlFlow::Poll;

        match event {
            Event::MainEventsCleared => {
                let frame_data = gb.next_frame();

                let fb = pixels.frame_mut();
                for y in 0..HEIGHT as usize {
                    for x in 0..WIDTH as usize {
                        let color = frame_data.buffer[y][x];
                        let r = ((color & 0x1F) as u8) << 3;
                        let g = (((color >> 5) & 0x1F) as u8) << 3;
                        let b = (((color >> 10) & 0x1F) as u8) << 3;
                        let idx = (y * WIDTH as usize + x) * 4;
                        fb[idx]     = r;
                        fb[idx + 1] = g;
                        fb[idx + 2] = b;
                        fb[idx + 3] = 0xFF;
                    }
                }

                if let Err(e) = pixels.render() {
                    eprintln!("pixels.render() failed: {e}");
                    *control_flow = ControlFlow::Exit;
                }
            }

            Event::WindowEvent { event, .. } => match event {
                WindowEvent::CloseRequested => *control_flow = ControlFlow::Exit,

                WindowEvent::Resized(size) => {
                    if let Err(e) = pixels.resize_surface(size.width, size.height) {
                        eprintln!("pixels.resize_surface() failed: {e}");
                        *control_flow = ControlFlow::Exit;
                    }
                }

                WindowEvent::KeyboardInput {
                    input: KeyboardInput { state, virtual_keycode: Some(key), .. },
                    ..
                } => {
                    let pressed = state == ElementState::Pressed;
                    match key {
                        VirtualKeyCode::Right  => { if pressed { gb.press(controls::RIGHT) } else { gb.release(controls::RIGHT) } }
                        VirtualKeyCode::Left   => { if pressed { gb.press(controls::LEFT) } else { gb.release(controls::LEFT) } }
                        VirtualKeyCode::Up     => { if pressed { gb.press(controls::UP) } else { gb.release(controls::UP) } }
                        VirtualKeyCode::Down   => { if pressed { gb.press(controls::DOWN) } else { gb.release(controls::DOWN) } }
                        VirtualKeyCode::A      => { if pressed { gb.press(controls::KEY_A) } else { gb.release(controls::KEY_A) } }
                        VirtualKeyCode::S      => { if pressed { gb.press(controls::KEY_B) } else { gb.release(controls::KEY_B) } }
                        VirtualKeyCode::Q      => { if pressed { gb.press(controls::START) } else { gb.release(controls::START) } }
                        VirtualKeyCode::W      => { if pressed { gb.press(controls::SELECT) } else { gb.release(controls::SELECT) } }
                        VirtualKeyCode::F5 if !pressed => gb.load_state(),
                        VirtualKeyCode::F6 if !pressed => gb.save_state(),
                        _ => {}
                    }
                }
                _ => {}
            },
            _ => {}
        }
    });
}
