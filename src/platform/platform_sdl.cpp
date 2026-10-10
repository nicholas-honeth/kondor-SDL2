// platform_sdl.cpp - SDL2 implementation of platform.h.
#include "platform.h"

#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace plat {
namespace {

constexpr double FRAME_MS = 1000.0 / 70.0;   // VGA mode 13h vertical refresh

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
bool headless;
bool fast;
bool quit;
bool mode13 = false;

uint8_t vram[SCREEN_W * SCREEN_H];
uint8_t dac[256][3];
uint32_t lut[256];
uint32_t pixels[SCREEN_W * SCREEN_H];

double next_frame_ms;
uint32_t last_present_ms;
uint64_t frames;
uint64_t max_frames;

ScancodeHandler key_handler;

// test hooks
std::multimap<uint64_t, std::pair<bool, uint8_t>> script;   // frame -> (down, code)
std::set<uint64_t> shot_frames;
std::string shot_dir;

// audio
SDL_AudioDeviceID audio_dev;
uint8_t *ring;
size_t ring_len, ring_pos, block_len = 1;
IrqHandler irq;
bool dma_running;
unsigned spk_hz;
double spk_phase;
int audio_rate = 22050;

// SDL scancode -> XT set-1 scancode; 0x100 bit marks an E0-prefixed (extended) key.
int xt_code(SDL_Scancode s) {
    switch (s) {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03; case SDL_SCANCODE_3: return 0x04;
    case SDL_SCANCODE_4: return 0x05; case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
    case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09; case SDL_SCANCODE_9: return 0x0A;
    case SDL_SCANCODE_0: return 0x0B; case SDL_SCANCODE_MINUS: return 0x0C; case SDL_SCANCODE_EQUALS: return 0x0D;
    case SDL_SCANCODE_BACKSPACE: return 0x0E; case SDL_SCANCODE_TAB: return 0x0F;
    case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11; case SDL_SCANCODE_E: return 0x12;
    case SDL_SCANCODE_R: return 0x13; case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17; case SDL_SCANCODE_O: return 0x18;
    case SDL_SCANCODE_P: return 0x19; case SDL_SCANCODE_LEFTBRACKET: return 0x1A;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1B; case SDL_SCANCODE_RETURN: return 0x1C;
    case SDL_SCANCODE_LCTRL: return 0x1D;
    case SDL_SCANCODE_A: return 0x1E; case SDL_SCANCODE_S: return 0x1F; case SDL_SCANCODE_D: return 0x20;
    case SDL_SCANCODE_F: return 0x21; case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25; case SDL_SCANCODE_L: return 0x26;
    case SDL_SCANCODE_SEMICOLON: return 0x27; case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_GRAVE: return 0x29; case SDL_SCANCODE_LSHIFT: return 0x2A;
    case SDL_SCANCODE_BACKSLASH: return 0x2B;
    case SDL_SCANCODE_Z: return 0x2C; case SDL_SCANCODE_X: return 0x2D; case SDL_SCANCODE_C: return 0x2E;
    case SDL_SCANCODE_V: return 0x2F; case SDL_SCANCODE_B: return 0x30; case SDL_SCANCODE_N: return 0x31;
    case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_COMMA: return 0x33; case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35; case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_KP_MULTIPLY: return 0x37; case SDL_SCANCODE_LALT: return 0x38;
    case SDL_SCANCODE_SPACE: return 0x39; case SDL_SCANCODE_CAPSLOCK: return 0x3A;
    case SDL_SCANCODE_F1: return 0x3B; case SDL_SCANCODE_F2: return 0x3C; case SDL_SCANCODE_F3: return 0x3D;
    case SDL_SCANCODE_F4: return 0x3E; case SDL_SCANCODE_F5: return 0x3F; case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42; case SDL_SCANCODE_F9: return 0x43;
    case SDL_SCANCODE_F10: return 0x44; case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
    case SDL_SCANCODE_SCROLLLOCK: return 0x46;
    case SDL_SCANCODE_KP_7: return 0x47; case SDL_SCANCODE_KP_8: return 0x48; case SDL_SCANCODE_KP_9: return 0x49;
    case SDL_SCANCODE_KP_MINUS: return 0x4A; case SDL_SCANCODE_KP_4: return 0x4B; case SDL_SCANCODE_KP_5: return 0x4C;
    case SDL_SCANCODE_KP_6: return 0x4D; case SDL_SCANCODE_KP_PLUS: return 0x4E; case SDL_SCANCODE_KP_1: return 0x4F;
    case SDL_SCANCODE_KP_2: return 0x50; case SDL_SCANCODE_KP_3: return 0x51; case SDL_SCANCODE_KP_0: return 0x52;
    case SDL_SCANCODE_KP_PERIOD: return 0x53; case SDL_SCANCODE_F11: return 0x57; case SDL_SCANCODE_F12: return 0x58;
    case SDL_SCANCODE_KP_ENTER: return 0x11C; case SDL_SCANCODE_RCTRL: return 0x11D;
    case SDL_SCANCODE_KP_DIVIDE: return 0x135; case SDL_SCANCODE_RALT: return 0x138;
    case SDL_SCANCODE_HOME: return 0x147; case SDL_SCANCODE_UP: return 0x148; case SDL_SCANCODE_PAGEUP: return 0x149;
    case SDL_SCANCODE_LEFT: return 0x14B; case SDL_SCANCODE_RIGHT: return 0x14D; case SDL_SCANCODE_END: return 0x14F;
    case SDL_SCANCODE_DOWN: return 0x150; case SDL_SCANCODE_PAGEDOWN: return 0x151;
    case SDL_SCANCODE_INSERT: return 0x152; case SDL_SCANCODE_DELETE: return 0x153;
    default: return 0;
    }
}

void feed(int code, bool down) {
    if (!key_handler || !code) return;
    if (code & 0x100) key_handler(0xE0);
    key_handler(uint8_t((code & 0x7F) | (down ? 0 : 0x80)));
}

void present() {
    if (headless) { last_present_ms = SDL_GetTicks(); return; }
    if (mode13) {
        for (int i = 0; i < SCREEN_W * SCREEN_H; i++) pixels[i] = lut[vram[i]];
    } else {
        std::memset(pixels, 0, sizeof pixels);   // text mode: show a blank screen
    }
    SDL_UpdateTexture(texture, nullptr, pixels, SCREEN_W * 4);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    last_present_ms = SDL_GetTicks();
}

void save_shot(uint64_t n) {
    char path[512];
    std::snprintf(path, sizeof path, "%s/frame_%06llu.ppm", shot_dir.c_str(), (unsigned long long)n);
    FILE *f = std::fopen(path, "wb");
    if (!f) {
        std::fprintf(stderr, "kondor: cannot write %s\n", path);
        return;
    }
    std::fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        uint32_t c = lut[vram[i]];
        uint8_t rgb[3] = {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
}

void pump() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT: quit = true; std::exit(0);   // window closed: leave like the DOS version on Ctrl-Break
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN && (ev.key.keysym.mod & KMOD_ALT)) {
                if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                    bool fs = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
                    SDL_SetWindowFullscreen(window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
                break;
            }
            feed(xt_code(ev.key.keysym.scancode), ev.type == SDL_KEYDOWN);
            break;
        default: break;
        }
    }
}

