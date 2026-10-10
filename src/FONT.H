// FONT.H - 5x5 bitmap font from KONDOR.FNT (reconstructed, FONT.H lines 9-121)
//
// _font holds the 62 glyphs (chars '!'..'^') "shaded" by convsprt into colours 90..105,
// _regfont holds the raw glyph masks (drawn in the current colour by drawcharreg).

// Shades a w*h sprite: each pixel becomes a weighted sum of its 3x3 neighbourhood.
// Note: like the original, col is NOT reset between pixels, so each result carries a
// 1/16 share of the previous one into the next.
void convsprt(int w, int h, char *sprt)
{
    char *temp = (char *)malloc(w * h);
    if (temp == NULL) memerr("CONVERT SPRITES");
    int col = 0;
    for (int iy = 0; iy < h; iy++) {
        for (int ix = 0; ix < w; ix++) {
            if (ix > 0) {
                if (iy > 0) col += sprt[(iy - 1) * w + ix - 1];
                col += sprt[iy * w + ix - 1];
                if (iy < h - 1) col += sprt[(iy + 1) * w + ix - 1];
            }
            if (iy > 0) col += sprt[(iy - 1) * w + ix];
            if (iy < h - 1) col += sprt[(iy + 1) * w + ix];
            if (ix < w - 1) {
                if (iy > 0) col += sprt[(iy - 1) * w + ix + 1];
                col += sprt[iy * w + ix + 1];
                if (iy < h - 1) col += sprt[(iy + 1) * w + ix + 1];
            }
            col += sprt[iy * w + ix] * 8;
            col /= 16;
            temp[iy * w + ix] = (char)(col + 90);
        }
    }
    memcpy(sprt, temp, w * h);
    free(temp);
}

void drawchar(int x, int y, char ch, uchar *fnt)
{
    ch -= 0x21;
    if (ch >= 0 && ch < 62) drawsprite(x, y, 5, 5, fnt + ch * 25);
}

void drawcharreg(int x, int y, char ch, uchar *fnt)
{
    ch -= 0x21;
    if (ch >= 0 && ch < 62) drawsprites(x, y, 5, 5, fnt + ch * 25);
}

// Port: the original strupr()'d the caller's string in place (including string literals);
// we upper-case a copy instead.
void drawstring(int x, int y, const char *s, uchar *fnt)
{
    char str[256];
    strncpy(str, s, sizeof str - 1);
    str[sizeof str - 1] = 0;
    strupr(str);
    for (unsigned chi = 0; chi < strlen(str); chi++) drawchar(x + chi * 6, y, str[chi] & 0x7F, fnt);
}

void drawstring(int x, int y, unsigned long n, uchar *fnt)
{
    char str[16];
    ultoa(n, str, 10);
    for (unsigned chi = 0; chi < strlen(str); chi++) drawchar(x + chi * 6, y, str[chi] & 0x7F, fnt);
}

void drawstringreg(int x, int y, const char *s, uchar *fnt)
{
    char str[256];
    strncpy(str, s, sizeof str - 1);
    str[sizeof str - 1] = 0;
    strupr(str);
    for (unsigned chi = 0; chi < strlen(str); chi++) drawcharreg(x + chi * 6, y, str[chi] & 0x7F, fnt);
}

void drawstringreg(int x, int y, unsigned long n, uchar *fnt)
{
    char str[16];
    ultoa(n, str, 10);
    for (unsigned chi = 0; chi < strlen(str); chi++) drawcharreg(x + chi * 6, y, str[chi], fnt);
}

void load_font(const char *filename)
{
    FILE *in = fopen_ci(filename, "rb");   // original: "r+" (text mode; the file has no CR/LF/^Z bytes)
    if (in == NULL) filerr();
    fread_z(font, 62, 25, in);
    fclose(in);
    memcpy(regfont, font, 1550);
    for (int fi = 0; fi < 62; fi++) convsprt(5, 5, (char *)font + fi * 25);
}

// Palette entries 90..105: a 16-step grey ramp for the shaded font.
void set_font_pal()
{
    for (int i = 0; i < 16; i++) {
        gamepal[(90 + i) * 3] = i << 2;
        gamepal[(90 + i) * 3 + 1] = i << 2;
        gamepal[(90 + i) * 3 + 2] = i << 2;
    }
}

void init_font()
{
    font = (uchar *)malloc(1550);
    if (font == NULL) memerr("FONT");
    regfont = (uchar *)malloc(1550);
    if (regfont == NULL) memerr("FONT");
    load_font("kondor.fnt");
    set_font_pal();
}

void close_font()
{
    free(font);
    free(regfont);
}
