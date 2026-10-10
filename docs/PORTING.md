# Porting Kondor to Linux: design decisions

This document explains how the recovered DOS code (see `DECOMPILATION.md`) was made to
run on modern Linux, and why each choice was made.

## Goals and constraints

1. **Build with libraries found on any modern Linux.** The only dependency besides the C++
   standard library is SDL2 (`libsdl2-dev` on Debian/Ubuntu, `SDL2-devel` on Fedora), and
   the build uses CMake and C++17. There are no other third-party libraries and no bundled
   code except a public-domain 8×8 font.
2. **Change the original game code as little as possible.** Function names, file layout,
   data structures and algorithms stay as they were, so the source can be compared with the
   decompiled drafts and with the binary.
3. **Prefer modern, safe behaviour over Borland quirks where stability is at stake.**
   Memory errors and undefined behaviour were fixed. Harmless gameplay oddities were kept.

These pull in different directions, which led to the main design decision: **emulate the
hardware at the boundary rather than rewrite the game.** The game still draws into a
320×200 8-bit frame buffer, still writes VGA palette registers, still receives raw
keyboard scan codes in an interrupt-style handler, and still mixes sound into a Sound
Blaster DMA ring. A thin platform layer makes those things real with SDL2.

## Source layout

```
src/KONDOR.CPP        original main file: globals, initgame/closeall/main, and the
                      28 #includes in the original order
src/KONDOR.H          the 31 recovered types, enums, globals and prototypes
src/<NAME>.H          one file per original source file; each starts with its port notes
src/compat/           the Borland C++ runtime functions the game used
src/platform/         platform.h (the hardware interface) and platform_sdl.cpp (SDL2)
```

**One translation unit, as in the original.** `KONDOR.CPP` includes the headers that
contain the code, exactly as QSoft wrote it. Splitting it into separately compiled `.cpp`
files would be more conventional. But it would change the order of definitions and the
visibility of statics, and it would make comparison with the original harder. The whole
game compiles in a couple of seconds anyway.

**The game code never calls SDL.** Everything hardware-related goes through
`platform.h`, which describes each function in terms of the DOS feature it replaces. A
different back-end (for example SDL3) would only need a new `platform_*.cpp`.

## The hardware boundary

| DOS original | Port | Where |
|---|---|---|
| `INT 10h` AX=13h / 03h (mode set) | `plat::set_mode13()` | QUNIT.H |
| writes to `A000:0000`, `rep movsd` blit in `drawscreen` | `plat::vga()` returns a 64,000-byte buffer | QUNIT.H |
| DAC ports `3C8h`/`3C9h` (write), `3C7h` (read) | `plat::dac_write()` / `dac_read()`, 6-bit values | QUNIT.H, PAL.H |
| busy-wait on port `3DAh` bit 3 (vertical retrace) | `plat::wait_retrace()`, paced at 70 Hz | QUNIT.H |
| `INT 9` handler reading port `60h` | `plat::set_scancode_handler()` | QUNIT.H |
| SB DSP ports, 8237 DMA, PIC, `ServiceIRQ2..11` | `plat::audio_start()` / `audio_position()` | QUNIT.H, SOUND.H |
| PC speaker `sound()` / `nosound()` | `plat::speaker_on()` / `speaker_off()` | compat |
| BIOS font at `F000:FA6E` | `font8x8_basic`, bit-reversed to BIOS order | KONDOR.CPP |

### Video

- **Frame buffer.** The game keeps drawing 8-bit palette indices into memory, as before.
  At each displayed frame, the platform converts the buffer through a 256-entry lookup
  table to 32-bit ARGB and uploads it to a streaming SDL texture. The lookup table is
  rebuilt only when the palette changes.
- **Palette.** VGA DAC values are 6-bit (0–63). They are widened to 8 bits as
  `(v << 2) | (v >> 4)`, so 63 becomes 255. Palette fades (`fadein`/`fadeout`) work
  unchanged, because they read and write the DAC through the same functions.
