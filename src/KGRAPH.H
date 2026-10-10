// KGRAPH.H - drawing clipped to the 256x192 play field (reconstructed, KGRAPH.H lines 1-154)

void kplotpixel(unsigned x, unsigned y, uchar color)
{
    if (x < 256 && y < 192) gbuffer[qpixels[y] + x] = color;
}

// Rotated sprite: the source is walked in float steps along the rotated axes. Each source
// pixel is plotted twice, half a step apart, so the rotated image has no holes.
// (cx,cy) is the rotation centre inside the sprite.
void krotsprite(int x, int y, int cx, int cy, int ang, int w, int h, uchar *sprt)
{
    float x_step = costbl[ang & 511];
    float y_step = sintbl[ang & 511];
    float x_step2 = costbl[(ang + 128) & 511];
    float y_step2 = sintbl[(ang + 128) & 511];
    float start_x = x - ((double)cx * x_step + (double)cy * x_step2);
    float start_y = y - ((double)cx * y_step + (double)cy * y_step2);
    x_step = x_step / 2.0f;
    y_step = y_step / 2.0f;
    int offset = 0;
    for (int iy = 0; iy < h; iy++) {
        float temp_x = start_x;
        float temp_y = start_y;
        for (int ix = 0; ix < w; ix++) {
            for (int i = 0; i < 2; i++) {
                if (sprt[offset] != 0) kplotpixel(ftol(temp_x), ftol(temp_y), sprt[offset]);
                temp_x = x_step + temp_x;
                temp_y = y_step + temp_y;
            }
            offset++;
        }
        start_x = x_step2 + start_x;
        start_y = y_step2 + start_y;
    }
}

void kdrawsprite(int x, int y, int w, int h, uchar *sprt)
{
    for (int iy = 0; iy < h; iy++)
        for (int ix = 0; ix < w; ix++)
            if (sprt[iy * w + ix] != 0) kplotpixel(x + ix, y + iy, sprt[iy * w + ix]);
}

// Rotated sprite partly sunk in water: outside the inner rectangle (sink_* margins) the
// pixel is replaced by the background 4 pixels to the left, in its shadow colour (+128).
void drawsink(int x, int y, int cx, int cy, int ang, int w, int h, uchar *sprt, int sink_w1, int sink_w2,
              int sink_h1, int sink_h2)
{
    float x_step = costbl[ang & 511];
    float y_step = sintbl[ang & 511];
    float x_step2 = costbl[(ang + 128) & 511];
    float y_step2 = sintbl[(ang + 128) & 511];
    float start_x = x - ((double)cx * x_step + (double)cy * x_step2);
    float start_y = y - ((double)cx * y_step + (double)cy * y_step2);
    x_step = x_step / 2.0f;
    y_step = y_step / 2.0f;
    for (int iy = 0; iy < h; iy++) {
        float temp_x = start_x;
        float temp_y = start_y;
        for (int ix = 0; ix < w; ix++) {
            int offset = iy * w + ix;
            for (int i = 0; i < 2; i++) {
                if (sprt[offset] != 0) {
                    uchar c;
                    if (ix >= sink_w1 && w - sink_w2 >= ix && iy >= sink_h1 && h - sink_h2 >= iy) {
                        c = sprt[offset];
                    } else {
                        // Port: the original read outside the buffer near the top/left edge.
                        int ty = ftol(temp_y), tx = ftol(temp_x);
                        int idx = (ty >= 0 && ty < 200) ? qpixels[ty] + tx - 4 : -1;
                        c = (idx >= 0 && idx < 64000 ? buffer[idx] : 0) | 0x80;
                    }
                    kplotpixel(ftol(temp_x), ftol(temp_y), c);
                }
                temp_x = x_step + temp_x;
                temp_y = y_step + temp_y;
            }
        }
        start_x = x_step2 + start_x;
        start_y = y_step2 + start_y;
    }
}

void khorline(int y, int x, int x2)
{
    if (x > x2) swapint(&x, &x2);
    if (x2 < 0 || x > 256 || y >= 192) return;
    if (x2 >= 256) x2 = 255;
    if (y < 0) return;
    unsigned w = x2 - x + 1;
    if (x <= 0) {
        w = x2 + 1;
        x = 0;
    }
    if ((int)w > 0) memset(gbuffer + qpixels[y] + x, gcol, w);
}

void kverline(int x, int y, int y2)
{
    if (y > y2) swapint(&y, &y2);
    if (y2 < 0 || y >= 192 || x >= 256) return;
    if (y2 >= 192) y2 = 191;
    if (x < 0) return;
    unsigned h = y2 - y + 1;
    if (y <= 0) {
        h = y2 + 1;
        y = 0;
    }
    uchar *p = gbuffer + qpixels[y] + x;
    while (h--) { *p = gcol; p += 320; }
}
