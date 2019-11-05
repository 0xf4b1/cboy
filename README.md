# cboy

An experimental GameBoy emulator written in C for educational purposes. It emulates the hardware, like the LR35902 CPU with its instruction set, MMU, and Display where the rendering is done with OpenGL, and you can play with your keyboard or joystick.

Some ROMs like _Tetris_ or _Super Mario Land_ are playable, but many others are not yet working.

## Building

	$ git clone https://github.com/0xf4b1/cboy && cd cboy
	$ mkdir build && cd build
	$ cmake ..
	$ make

## Usage

	$ ./cboy <rom>

It should use joystick from `/dev/input/js0` if available. Only tested with XBOX 360 controller.

## Controls

| Key   | Keyboard    | XBOX controller |
|-------|:-----------:|:---------------:|
|Up     | Up          | Up              |
|Down   | Down        | Down            |
|Left   | Left        | Left            |
|Right  | Right       | Right           |
|A      | A           | A               |
|B      | S           | B               |
|Start  | Q           | Start           |
|Select | W           | Back            |

### Implemented

- CPU with full LR35902 instruction set
- MMU (partly)
- Display (partly) and output via OpenGL (GLUT)
- Controls and input via keyboard and joystick

### Unimplemented

- Display windows
- Saving
- Timers
- Sound
- more ...

## Resources

- http://pastraiser.com/cpu/gameboy/gameboy_opcodes.html
- http://problemkaputt.de/pandocs.htm
- http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
- https://github.com/0xf4b1/cyboy