- **Aspect ratio.** Mode 13h pixels were not square: 320×200 filled a 4:3 CRT. The
  renderer's logical size is set to 4:3 (`SDL_RenderSetLogicalSize`), and the window
  defaults to the largest integer multiple of 320×240 that fits 90% of the desktop, capped
  at 5×. Scaling uses nearest-neighbour filtering to keep pixels sharp. Alt+Enter toggles
  borderless full screen.
- **Text mode.** `closegraph()` switches back to text mode, which shows as a blank screen
  until the program exits. Error messages are also printed to stderr.

### Timing

The original has **no time base other than the monitor's refresh rate**. Every frame,
`drawNhandle()` advances all game objects by one step and then waits for the vertical
retrace. Mode 13h refreshes at 70 Hz, so the game runs at 70 steps per second.

Options considered:

1. *Use the display's vsync.* Rejected. Modern displays run at 60, 75, 120 or 144 Hz,
   which would change the game's speed.
2. *Rewrite the game with a time delta.* Rejected. It would touch every movement
   function and break goal 2.
3. **Pace at exactly 70 Hz with a software clock.** Chosen. `wait_retrace()` sleeps until
   the next 1/70 s tick (`SDL_Delay` plus a short spin). If the game falls more than four
   frames behind, for example after the window was dragged, the clock resynchronises
   instead of fast-forwarding.

The original also has loops that never wait for the retrace. Menu and option screens
redraw in a tight loop, and "press any key" prompts spin on the keyboard state, which the
interrupt handler updated in the background. On Linux, a loop that never returns to SDL
freezes the window and never sees a key. This was found the hard way: the first build hung
in the menu and couldn't even be killed with SIGTERM, because SDL catches it. The
platform therefore has three entry points:

- `wait_retrace()`: a real frame (pace, present, count);
- `poll()`: called from `drawscreen()`. It processes window events, and if the loop didn't
  wait for the retrace itself it waits for the next tick, which paces menus at 70 Hz too;
- `idle()`: called from `skeypressed()` (the key-wait loops). It processes events and
  presents a frame every 1/70 s without sleeping.

These are the only places where game files call into the platform for timing, and each is
marked with a `port:` comment.

### Keyboard

The original installs its own `INT 9` handler. It reads raw XT "set 1" scan codes from
port 60h (an `E0` prefix for extended keys, bit 7 set on release) and keeps a
`keypress[256]` table. Key bindings in `KONDOR.CFG` are stored as these scan codes, and
the options screen shows key names from a scan-code table.

The port **feeds the original handler logic the same byte stream**. SDL scan codes, which
describe physical key positions and so don't depend on the keyboard layout, are converted
to XT codes (`xt_code()` in `platform_sdl.cpp`). Extended keys such as the arrows send
`E0` first. As a result:

- the shipped `KONDOR.CFG` works unchanged, and a `KONDOR.CFG` written by the port works in
  DOS;
- key redefinition and key names on the options screen work without changes;
- physical positions are used, so `[`, `]`, `Z` and `X` are the keys in those positions
  on any layout.

Alt+Enter is taken by the platform for full screen and never reaches the game. Left Alt
is the default fire key. Some desktop environments use Alt+drag or Alt+key shortcuts; the
key can be rebound under OPTIONS.

### Sound

The Sound Blaster code is the most hardware-bound part of the game. It sets the DSP to
8-bit mono auto-init DMA at time constant `0xA5` (10,989 Hz) over a 32 KB ring buffer,
with an interrupt after every 8 KB block. To play an effect, `MixVoice` *adds*
pre-converted samples into the ring starting at the current DMA position, wrapping on 8-bit
overflow. The interrupt handler clears the block that finished playing. The game reads the
DMA controller's count register to find the play position.

Options considered:

