# cboy

![](https://github.com/0xf4b1/cboy/workflows/linux/badge.svg)
![](https://github.com/0xf4b1/cboy/workflows/android/badge.svg)

An experimental GameBoy emulator written in C for educational purposes. It emulates the hardware, like the LR35902 CPU with its instruction set, MMU, and Display where the rendering is done with OpenGL, and you can play with your keyboard or joystick. Saving is now possible by storing the whole emulation state (CPU registers and memory), so you can immediately continue where you left off.

## Screenshots

![](images/screenshot1.png)
![](images/screenshot2.png)

## Building

	$ git clone https://github.com/0xf4b1/cboy && cd cboy
	$ mkdir build && cd build
	$ cmake ..
	$ make

## Usage

	$ ./cboy <rom>

It should use joystick from `/dev/input/js0` if available. Only tested with XBOX 360 controller.

## Framebuffer output instead OpenGL

Instead of using OpenGL as renderer, there is another display output method that directly writes to the framebuffer `/dev/fb0`. It only works outside X11 and only takes input from joystick. This method was intended to run the emulator on a Raspberry Pi 2, where the OpenGL performance with GLUT is very poor (would need OpenGL ES 2.0).

To enable display output to framebuffer, use cmake with the following option enabled:

	$ cmake .. -DRENDERER=FRAMEBUFFER

Disable cursor blinking:

	# echo 0 > /sys/class/graphics/fbcon/cursor_blink

## AnBoy

AnBoy is cboy on Android! It uses libcboy based on [NativeActivity](https://github.com/android/ndk-samples/tree/master/native-activity), the rendering is done via OpenGL ES, and the controls are currently very basic, e.g. direction keys via swipe gestures and buttons via tapping on specific display regions. This works quite well for ROMs like `Tetris` or `Pokemon Red/Blue`! :) It comes with a small Activity that allows you to browse your `/sdcard` to select a ROM.

## Building

	$ git clone https://github.com/0xf4b1/cboy && cd cboy/anboy
	$ ./gradlew assembleDebug
	$ adb install anboy/build/outputs/apk/debug/anboy-debug.apk

## Controls

| Key   | Keyboard    | XBOX controller | AnBoy            |
|-------|:-----------:|:---------------:|:----------------:|
|Up     | Up          | Up              | Swipe up         |
|Down   | Down        | Down            | Swipe down       |
|Left   | Left        | Left            | Swipe left       |
|Right  | Right       | Right           | Swipe right      |
|A      | A           | A               | Tap lower screen |
|B      | S           | B               | unassigned       |
|Start  | Q           | Start           | Tap middle right |
|Select | W           | Back            | Tap middle left  |
|Load   | F5          | LB              | Tap upper left   |
|Save   | F6          | RB              | Tap upper right  |

## Implemented

- CPU with full LR35902 instruction set
- MMU
- Display and output via OpenGL (GLUT)
- Controls and input via keyboard and joystick
- Saving emulation state

## Unimplemented

- ~~Saving~~
- Timers
- Sound
- more ...

## Resources

- http://pastraiser.com/cpu/gameboy/gameboy_opcodes.html
- http://problemkaputt.de/pandocs.htm
- http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
- https://github.com/0xf4b1/cyboy
