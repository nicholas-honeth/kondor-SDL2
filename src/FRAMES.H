// FRAMES.H - simple frame animations (reconstructed, FRAMES.H lines 1-23)

void frames_type::add(int w, int h, uchar *sprt)
{
    frames[maxframe].w = w;
    frames[maxframe].h = h;
    frames[maxframe].sprt = sprt;
    maxframe++;
}

// Draws the current frame centred on map position (x,y) if it is on screen.
void frames_type::draw(int x, int y)
{
    x -= frames[frame].w >> 1;
    y -= frames[frame].h >> 1;
    if (-frames[frame].w < x - scroll_x && frames[frame].w + 256 > x - scroll_x &&
        -frames[frame].h < y - scroll_y && frames[frame].h + 192 > y - scroll_y)
        kdrawsprite(x - scroll_x, y - scroll_y, frames[frame].w, frames[frame].h, frames[frame].sprt);
}

frames_type::frames_type()
{
    frame = 0;
    maxframe = 0;
}
