// QUNIT.H - low-level unit: VGA mode 13h, palette, primitives, keyboard, Sound Blaster DSP.
// Reconstructed from KONDOR.EXE (QUNIT.H lines 47-481). Hardware access goes through platform.h.
//
// Port notes (deviations from the 1999 binary, all for stability):
//  * far pointers / segment juggling (_sgbuffer, _svidbuf) are gone: a draw target is a plain
//    pointer; buffers are used from their real start (the original used seg:0000 and so wrote
//    over the heap header of every malloc'ed screen).
//  * loadpal() no longer free()s its stack array, writestrxy(int) no longer itoa()s into a
//    1-byte string literal.
//  * the keyboard ISR is fed by the platform layer with the same XT scancode byte stream.

void init320x200()
{
    video_buffer = plat::vga();
    gbuffer = video_buffer;
    plat::set_mode13(true);
}

void closegraph()
{
    plat::set_mode13(false);
}

void qpixelstable()
{
    for (int i = 0; i < 200; i++) qpixels[i] = (i << 8) + (i << 6);
}

void setpal(char col, char r, char g, char b)
{
    plat::dac_write((uchar)col, (uchar)r, (uchar)g, (uchar)b);
}

void getpal(char col, char *red, char *green, char *blue)
{
    uchar r2, g2, b2;
    plat::dac_read((uchar)col, &r2, &g2, &b2);
    *red = r2;
    *green = g2;
    *blue = b2;
}

void waitretrace()
{
    plat::wait_retrace();
}

void fadeout()
{
    uchar pal[3];
    for (int c = 0; c < 64; c++) {
        waitretrace();
        for (int i = 0; i < 256; i++) {
            getpal(i, (char *)&pal[0], (char *)&pal[1], (char *)&pal[2]);
            if (pal[0] > 0) pal[0]--;
            if (pal[1] > 0) pal[1]--;
            if (pal[2] > 0) pal[2]--;
            setpal(i, pal[0], pal[1], pal[2]);
        }
    }
}

void fadein(char *savedpal)
{
    uchar pal[3];
    for (int c = 0; c < 64; c++) {
        waitretrace();
        for (int i = 0; i < 256; i++) {
            getpal(i, (char *)&pal[0], (char *)&pal[1], (char *)&pal[2]);
            if (pal[0] < 64 && (uchar)savedpal[i * 3] > pal[0]) pal[0]++;
            if (pal[1] < 64 && (uchar)savedpal[i * 3 + 1] > pal[1]) pal[1]++;
            if (pal[2] < 64 && (uchar)savedpal[i * 3 + 2] > pal[2]) pal[2]++;
            setpal(i, pal[0], pal[1], pal[2]);
        }
    }
}

void blackpal()
{
    for (int i = 0; i < 256; i++) setpal(i, 0, 0, 0);
}

void plotpixel(unsigned x, unsigned y, uchar color)
{
    if (x <= 319 && y <= 199) gbuffer[qpixels[y] + x] = color;
}

void setcolor(uchar color)
{
    gcol = color;
}

void cleardevice()
{
    memset(gbuffer, 0, 64000);
}

void swapint(int *num1, int *num2)
{
    int num3 = *num1;
    *num1 = *num2;
    *num2 = num3;
}

void swapdouble(double *num1, double *num2)
{
    double num3 = *num1;
    *num1 = *num2;
    *num2 = num3;
}

// verline/horline/bar do no clipping, exactly like the original; callers keep them on screen.
void verline(int x, int y, int y2)
{
    if (y > y2) swapint(&y, &y2);
    unsigned h = y2 - y + 1;
    uchar *p = gbuffer + y * 320 + x;
    while (h--) { *p = gcol; p += 320; }
}

void horline(int y, int x, int x2)
{
    if (x > x2) swapint(&x, &x2);
    unsigned w = x2 - x + 1;
    memset(gbuffer + y * 320 + x, gcol, w);
}

void bar(int x, int y, int x2, int y2)
{
    if (x > x2) swapint(&x, &x2);
    if (y > y2) swapint(&y, &y2);
    unsigned w = x2 - x + 1;
    unsigned h = y2 - y + 1;
    uchar *p = gbuffer + y * 320 + x;
    while (h--) { memset(p, gcol, w); p += 320; }
}

void drawsprite(int x, int y, int w, int h, uchar *sprt)
{
    for (int iy = 0; iy < h; iy++)
        for (int ix = 0; ix < w; ix++)
            if (sprt[iy * w + ix] != 0) plotpixel(x + ix, y + iy, sprt[iy * w + ix]);
}