void audio_cb(void *, Uint8 *out, int len) {
    for (int i = 0; i < len; i++) {
        int s = 128;
        if (dma_running && ring) {
            s = ring[ring_pos];
            if (++ring_pos == ring_len) ring_pos = 0;
            if (ring_pos % block_len == 0 && irq) irq();   // "block complete" interrupt
        }
        if (spk_hz) {
            spk_phase += double(spk_hz) / audio_rate;
            if (spk_phase >= 1.0) spk_phase -= 1.0;
            s += spk_phase < 0.5 ? 24 : -24;
        }
        out[i] = Uint8(s < 0 ? 0 : s > 255 ? 255 : s);
    }
}

bool open_audio(unsigned rate) {
    if (audio_dev) { SDL_CloseAudioDevice(audio_dev); audio_dev = 0; }
    SDL_AudioSpec want{}, have{};
    want.freq = int(rate);
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_cb;
    audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);   // SDL converts if needed
    if (!audio_dev) return false;
    audio_rate = int(rate);
    SDL_PauseAudioDevice(audio_dev, 0);
    return true;
}

// A boolean test hook is on when set to anything but "" or "0".
bool env_flag(const char *name) {
    const char *v = std::getenv(name);
    return v && *v && std::strcmp(v, "0") != 0;
}

