# Kondor for Linux (SDL2)

A native port of **Kondor** v1.0 (12/20/1999), a freeware DOS tank game by QSoft
(Gil Dabah and Oren Becker), reconstructed from the shipped `KONDOR.EXE` avaliable at the [DOS Games Archive](https://www.dosgamesarchive.com/download/kondor).

The DOS executable was built with Borland C++ and still contained its Turbo Debugger
information: function and variable names, all 31 struct/class layouts, local variables,
and line numbers mapped to the original source files (`KONDOR.CPP` plus 28 included `.H`
files). This port keeps that structure, the names and the game logic. Only the parts that
talked to DOS hardware were replaced, plus fixes for crashes and undefined behaviour
(listed below).

## Documentation

- [`docs/DECOMPILATION.md`](docs/DECOMPILATION.md): how the source was recovered from
  `KONDOR.EXE`, step by step with commands;
- [`docs/PORTING.md`](docs/PORTING.md): the design of the Linux port and the reasons for
  each decision;
- [`docs/TESTING.md`](docs/TESTING.md): how the port was tested, with commands to repeat
  every test by hand.

## Building

You need a C++17 compiler, CMake 3.16 or newer, and the SDL2 development package:

```sh
sudo apt install build-essential cmake libsdl2-dev      # Debian/Ubuntu
sudo dnf install gcc-c++ cmake SDL2-devel               # Fedora
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Running

The game data isn't included. Unpack the original distribution (`kond-box.zip`) and run
`kondor` from that directory, or point `KONDOR_DATA` at it:

```sh
cd /path/to/KONDOR && /path/to/build/kondor
KONDOR_DATA=/path/to/KONDOR ./build/kondor
```

- `KONDOR.EXE` must be present. The port reads the game's built-in data from it: help
  text, the menu string, and the rock, tank and base bitmaps.
- The directory must be writable, because the game saves `KONDOR.CFG` and `KONDOR.HSC`
  there, as the original did.
- File names are matched case-insensitively.

Controls are the defaults from `KONDOR.CFG` and can be changed under OPTIONS:

| Action | Key |
|---|---|
| fire | Left Alt |
| brake | Space |
| turn left / right | ← / → |
| drive forward / backward | ↑ / ↓ |
| use inventory item | Enter |
| previous / next inventory slot | `[` / `]` |
| turn cannon left / right | Z / X |

Escape returns to the menu, where RESUME continues the game. Alt+Enter toggles full
screen. The image is shown at 4:3, as on a VGA monitor.

## How the hardware was replaced

| DOS original | Port |
|---|---|
| VGA mode 13h, `A000:0000`, `rep movsd` blit | 64000-byte frame buffer shown through an SDL texture |
| VGA DAC ports 3C7h–3C9h | 256-entry palette (6-bit values, scaled to 8-bit) |
| busy-wait on 3DAh for vertical retrace | frame pacing at exactly 70 Hz, which is also the game speed |
| INT 9 keyboard handler reading port 60h | SDL key events converted to the same XT scan-code byte stream, so `KONDOR.CFG` stays compatible |
| Sound Blaster DSP, DMA auto-init ring, IRQ handler | SDL audio callback that plays the same 32 KB ring at 10989 Hz and calls the original block-complete IRQ logic |
| PC speaker `sound()`/`nosound()` | square wave mixed into the audio output |
| BIOS ROM 8×8 font (error screens only) | the public-domain `font8x8_basic` |
| Borland `rand`/`random`/`randomize` | the same LCG (seed × 0x015A4E35 + 1), so a given seed gives the original's random sequence |

## Source layout

- `src/KONDOR.CPP` and `src/*.H`: the game. The original single translation unit, with
  one header per original source file. `src/KONDOR.H` declares the recovered types and
  globals.
- `src/compat/`: replacements for the Borland runtime functions the game used.
- `src/platform/`: the SDL2 hardware layer (video, keyboard, audio, timing).
- `docs/`: decompilation, porting and testing notes.
- `tools/`: the reconstruction tooling:
  - TDInfo parser and spec generator;
  - headless Ghidra script that applies types, names and signatures and decompiles;
  - annotated-disassembly generator;
  - random-input generator and runner used for the sanitizer runs.

## Changes from the 1999 code

Each header lists its own changes. In summary:

**Undefined behaviour and memory errors fixed**
- `loadpal` called `free()` on a stack array.
- `writestrxy(int)` wrote `itoa()` output into a 1-byte string literal, overwriting the next literal (`"rb"`).
- `getkeyname` returned a pointer to its own stack.
- `drawstring` and `drawstringreg` called `strupr()` on string literals.
- Off-screen buffers were used from `seg:0000`, ignoring the `malloc` offset, which overwrote the heap header.
- Map lookups outside the 96×96 grid now count as blocking and non-water. The original read unrelated memory there.
- `drawmap` reads one row past the map at the bottom edge; the map buffer is padded for it.
- `drawsink` read outside the frame buffer near the screen edge.
- `findtheway`: waypoint lists can no longer overflow into the next enemy, the search lists are sized for the whole map, and an invalid direction code can no longer spin forever.
- `handlecounter` used inventory slot −1.
- `bases_type::add` had no bounds check.
- `loadimg` leaked a file handle.
- The level message is now always terminated.
- `fread` results were never checked. A short read is now zero-filled (`fread_z`).

**16-bit arithmetic.** `int` is 32-bit. The only place this changes behaviour is
`kincircle`, whose squared distance wrapped at about 181 px in 16-bit arithmetic.

**Original quirks kept**
- The path search never tests the south-west neighbour.
- Drawing a dead enemy beeps the PC speaker for half a second (a leftover debugging aid).
- `turn_cannon` masks the hull angle instead of the cannon's.
- `objects_type::handle` tests a fixed point at (1,1).
- A held Escape skips all remaining entries on the key configuration screen.
- On exit, `KONDOR.CFG` is rewritten only when no sound card was found, to restore the sound setting. Changes made under OPTIONS are saved straight away.

## Test hooks

These environment variables are for automated testing:

| Variable | Effect |
|---|---|
| `KONDOR_HEADLESS=1` | run without a window |
| `KONDOR_NOSOUND=1` | no audio device; "Sound Blaster not found" |
| `KONDOR_FAST=1` | don't wait for the 70 Hz frame time |
| `KONDOR_SEED=n` | fixed random seed |
| `KONDOR_SCRIPT=file` | key events, one per line: `<frame> down|up <scancode hex>` (≥ 0x80 means extended) |
| `KONDOR_SHOTS=dir` with `KONDOR_SHOT_FRAMES=a,b,c` | save those frames as PPM |
| `KONDOR_MAXFRAMES=n` | quit after n frames |
| `KONDOR_TEST_LEVELS=dir` | play each of the 12 levels without input and save a frame of each |

Flags count as off when unset, empty or `0`. Use absolute paths for `KONDOR_SCRIPT` and
`KONDOR_SHOTS`. A script that can't be opened is a fatal error. `docs/TESTING.md` shows
these in use.

## Legal

Kondor is © 1997–1999 QSoft and was released as freeware. This is an unofficial
reconstruction for preservation and interoperability. It contains none of the game's
data: images, sounds, maps and the strings inside `KONDOR.EXE` are read from your copy
at run time. Published with the original authors' permission. `src/compat/font8x8_basic.h` is
public domain (Daniel Hepper, from the public-domain IBM VGA font).
