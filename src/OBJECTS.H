// OBJECTS.H - 16x16 map objects that the tank collects (reconstructed, OBJECTS.H lines 10-73)

void object_type::draw()
{
    int tx = x - scroll_x;
    int ty = y - scroll_y;
    if (-w < tx && -h < ty && tx < 256 && ty < 192) kdrawsprite(tx, ty, w, h, sprt);
}

// Returns the type of the object the rotated rect touches (and removes it), else -1.
int objects_type::checkvehiclehit(int x, int y, int ang, int Alpha, double Radius)
{
    for (unsigned b_index = 0; b_index < objects_no; b_index++) {
        object_type &o = data[b_index];
        if (krectinrot(o.x, o.y, o.x + o.w, o.y + o.h, x, y, ang, Alpha, Radius)) {
            int temp = o.type;
            objects_no--;
            data[b_index] = data[objects_no];
            return temp;
        }
    }
    return -1;
}

// (Original: tests a fixed point at (1,1); the real pick-up test is in tank_type::handle.)
void objects_type::handle()
{
    int type = checkvehiclehit(1, 1, 1, 1, 1.0);
    if (type != -1) slots.add(type);
}

void objects_type::draw()
{
    for (unsigned b_index = 0; b_index < objects_no; b_index++) data[b_index].draw();
}

void objects_type::add(double x, double y, int type)
{
    if (objects_no >= 40) return;
    object_type &o = data[objects_no];
    o.x = ftol(x);
    o.y = ftol(y);
    o.w = 16;
    o.h = 16;
    o.type = type;
    o.sprt = rock_img;
    objects_no++;
}

objects_type::objects_type()
{
    objects_no = 0;
}