void load_hooks() {
    if (const char *s = std::getenv("KONDOR_SCRIPT")) {
        FILE *f = std::fopen(s, "r");
        if (!f) {
            std::fprintf(stderr, "kondor: cannot open KONDOR_SCRIPT %s\n", s);
            std::exit(1);
        }
        unsigned long long fr; char what[8]; unsigned code;
        while (std::fscanf(f, "%llu %7s %x", &fr, what, &code) == 3)
            script.emplace(fr, std::make_pair(std::strcmp(what, "down") == 0, uint8_t(code)));
        std::fclose(f);
    }
    if (const char *s = std::getenv("KONDOR_SHOTS")) shot_dir = s;
    if (const char *s = std::getenv("KONDOR_SHOT_FRAMES")) {
        std::string v = s; size_t p = 0;
        while (p < v.size()) {
            size_t q = v.find(',', p); if (q == std::string::npos) q = v.size();
            shot_frames.insert(std::strtoull(v.substr(p, q - p).c_str(), nullptr, 10));
            p = q + 1;
        }
    }
    if (const char *s = std::getenv("KONDOR_MAXFRAMES")) max_frames = std::strtoull(s, nullptr, 10);
    fast = env_flag("KONDOR_FAST");
    headless = env_flag("KONDOR_HEADLESS");
}

}  // namespace

bool init() {
    load_hooks();
    Uint32 flags = SDL_INIT_TIMER | SDL_INIT_EVENTS | (headless ? 0 : SDL_INIT_VIDEO);
    if (!env_flag("KONDOR_NOSOUND")) flags |= SDL_INIT_AUDIO;
    if (SDL_Init(flags) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (!headless) {
        // Largest integer multiple of 320x240 (4:3, like the CRT) that fits the desktop.
        int scale = 3;
        SDL_DisplayMode dm;
        if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
            int sx = dm.w * 9 / 10 / SCREEN_W, sy = dm.h * 9 / 10 / (SCREEN_H * 6 / 5);
            scale = sx < sy ? sx : sy;
            if (scale < 1) scale = 1;
            if (scale > 5) scale = 5;
        }
        window = SDL_CreateWindow("Kondor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W * scale,
                                  SCREEN_H * scale * 6 / 5, SDL_WINDOW_RESIZABLE);
        if (!window) { std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return false; }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer) { std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return false; }
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        SDL_RenderSetLogicalSize(renderer, SCREEN_W * 4, SCREEN_H * 4 * 6 / 5);   // 4:3, like a CRT
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                    SCREEN_W, SCREEN_H);
    }
    next_frame_ms = SDL_GetTicks();
    return true;
}

