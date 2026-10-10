// platform.h - replacement for the DOS/VGA/Sound Blaster hardware Kondor talked to directly.
//
// The original game wrote to VGA memory at A000:0000, programmed the DAC through
// ports 3C7h-3C9h, polled 3DAh for vertical retrace, hooked INT 9 for the keyboard
// and drove a Sound Blaster through DSP ports, DMA and an IRQ handler. Each of those
// touch points is mapped to a function here, implemented with SDL2 in platform_sdl.cpp.
#pragma once
#include <cstdint>
#include <cstddef>

namespace plat {

constexpr int SCREEN_W = 320, SCREEN_H = 200;

bool init();                 // window, renderer, audio device (audio failure is not fatal)
void shutdown();

// --- video -------------------------------------------------------------------
uint8_t *vga();              // 64000-byte mode 13h frame buffer (stands in for A000:0000)
void set_mode13(bool on);    // INT 10h AX=13h / AX=03h
void dac_write(uint8_t index, uint8_t r, uint8_t g, uint8_t b);   // 6-bit components
void dac_read(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b);
void wait_retrace();         // blocks until the next 70 Hz tick and presents the frame
void poll();                 // after drawing a frame in a loop that never calls wait_retrace()
void idle();                 // inside a busy-wait on input: pump events, advance time, no sleep

// --- keyboard (INT 9 replacement) ----------------------------------------------
// The game installs a handler that receives raw XT set-1 scancodes from port 60h.
// The platform layer synthesizes the same byte stream (E0 prefix for extended keys,
// bit 7 set for key release) and feeds it to this callback.
using ScancodeHandler = void (*)(uint8_t code);
void set_scancode_handler(ScancodeHandler h);
bool quit_requested();       // window closed

// --- timing ------------------------------------------------------------------
uint32_t ticks_ms();
void delay_ms(unsigned ms);  // Borland delay()

// --- sound (Sound Blaster DMA replacement) -------------------------------------
// The game mixes 8-bit unsigned mono samples into an auto-init DMA ring buffer and the
// SB raises an IRQ after every block. The platform plays that ring at the programmed
// rate and calls the IRQ callback at each block boundary, from the audio thread with
// the audio device locked against the game thread (see audio_lock()).
// audio_position() replaces reading the DMA controller's count register: it returns the
// number of bytes of the ring already transferred (0..ring_len-1).
using IrqHandler = void (*)();
bool audio_start(uint8_t *ring, size_t ring_len, size_t block_len, unsigned rate_hz, IrqHandler on_block);
size_t audio_position();
void audio_stop();
void audio_lock();
void audio_unlock();
bool audio_available();

// --- PC speaker (Borland sound()/nosound()) ----------------------------------------
void speaker_on(unsigned hz);
void speaker_off();

// --- deterministic test hooks --------------------------------------------------
// KONDOR_SCRIPT=<file>: lines "<frame> <down|up> <scancode-hex>" injected on that frame.
// KONDOR_SHOTS=<dir>:    with KONDOR_SHOT_FRAMES="10,200,...", dumps those frames as PPM.
// KONDOR_MAXFRAMES=<n>:  request quit after n frames.
// KONDOR_FAST=1:         don't sleep in wait_retrace (frames run as fast as possible).
uint64_t frame_count();
void save_frame(const char *path);   // current VGA frame as a PPM

}  // namespace plat
