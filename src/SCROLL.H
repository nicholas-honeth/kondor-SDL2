// SCROLL.H - images, tile map rendering, water animation, minimap, scrolling
// (reconstructed, SCROLL.H lines 7-440)
//
// The map is 96x96 tiles of 16x16 pixels (tile n = 256 bytes at tiles + n*256, 115 tiles
// in KONDOR.DAT). kpixels[y] = y*96 is the map row offset. The play view is 16x12 tiles;
// the map is rendered into _bkg only when the view scrolls.

int loadsprites()
{
    tiles = (uchar *)malloc(0x7300);
    if (tiles == NULL) memerr("LOADSPRITES");
    memset(tiles, 0, 0x2C00);
    FILE *fload = fopen_ci("kondor.dat", "rb");
    if (fload == NULL) filerr();
    fread_z(tiles, 0x73, 0x100, fload);
    fclose(fload);
    return 1;
}

void setimagesfile(const char *str)
{
    strcpy(IMAGES_FNAME, str);
}

// Image files are a sequence of {int16 w, int16 h, w*h bytes}. Loads image number n.
int loadimg(int n, uchar **img)
{
    FILE *fil = fopen_ci(IMAGES_FNAME, "rb");
    if (fil == NULL) filerr();
    for (int i = 0; i <= n; i++) {
        int w = read_le16(fil);
        int h = read_le16(fil);
        if (i == n) {
            free(*img);
            *img = (uchar *)malloc(w * h);
            if (*img == NULL) memerr("IMAGES");
            fread_z(*img, 1, w * h, fil);
            fclose(fil);
            return 1;
        }
        for (int fs = 0; fs < w * h; fs++) fgetc(fil);
    }
    fclose(fil);   // port: the original leaked the handle here
    return -1;
}

void loadimages()
{
    setimagesfile("kondor2.dat");
    if (!loadimg(0, &bullet_img)) filerr();
    bullets_sprts[0].set(0, 0, 2, 2, 0, 5, 5, bullet_img, true);
    loadimg(1, &tank_img);
    loadimg(2, &cannon_img);
    loadimg(3, &expl1);
    loadimg(4, &expl2);
    loadimg(5, &expl3);
    loadimg(6, &expl4);
    loadimg(7, &texpl1);
    loadimg(8, &texpl2);
    loadimg(9, &texpl3);
    loadimg(10, &texpl4);
    loadimg(11, &texpl5);
    loadimg(12, &skey_img);
    loadimg(13, &cross_img);
    loadimg(14, &rocket_img);
    bullets_sprts[1].set(0, 0, 8, 5, 0, 15, 9, rocket_img, true);
    loadimg(15, &shield_img);
    loadimg(16, &fly_img);
    loadimg(17, &turpad_img);
    loadimg(18, &mig_img);
    loadimg(19, &fshoe_img);
    loadimg(20, &turcan_img);
    setimagesfile("kondor3.dat");
    if (!loadimg(0, &start_s)) filerr();
    loadimg(1, &startc_s);
    loadimg(2, &resume_s);
    loadimg(3, &resumec_s);
    loadimg(4, &options_s);
    loadimg(5, &optionsc_s);
    loadimg(6, &help_s);
    loadimg(7, &helpc_s);
    loadimg(8, &exit_s);
    loadimg(9, &exitc_s);
    loadimg(10, &gameover_s);
    loadimg(11, &missacco_s);
    loadimg(12, &scores_s);
    loadimg(13, &scoresc_s);
}

void closeimages()
{
    uchar **imgs[] = {&bullet_img, &tank_img, &cannon_img, &expl1, &expl2, &expl3, &expl4, &texpl1,
                      &texpl2, &texpl3, &texpl4, &texpl5, &skey_img, &cross_img, &rocket_img, &shield_img,
                      &fly_img, &turpad_img, &mig_img, &fshoe_img, &turcan_img, &start_s, &startc_s,
                      &resume_s, &resumec_s, &options_s, &optionsc_s, &help_s, &helpc_s, &exit_s,
                      &exitc_s, &gameover_s, &missacco_s, &scores_s, &scoresc_s};
    for (uchar **p : imgs) {
        free(*p);
        *p = NULL;
    }
}

// size in 16-bit words / 32-bit dwords, like the original rep movsw / rep movsd
void qmemcpy(uchar *dest, uchar *src, int size) { memcpy(dest, src, size * 2); }
void kmemcpy(uchar *dest, uchar *src, int size) { memcpy(dest, src, size * 4); }

// Partial tiles at the view edges: left/right columns are 'left' pixels wide offsets,
// top/bottom rows likewise.
void ldrawcell(int y, int n, int left)
{
    int draw_x = 16 - left;
    for (int iy = 0, ty = y; iy < 16; iy++, ty++)
        memcpy(gbuffer + qpixels[ty], tiles + (n << 8) + (iy << 4) + left, draw_x);
}

void rdrawcell(int y, int n, int left)
{
    int start_x = 256 - left;
    for (int iy = 0, ty = y; iy < 16; iy++, ty++)
        memcpy(gbuffer + qpixels[ty] + start_x, tiles + (n << 8) + (iy << 4), left);
}

