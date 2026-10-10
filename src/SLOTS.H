// SLOTS.H - the 2x2 inventory on the side bar (reconstructed, SLOTS.H lines 21-139)
// Slot layout on screen: 0 = top left, 1 = top right, 2 = bottom left, 3 = bottom right.
// The side bar lives in _bkg, so slot graphics are drawn there.

static const int slot_x[4] = {265, 294, 265, 294};
static const int slot_y[4] = {72, 72, 102, 102};

void drawslotrect(int x, int y, int x2, int y2)
{
    horline(y, x + 2, x2 - 2);
    horline(y2, x + 2, x2 - 2);
    verline(x, y + 2, y2 - 2);
    verline(x2, y + 2, y2 - 2);
}

// Frame around the current slot (colour 0 erases it, 80 highlights it).
void drawslots(int color)
{
    screenis(bkg);
    setcolor(color);
    switch (current_slot) {
    case 0: drawslotrect(262, 70, 284, 90); break;
    case 1: drawslotrect(291, 70, 313, 90); break;
    case 2: drawslotrect(262, 100, 284, 120); break;
    case 3: drawslotrect(291, 100, 313, 120); break;
    }
    screenis(buffer);
}

// (Original: returns the type of an *empty* slot, -1 otherwise.)
int slot_type::slotempty(int n)
{
    return data[n].amount == 0 ? data[n].type : -1;
}

int slot_type::getslotn(int type)
{
    for (int i = 0; i < 4; i++)
        if (data[i].type == type) return i;
    return -1;
}

void slot_type::add(int type)
{
    drawslots(0);
    for (int i = 0; i < 4; i++) {
        if (data[i].type == type) {
            data[i].amount += random(7) + 4;
            if (data[i].amount > 99) data[i].amount = 99;
            current_slot = i;
            drawslots(80);
            return;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (data[i].type == -1) {
            data[i].type = type;
            data[i].amount = random(7) + 4;
            if (data[i].amount > 99) data[i].amount = 99;
            drawsprt(i);
            current_slot = i;
            drawslots(80);
            return;
        }
    }
}

void slot_type::drawsprt(int n)
{
    screenis(bkg);
    if (n >= 0 && n <= 3 && data[n].type >= 0 && data[n].type < 5) {
        bonus_type &b = bonussprts[data[n].type];
        drawsprite(slot_x[n] + ((18 - b.w) >> 1), slot_y[n] + ((18 - b.h) >> 1), b.w, b.h, b.sprt);
    }
    screenis(buffer);
}

// Item counts, drawn every frame into the frame buffer.
void slot_type::draw()
{
    setcolor(86);
    if (data[0].amount) drawstringreg(264, 72, (unsigned long)(long)data[0].amount, regfont);
    if (data[1].amount) drawstringreg(293, 72, (unsigned long)(long)data[1].amount, regfont);
    if (data[2].amount) drawstringreg(264, 102, (unsigned long)(long)data[2].amount, regfont);
    if (data[3].amount) drawstringreg(293, 102, (unsigned long)(long)data[3].amount, regfont);
}

void slot_type::erase(int n)
{
    screenis(bkg);
    setcolor(0);
    if (n >= 0 && n <= 3) bar(slot_x[n], slot_y[n], slot_x[n] + 16, slot_y[n] + 16);
    screenis(buffer);
    for (int i = 0; i < 4; i++) {
        if (data[i].amount > 0) {
            drawslots(0);
            current_slot = i;
            drawslots(80);
            return;
        }
    }
}

int slot_type::use()
{
    return data[current_slot].amount > 0 ? data[current_slot].type : -1;
}