1. *Replace the sound code with an SDL mixer.* Rejected. It would rewrite SOUND.H and lose
   the original's mixing behaviour, including its wrap-around distortion when many effects
   overlap.
2. **Emulate the DMA ring and the interrupt.** Chosen. `audio_start()` hands the ring to
   the platform. The SDL audio callback plays it byte by byte at the programmed rate and
   calls the game's original `ServiceIRQ()` logic at every 8 KB boundary.
   `audio_position()` replaces the DMA count register. The callback runs on SDL's audio
   thread, a real second thread, whereas the DOS interrupt only ever interrupted a single
   CPU. So the game reads the play position and mixes into the ring (`isplaying`,
   `MixVoice`) between `audio_lock()` and `audio_unlock()`.

The output format is unsigned 8-bit mono at 10,989 Hz, and SDL converts it to whatever the
sound server wants. The PC-speaker beep (`sound()`/`nosound()`) is mixed in as a square
wave.

Sound card detection (`initsb`/`getsb`, which probed ports, IRQs and DMA channels) has
nothing left to probe. If SDL opens an audio device, the boot screen reports
"FOUND AT IRQ: 5 DMA: 1", the classic default settings. If not, it reports "FAILED!" and the
game runs silently, as the original did without a sound card.

### Data that lived inside KONDOR.EXE

Some content was compiled into the executable rather than shipped as data files: the
help text (150 lines), the menu caption, and the rock, enemy-sprite and gap bitmaps.
Copying them into the port's source would mean redistributing QSoft's content. Instead,
`load_exe_data()` in `KONDOR.CPP` reads them from the player's own `KONDOR.EXE` at their
DGROUP offsets. It checks the file size (221,323 bytes) and the first bytes of the menu
caption (stored with bit 7 set), and refuses to start with a clear message if the EXE is
missing or a different version.

### Files

- **Case.** DOS file names are case-insensitive, and the game opens `kondor1.map`,
  `KONDOR.PAL` and so on in mixed case. `fopen_ci()` looks up names case-insensitively in
  the data directory; new files are created in upper case.
- **Location.** The data directory is `$KONDOR_DATA`, or the current directory if that is
  unset. `KONDOR.CFG` and `KONDOR.HSC` are written there, as in the original.
- **Binary formats.** The game's own file formats were written by dumping 16-bit structs.
  The port reads and writes them field by field as 16-bit little-endian values
  (`read_le16`/`write_le16`). The packed on-disk structs (`loadtank_type`,
  `loadenemy_type` and the others) have `static_assert`s on their sizes. Files stay
  compatible in both directions.
- **Short reads.** The original never checked `fread()`. `fread_z()` zero-fills anything a
  short read leaves, so a truncated data file can't leave buffers uninitialised.

## Modernising Borland C++ 3.x

| Borland | Port | Why |
|---|---|---|
| 16-bit `int` | 32-bit `int` | Overflow in 16-bit arithmetic is undefined behaviour in modern C++. Every expression that could exceed 16 bits was checked; only `kincircle` changed results, because its squared distance wrapped at about 181 px |
| far pointers and `seg:0000` tricks | plain pointers | The original used off-screen buffers from offset 0 of their segment, overwriting each `malloc` block header |
| `enum boolean {false, true}` | `typedef int boolean` | C++ has its own `true`/`false` |
| `rand()` / `random()` / `randomize()` | the same 32-bit LCG (`seed*0x015A4E35+1`, bits 16–30) and `rand()*n/32768` | The same seed gives the same random sequence. `KONDOR_SEED` fixes the seed for tests |
| right-to-left argument evaluation | explicit statement order | C++ leaves the order unspecified. Calls with two `random()` arguments were split so that they keep Borland's order |
| `FTOL@` (truncating float→int) | `ftol()` truncates toward zero | Same results for negative values |
| `itoa`, `ultoa`, `strupr`, `delay` | `compat/borland.cpp` | Not in standard C/C++ |
| `strupr()` on string literals | uppercase into a local copy | Writing to literals crashes on modern systems |
| `or` as a variable name | `or_` | `or` is a C++ keyword |