void udrawcell(int x, int n, int left)
{
    for (int iy = left, ty = 0; iy < 16; iy++, ty++)
        memcpy(gbuffer + qpixels[ty] + x, tiles + (n << 8) + (iy << 4), 16);
}

void ddrawcell(int x, int n, int left)
{
    for (int iy = 0, ty = 192 - left; iy < left; iy++, ty++)
        memcpy(gbuffer + qpixels[ty] + x, tiles + (n << 8) + (iy << 4), 16);
}

void fdrawcell(int x, int y, int n)
{
    for (int iy = 0, ty = y; iy < 16; iy++, ty++)
        kmemcpy(gbuffer + qpixels[ty] + x, tiles + (n << 8) + (iy << 4), 4);
}

// A tile clipped on any side (used for the four corners).
void drawcell(int x, int y, int n)
{
    int sx = 0, sy = 0, ex = 0, ey = 0;
    if (x < 0) sx = -x;
    else if (x + 16 > 256) ex = x - 240;
    if (y < 0) sy = -y;
    else if (y + 16 > 192) ey = y - 176;
    x += sx;
    for (int iy = sy, ty = y + sy; 16 - ey > iy; iy++, ty++)
        memcpy(gbuffer + qpixels[ty] + x, tiles + (n << 8) + (iy << 4) + sx, 16 - ex - sx);
}

void drawmap()
{
    int left_x = scroll_x, x = (unsigned)scroll_x >> 4;
    int left_y = scroll_y, y = (unsigned)scroll_y >> 4;
    left_x &= 15;
    left_y &= 15;
    int tx, ty;
    for (int ix = x + 1, tx = 16 - left_x; ix < x + 16; ix++, tx += 16)
        for (int iy = y + 1, ty = 16 - left_y; iy < y + 12; iy++, ty += 16)
            fdrawcell(tx, ty, map[kpixels[iy] + ix]);
    tx = 16 - left_x;
    for (int ix = x + 1; ix < x + 16; ix++, tx += 16) {
        udrawcell(tx, map[kpixels[y] + ix], left_y);
        ddrawcell(tx, map[kpixels[y + 12] + ix], left_y);
    }
    ty = 16 - left_y;
    for (int iy = y + 1; iy < y + 12; iy++, ty += 16) {
        ldrawcell(ty, map[kpixels[iy] + x], left_x);
        rdrawcell(ty, map[kpixels[iy] + x + 16], left_x);
    }
    drawcell(-left_x, -left_y, map[kpixels[y] + x]);
    drawcell(256 - left_x, -left_y, map[kpixels[y] + x + 16]);
    drawcell(-left_x, 192 - left_y, map[(y + 12) * 96 + x]);
    drawcell(256 - left_x, 192 - left_y, map[(y + 12) * 96 + x + 16]);
}

void set_map(int x, int y, int val)
{
    map[kpixels[y] + x] = val;
}

// Animated water: tiles 0/1 toggle, and the water-edge tiles flip between two 8-tile sets.
void checkwater(int offset)
{
    uchar c = map[offset];
    if (c < 2) {
        uchar old = map[offset]++;
        if (old >= 1) map[offset] = 0;
    }
    if (c >= 0x15 && c <= 0x1C) map[offset] += 8;
    if (c >= 0x1D && c <= 0x24) map[offset] -= 8;
    if (c >= 0x25 && c <= 0x2C) map[offset] += 8;
    if (c >= 0x2D && c <= 0x34) map[offset] -= 8;
    if (c >= 0x35 && c <= 0x3C) map[offset] += 8;
    if (c >= 0x3D && c <= 0x44) map[offset] -= 8;
    if (c >= 0x55 && c <= 0x5C) map[offset] += 8;
    if (c >= 0x5D && c <= 0x64) map[offset] -= 8;
}

void movewater()
{
    int x = (unsigned)scroll_x >> 4;
    int y = (unsigned)scroll_y >> 4;
    for (int i = x; i < x + 16; i++)
        for (int j = y; j < y + 12; j++) checkwater(kpixels[j] + i);
    if (x < 80)
        for (int j = y; j < y + 12; j++) checkwater(kpixels[j] + x + 16);
    if (y < 84)
        for (int i = x; i < x + 16; i++) checkwater(kpixels[y + 12] + i);
}

int water(uchar value)
{
    return value <= 1 || (value >= 0x15 && value <= 0x34) ? 1 : 0;
}

// Port: the *xy() lookups are bounds-checked; outside the 96x96 map the original read
// unrelated memory. Off-map cells count as blocking, non-water.
static inline int onmapcell(int x, int y)
{
    return x >= 0 && y >= 0 && x < 96 && y < 96;
}

uchar maptile(int x, int y)
{
    return onmapcell(x, y) ? map[kpixels[y] + x] : 0xFF;
}

int waterxy(int x, int y)
{
    return onmapcell(x, y) ? water(map[kpixels[y] + x]) : 0;
}

