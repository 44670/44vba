# 44VBA

A GBA emulator for various platforms, including ESP32-S3, embedded linux, wasm, etc.

Forked from https://github.com/libretro/vba-next .

# Platforms and Test Results

Tested with [Goodboy Advance](https://www.gbadev.org/demos.php?showinfo=1486), in game-play.

| Name | Tested hardware | Performance | Notes |
| --- | --- | --- | --- |
| ESP32-S3 | ESP32-S3-WROOM-1-N8R8 | 20 fps | frameskip: 1 |
| SDL2 | AMD 3800X | 1800 fps | |
| SDL2 | Switch | 314 fps | |
| SDL2 | Apple M1 | 2300 fps | |
| SDL2 | Vita | 131 fps | frameskip: 1, overclocked | |
| SDL1 | New 3DS |  111 fps | frameskip: 1, overclocked | |
| watchOS | Apple Watch Series 5 | 451 fps | Not public yet |
| WebAssembly | - | - | https://gba.44670.org |

# Download 

Pre-built binaries are available at:

https://github.com/44670/44vba/releases

Available platforms: Windows, Switch.

You can also build from source for other platforms.