// Same as drawsprite, but every opaque pixel is drawn in the current colour (used for shadows/text).
void drawsprites(int x, int y, int w, int h, uchar *sprt)
{
    for (int iy = 0; iy < h; iy++)
        for (int ix = 0; ix < w; ix++)
            if (sprt[iy * w + ix] != 0) plotpixel(x + ix, y + iy, gcol);
}

void circle(int x, int y, int radius)
{
    for (int index = 0; index < 511; index++)
        plotpixel(ftol(x + radius * costbl[index & 511]), ftol(y + radius * sintbl[index & 511]), gcol);
}

void drawscreen(uchar *buf)
{
    memcpy(video_buffer, buf, 64000);
    // port: menus and option screens redraw in busy loops without waiting for the retrace;
    // let the platform pump events and pace those loops at the 70 Hz refresh.
    plat::poll();
}

void copyscreen(uchar *dest, uchar *src)
{
    memcpy(dest, src, 64000);
}

void screenis(uchar *buf)
{
    gbuffer = buf;
}

void closevirscr(uchar *buffer)
{
    free(buffer);
    screenis(video_buffer);
}

void maketbls()
{
    for (int i = 0; i < 512; i++) {
        costbl[i] = cos(i * M_PI / 256);
        sintbl[i] = sin(i * M_PI / 256);
    }
}

int loadpal(const char *fname)
{
    uchar pal[768];
    FILE *fil = fopen_ci(fname, "rb");
    if (fil == NULL) return 0;
    fread_z(pal, 768, 1, fil);
    fclose(fil);
    for (int index = 0; index < 256; index++)
        setpal(index, pal[index * 3], pal[index * 3 + 1], pal[index * 3 + 2]);
    return 1;
}

// --- keyboard: INT 9 handler -------------------------------------------------------
// keypress[] is indexed by scancode, +0x80 for E0-prefixed (extended) keys.
static void key_interrupt(uchar ch)
{
    uchar ext = 0;
    if (nextisext) {
        ext = 0x80;
        nextisext = 0;
    }
    if (ch == 0xE0) nextisext = 1;
    keypress[(ch & 0x7F) + ext] = ch < 0x80 ? 1 : 0;
}

void getskeys()
{
    memset(keypress, 0, 256);
    plat::set_scancode_handler(key_interrupt);
}

int keydown(int nkey)
{
    return keypress[nkey] ? 1 : 0;
}

int skeypressed()
{
    plat::idle();   // port: the original spun on the ISR-updated table; we must pump events
    for (int i = 0; i < 256; i++)
        if (keypress[i]) return i;
    return 0;
}

void clearskeys()
{
    memset(keypress, 0, 256);
}

void closeskeys()
{
    plat::set_scancode_handler(nullptr);
}

// --- Sound Blaster DSP ------------------------------------------------------------
// The DSP itself is gone; the platform audio device plays the DMA ring. These keep the
// original call structure so SOUND.H stays close to the original.
void WriteDSP(uchar value)
{
    (void)value;
}

int ResetDSP(unsigned test)
{
    if (!plat::audio_available()) return 0;
    Base = test;
    return 1;
}

uchar ReadDSP()
{
    return 0xAA;
}

int initsb()
{
    for (int i = 1; i < 9; i++) {
        if (i == 7) continue;
        if (ResetDSP(0x200 + (i << 4))) return 1;
    }
    return 0;
}

void closesb()
{
    WriteDSP(0xD0);   // halt DMA
    WriteDSP(0xDA);   // exit auto-init
}

// --- text with the BIOS 8x8 ROM font ----------------------------------------------
void plotchar(int x, int y, char c)
{
    const uchar *work_char = rom_char_set + ((signed char)c) * 8;
    for (int y2 = 0; y2 < 8; y2++) {
        uchar bit_mask = 0x80;
        for (int x2 = 0; x2 < 8; x2++) {
            if (*work_char & bit_mask) plotpixel(x + x2, y + y2, gcol);
            bit_mask >>= 1;
        }
        work_char++;
    }
}

void writestrxy(int x, int y, const char *string)
{
    for (int index = 0; string[index] != 0; index++)
        plotchar(x + index * 8, y, string[index] & 0x7F);
}

void writestrxy(int x, int y, int number)
{
    char string[16];
    itoa(number, string, 10);
    for (unsigned index = 0; index < strlen(string); index++)
        plotchar(x + index * 8, y, string[index]);
}

int getvirscr(uchar **buf)
{
    *buf = (uchar *)malloc(64000);
    if (*buf == NULL) return 0;
    gbuffer = *buf;
    cleardevice();
    return 1;
}

int load32(const char *fname)
{
    FILE *file3x2 = fopen_ci(fname, "rb");
    if (file3x2 == NULL) return 0;
    fread_z(gbuffer, 64000, 1, file3x2);
    fclose(file3x2);
    return 1;
}
