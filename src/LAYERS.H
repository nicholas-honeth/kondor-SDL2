// LAYERS.H - rotated sprites and stacks of sprite layers (reconstructed, LAYERS.H lines 12-54)

void sprite_type::set(double nx, double ny, double ncx, double ncy, int nang, int nw, int nh, uchar *ndata,
                      boolean nvisible)
{
    x = nx;
    y = ny;
    cx = ncx;
    cy = ncy;
    ang = nang;
    w = nw;
    h = nh;
    data = ndata;
    visible = nvisible;
}

// (px,py) is the owner's screen position, pang its angle; the sprite adds its own offsets.
void sprite_type::draw(double px, double py, double pang)
{
    if (visible)
        krotsprite(ftol(x + px), ftol(y + py), ftol(cx), ftol(cy), ftol(ang + pang), w, h, data);
}

void sprt_layers_type::set(double nx, double ny, int nang, unsigned nsprts_no)
{
    x = nx;
    y = ny;
    ang = nang;
    sprts_no = nsprts_no;
}

void sprt_layers_type::draw()
{
    for (unsigned s_index = 0; s_index < sprts_no; s_index++)
        sprts[s_index].draw(x - scroll_x, y - scroll_y, ang);
}

void sprt_layers_type::close()
{
    sprts_no = 0;
}