## Bugs: what was fixed and what was kept

The rule: **fix anything that corrupts memory, reads out of bounds, or can hang or crash;
keep anything that is just how the game behaves.**

Fixed (each is marked `port:` in the source):

| Where | Original behaviour |
|---|---|
| `loadpal` | `free()` of a stack array |
| `writestrxy(int)` | `itoa()` into a 1-byte string literal, overwriting the next literal (`"rb"`) |
| `getkeyname` | returned a pointer to its own stack |
| `drawstring`, `drawstringreg` | `strupr()` on the caller's string, including literals |
| screen buffers | used from `seg:0000`, overwriting the heap header |
| `waterxy`, `blockingxy`, `minblockingxy` | no bounds check; off-map cells now count as blocking and not water |
| `drawmap` | reads one row past the map at the bottom edge; the map buffer is padded |
| `drawsink` | read outside the frame buffer near the screen edge |
| `findtheway` | waypoint list overflowed into the next enemy's fields; search lists were sized at 500 points but the map has 9,216 cells; an invalid direction code could loop forever |
| `handlecounter` | indexed inventory slot −1 |
| `bases_type::add` | no bounds check on the 5 bases |
| `loadimg` | leaked a file handle |
| `loadfmap` | the level message was not always terminated |
| all `fread` calls | unchecked; now zero-filled on a short read |

Kept, because they are part of how the game plays:

- the path search never tests the south-west neighbour;
- drawing a dead enemy beeps the PC speaker for half a second (a leftover debugging aid);
- `turn_cannon` masks the hull's angle instead of the cannon's;
- `objects_type::handle` tests a fixed point at (1,1);
- holding Escape on the key configuration screen skips all remaining entries;
- `KONDOR.CFG` is rewritten on exit only when no sound card was found. Changes under
  OPTIONS are saved immediately.

## Test hooks

Testing a game without a person at the keyboard needs determinism. The platform layer has
hooks controlled by environment variables:

- **Deterministic input.** `KONDOR_SCRIPT` injects key events at given frame numbers.
  Frames, not milliseconds, are the unit, so a script does the same thing at any speed.
- **Deterministic randomness.** `KONDOR_SEED` seeds Borland's generator.
- **Speed.** `KONDOR_FAST` drops the 70 Hz pacing, and the game then runs as fast as the
  CPU allows. Results don't change, because the game only counts frames.
- **No display or sound needed.** `KONDOR_HEADLESS` and `KONDOR_NOSOUND`.
- **Observation.** `KONDOR_SHOTS`/`KONDOR_SHOT_FRAMES` save frames as PPM images, and
  `KONDOR_MAXFRAMES` stops the run.
- **Level coverage.** `KONDOR_TEST_LEVELS` plays each of the 12 levels for 400 frames and
  saves a picture of each.

As a result, a run with the same script and seed repeats exactly: random-input sessions
reach the same frame counts on every run. Flags count as off when unset, empty or `0`.
A `KONDOR_SCRIPT` that can't be opened is a fatal error rather than a silent no-input run.
`TESTING.md` shows how these are used.

## Known limitations

- **Never played by a person on a real desktop.** All testing so far ran on a headless VM.
  How the controls feel and how the sound is timed are untested by ear.
- **No frame-exact comparison with the original.** DOSBox doesn't run the game at a fixed
  frame rate relative to input, so a recorded DOS session can't be replayed into the port
  frame for frame. Correctness rests on the line-by-line translation, the struct-size
  checks, visual comparison with DOSBox, and the sanitizer runs.
- **Keyboard only.** The original had no joystick or mouse support either.
- **SDL2, not SDL3.** SDL2 is still the version packaged everywhere. A port to SDL3 would
  only touch `platform_sdl.cpp`.
