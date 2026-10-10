// BASES.H - enemy generator bases, 43x21 px (reconstructed, BASES.H lines 1-136)
// Every 'gap' frames a base spawns a RANDOM or FOLLOW tank 50 px above itself, unless
// something is in the way or 20 enemies are already about.

void bases_type::add(int x, int y, int gap, int power)
{
    if (bases_no >= 5) return;   // port: the original had no bounds check
    base_type &b = data[bases_no];
    b.x = x;
    b.y = y;
    b.gap = gap;
    b.counter = gap;
    b.power = power;
    b.size = 43;
    b.size2 = 21;
    b.power2 = power;
    bases_no++;
}

void bases_type::handle()
{
    for (int b_index = 0; b_index < bases_no; b_index++) {
        base_type &b = data[b_index];
        b.handle();
        if (b.power <= 0) {
            current_scores.scores += b.power2 * 4;
            expls.add(b.x + 5, b.y + 5, 45, 1);
            expls.add(b.x + 38, b.y + 5, 45, 1);
            expls.add(b.x + 38, b.y + 16, 45, 1);
            expls.add(b.x + 5, b.y + 16, 45, 1);
            expls.add(b.x + 21, b.y + 10, 45, 1);
            MixVoice(&VoiceData[4]);
            bases_no--;
            b.clearmap();
            data[b_index] = data[bases_no];
            b_index--;
        }
    }
}

void bases_type::draw()
{
    for (int b_index = 0; b_index < bases_no; b_index++) data[b_index].draw();
}

// The player's tank rams a base: |speed| damage, returns 1.
int bases_type::checkvehiclehit(int x, int y, int ang, int Alpha, double Radius)
{
    for (int b_index = 0; b_index < bases_no; b_index++) {
        base_type &b = data[b_index];
        if (krectinrot(b.x, b.y, b.x + 43, b.y + 21, x, y, ang, Alpha, Radius)) {
            b.power -= abs(ftol(tank.speed));
            return 1;
        }
    }
    return 0;
}

void base_type::handle()
{
    int canadd = 1;
    switch (bullets.checkbasehit(x, y)) {
    case 1: power -= 3; break;
    case 2: power -= 8; break;
    }
    if (--counter == 0) {
        if (enemies.enemies_no < 20) {
            for (int e_index = 0; e_index < enemies.enemies_no; e_index++) {
                enemy_type &e = enemies.data[e_index];
                if (krectinrot(x, y - 70, x + 43, y, e.layers.x, e.layers.y, e.layers.ang, e.Alpha, e.Radius)) canadd = 0;
            }
            if (krectinrot(x, y - 70, x + 43, y, tank.layers.x, tank.layers.y, tank.layers.ang, tank.Alpha, tank.Radius))
                canadd = 0;
            if (canadd) {
                enemy_type &n = enemies.data[enemies.enemies_no];
                if (enemies.enemies_no < 55) {
                    n.path[0].x = x + 21;
                    n.path[0].y = y - 50;
                }
                // (argument evaluation order of the original: right to left)
                enemytag behav = (enemytag)((random(2) + 1) * 3);   // RANDOM or FOLLOW
                unsigned pwr = random(3) + 1;
                double turn = random(4) + 3;
                double spd = random(3) + 1;
                enemies.add(x + 21, y + 10, 384, spd, turn, pwr, 1, behav, 1);
            }
        }
        counter = gap;
    }
    drawmap();
}

void base_type::drawpowerbar()
{
    setcolor(0);
    int layersy = y + size2 - scroll_y + 2;
    int layersx = x - scroll_x;
    int layersx2 = x + 43 - scroll_x;
    int layersx3 = ftol((double)power / power2 * 43.0 + x - scroll_x);
    khorline(layersy - 1, layersx, layersx2);
    khorline(layersy + 2, layersx, layersx2);
    kverline(layersx - 1, layersy, layersy + 1);
    kverline(layersx2 + 1, layersy, layersy + 1);
    setcolor(4);
    khorline(layersy, layersx, layersx2);
    khorline(layersy + 1, layersx, layersx2);
    setcolor(80);
    khorline(layersy, layersx, layersx3);
    khorline(layersy + 1, layersx, layersx3);
}

void base_type::draw()
{
    if (x - scroll_x > -43 && y - scroll_y > -21 && x - scroll_x < 256 && y - scroll_y < 192) {
        if (showpb) drawpowerbar();
        kdrawsprite(x - scroll_x, y - scroll_y, 43, 21, gap_img);
    }
}

// Bases show as black on the minimap.
void base_type::drawmap()
{
    for (int ix = x >> 4; ix <= (x + 43) >> 4; ix++)
        for (int iy = y >> 4; iy <= (y + 21) >> 4; iy++)
            if (onmapcell(ix, iy)) minmap[iy * 96 + ix] = 0;
}

void base_type::clearmap()
{
    for (int ix = x >> 4; ix <= (x + 43) >> 4; ix++)
        for (int iy = y >> 4; iy <= (y + 21) >> 4; iy++)
            if (onmapcell(ix, iy)) minmap[iy * 96 + ix] = getmapcol(map[iy * 96 + ix]);
}