int blocking(uchar value)
{
    return (value > 0x64 && value < 0x73) || value == 0xFF ? 1 : 0;
}

int blockingxy(int x, int y)
{
    return onmapcell(x, y) ? blocking(map[kpixels[y] + x]) : 1;
}

int minblocking(uchar value)
{
    return value == 0x64 || value == 0 || value == 0x48 ? 1 : 0;
}

int minblockingxy(int x, int y)
{
    return onmapcell(x, y) ? minblocking(minmap[kpixels[y] + x]) : 1;
}

// Minimap colour of a map tile.
uchar getmapcol(uchar tile)
{
    if (tile == 0 || tile == 1) {
        switch (random(5)) {
        case 0: return 0x04;
        case 1: return 0x05;
        case 2: return 0x07;
        case 3: return 0x0A;
        case 4: return 0x0B;
        }
    }
    switch (tile) {
    case 0x07: return 0x1D;
    case 0x08: return 0x1E;
    case 0x09: return 0x1F;
    case 0x0A: return 0x20;
    case 0x0B: return 0x21;
    case 0x0C: return 0x1E;
    case 0x0D: return 0x20;
    case 0x0E: return 0x2C;
    case 0x0F: return 0x32;
    case 0x10: return 0x34;
    case 0x11: return 0x2F;
    case 0x12: return 0x3A;
    case 0x13: return 0x3B;
    case 0x14: return 0x37;
    case 0xFD: return 0x21;
    case 0xFE: return 0x0B;
    case 0xFF: return 0x48;
    }
    if ((tile >= 0x15 && tile <= 0x24) || (tile >= 0x35 && tile <= 0x44) || (tile >= 0x45 && tile <= 0x4C))
        return 0xA0;
    if ((tile >= 0x25 && tile <= 0x34) || (tile >= 0x55 && tile <= 0x64) || (tile >= 0x4D && tile <= 0x54))
        return 0xB7;
    if (tile >= 0x65) return 0x64;
    return 0;
}

void setminmap()
{
    for (int i = 0; i < 0x2400; i++) minmap[i] = getmapcol(map[i]);
}

// Restore the 5-pixel cross around (x2,y2) (map pixels) on the minimap.
void clearminmap(int x2, int y2)
{
    int x = x2 >> 4, y = y2 >> 4;
    int offset = kpixels[y] + x;
    // Port: the original could touch one row outside the map at the edges.
    static const int d[5] = {-96, -1, 0, 1, 96};
    for (int k = 0; k < 5; k++) {
        int o = offset + d[k];
        if (o >= 0 && o < 0x2400) minmap[o] = getmapcol(map[o]);
    }
}

void drawonminmap(int x2, int y2, int col, int ccol)
{
    int x = x2 >> 4, y = y2 >> 4;
    int offset = kpixels[y] + x;
    static const int d[5] = {-96, -1, 0, 1, 96};
    for (int k = 0; k < 5; k++) {
        int o = offset + d[k];
        if (o >= 0 && o < 0x2400) minmap[o] = k == 2 ? ccol : col;
    }
}

void initminmap()
{
    minmap = (uchar *)malloc(0x2400);
    if (minmap == NULL) memerr("MINI MAP");
}

void kpixelstable()
{
    for (int i = 0; i < 96; i++) kpixels[i] = (i << 5) + (i << 6);
}

void closeminmap()
{
    free(minmap);
}

void drawminmap()
{
    int sx = (scroll_x >> 4) - 16;
    int sy = (scroll_y >> 4) - 16;
    if (sx < 1) sx = 1;
    if (sy < 1) sy = 1;
    if (sx > 47) sx = 47;
    if (sy > 47) sy = 47;
    for (int ix = 0; ix < 48; ix++)
        for (int iy = 0; iy < 48; iy++) plotpixel(ix + 263, iy + 8, minmap[kpixels[sy + iy] + sx + ix]);
}

void drawscroll()
{
    screenis(bkg);
    drawmap();
    scrollmove = false;
    screenis(buffer);
}

void check4scroll()
{
    double x = tank.layers.x;
    double y = tank.layers.y;
    if (x - scroll_x > 161.0 && scroll_x < 1280) {
        scroll_x = ftol(x - scroll_x - 161.0 + 1 + scroll_x);
        scrollmove = true;
    }
    if (x - scroll_x < 95.0 && scroll_x > 0) {
        scroll_x = ftol(scroll_x + (x - scroll_x - 95.0));
        scrollmove = true;
    }
    if (y - scroll_y > 97.0 && scroll_y < 1344) {
        scroll_y = ftol(y - scroll_y - 97.0 + 1 + scroll_y);
        scrollmove = true;
    }
    if (y - scroll_y < 95.0 && scroll_y > 0) {
        scroll_y = ftol(scroll_y + (y - scroll_y - 95.0));
        scrollmove = true;
    }
    if (scroll_x < 0) scroll_x = 0;
    if (scroll_x > 1280) scroll_x = 1280;
    if (scroll_y < 0) scroll_y = 0;
    if (scroll_y > 1344) scroll_y = 1344;
    if (scrollmove) drawscroll();
}
