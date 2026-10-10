// EXPL.H - explosions (reconstructed, EXPL.H lines 10-80)
// type 0: small explosion (4 frames), type 1: tank explosion (5 frames).
// The frame shown is (45 - age) / 5, so the start age selects how much of the animation plays.

int expl_type::dead()
{
    return age <= 0 ? 1 : 0;
}

void expl_type::draw()
{
    frames.draw(x, y);
}

void expl_type::handle()
{
    age--;
    frames.frame = (45 - age) / 5;
}

// Original behaviour kept: a dead explosion is replaced by the last one, which is then
// advanced in the same pass.
void expls_type::handle()
{
    for (int e_index = 0; e_index < expls_no; e_index++) {
        if (data[e_index].dead()) {
            expls_no--;
            data[e_index] = data[expls_no];
        }
        data[e_index].handle();
    }
}

void expls_type::draw()
{
    for (int e_index = 0; e_index < expls_no; e_index++) data[e_index].draw();
}

void expls_type::add(int x, int y, int age, int type)
{
    if (expls_no >= 10) return;
    expl_type &e = data[expls_no];
    e.x = x;
    e.y = y;
    e.age = age;
    e.frames.frame = 0;
    e.frames.maxframe = 0;
    if (type == 0) {
        e.frames.add(7, 6, expl1);
        e.frames.add(7, 9, expl2);
        e.frames.add(9, 10, expl3);
        e.frames.add(9, 11, expl4);
    }
    if (type == 1) {
        e.frames.add(14, 11, texpl1);
        e.frames.add(14, 13, texpl2);
        e.frames.add(16, 14, texpl3);
        e.frames.add(16, 16, texpl4);
        e.frames.add(16, 15, texpl5);
    }
    expls_no++;
}

expls_type::expls_type()
{
    expls_no = 0;
}