void shutdown() {
    if (std::getenv("KONDOR_SCRIPT")) std::fprintf(stderr, "kondor: exit after %llu frames\n", (unsigned long long)frames);
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

uint8_t *vga() { return vram; }
void set_mode13(bool on) {
    mode13 = on;
    if (on) std::memset(vram, 0, sizeof vram);
    present();
}

void dac_write(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
    dac[i][0] = r & 63; dac[i][1] = g & 63; dac[i][2] = b & 63;
    auto x = [](uint8_t v) { return uint32_t((v << 2) | (v >> 4)); };
    lut[i] = 0xFF000000u | x(dac[i][0]) << 16 | x(dac[i][1]) << 8 | x(dac[i][2]);
}
void dac_read(uint8_t i, uint8_t *r, uint8_t *g, uint8_t *b) { *r = dac[i][0]; *g = dac[i][1]; *b = dac[i][2]; }

// One display frame: present, count, inject scripted keys, dump requested shots.
static void tick() {
    present();
    ++frames;
    auto range = script.equal_range(frames);
    for (auto it = range.first; it != range.second; ++it) {
        int code = it->second.second;
        if (code & 0x80 && code != 0xE0) {
            if (key_handler) key_handler(0xE0);
            code &= 0x7F;
        }
        if (key_handler) key_handler(uint8_t(code | (it->second.first ? 0 : 0x80)));
    }
    if (!shot_dir.empty() && shot_frames.count(frames)) save_shot(frames);
    if (max_frames && frames >= max_frames) {
        std::fprintf(stderr, "kondor: KONDOR_MAXFRAMES reached\n");
        std::exit(0);
    }
}

static bool ticked_since_poll;
static unsigned idle_calls;

void wait_retrace() {
    pump();
    if (!fast) {
        double now = SDL_GetTicks();
        if (next_frame_ms < now - 4 * FRAME_MS) next_frame_ms = now;   // fell behind: resync
        while (now < next_frame_ms) {
            SDL_Delay(Uint32(next_frame_ms - now) > 1 ? Uint32(next_frame_ms - now) - 1 : 0);
            now = SDL_GetTicks();
        }
        next_frame_ms += FRAME_MS;
    }
    tick();
    ticked_since_poll = true;
}

void poll() {
    pump();
    if (!ticked_since_poll) wait_retrace();   // a loop that never waits for the retrace
    ticked_since_poll = false;
}

void idle() {
    pump();
    bool due = fast ? (++idle_calls % 16 == 0) : (SDL_GetTicks() - last_present_ms >= Uint32(FRAME_MS));
    if (due) {
        tick();
        next_frame_ms = SDL_GetTicks() + FRAME_MS;
    }
}

void set_scancode_handler(ScancodeHandler h) { key_handler = h; }
bool quit_requested() { return quit; }

uint32_t ticks_ms() { return SDL_GetTicks(); }
void delay_ms(unsigned ms) {
    if (fast) { pump(); return; }
    Uint32 end = SDL_GetTicks() + ms;
    while (SDL_GetTicks() < end) {
        pump();
        Uint32 left = end - SDL_GetTicks();
        SDL_Delay(left > 10 ? 10 : left);
        if (SDL_GetTicks() - last_present_ms >= 50) present();
    }
}

bool audio_start(uint8_t *r, size_t len, size_t block, unsigned rate_hz, IrqHandler on_block) {
    if (!(SDL_WasInit(SDL_INIT_AUDIO))) return false;
    if (!audio_dev || audio_rate != int(rate_hz)) {
        if (!open_audio(rate_hz)) return false;
    }
    SDL_LockAudioDevice(audio_dev);
    ring = r; ring_len = len; block_len = block ? block : len; ring_pos = 0; irq = on_block; dma_running = true;
    SDL_UnlockAudioDevice(audio_dev);
    return true;
}
size_t audio_position() { return ring_pos; }   // read under audio_lock() for consistency
void audio_stop() {
    if (!audio_dev) return;
    SDL_LockAudioDevice(audio_dev);
    dma_running = false; ring = nullptr;
    SDL_UnlockAudioDevice(audio_dev);
}
void audio_lock() { if (audio_dev) SDL_LockAudioDevice(audio_dev); }
void audio_unlock() { if (audio_dev) SDL_UnlockAudioDevice(audio_dev); }
bool audio_available() {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) return false;
    if (!audio_dev) open_audio(audio_rate);
    return audio_dev != 0;
}

void speaker_on(unsigned hz) {
    if (!audio_available()) return;
    audio_lock(); spk_hz = hz; audio_unlock();
}
void speaker_off() { audio_lock(); spk_hz = 0; audio_unlock(); }

uint64_t frame_count() { return frames; }

void save_frame(const char *path) {
    FILE *f = std::fopen(path, "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        uint32_t c = lut[vram[i]];
        uint8_t rgb[3] = {uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
}

}  // namespace plat
