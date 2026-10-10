// HELP.H - scrolling help text between two random "cave walls" (reconstructed, HELP.H lines 36-84)

void inithelpscroll()
{
    int tempx = 20;
    for (int index = 0; index < 900; index++) {
        switch (random(2)) {
        case 0: if (tempx < 40) tempx++; break;
        case 1: if (tempx > 10) tempx--; break;
        }
        terrainy[index] = tempx;
    }
}

void drawhelpscroll()
{
    setcolor(107);
    for (int id = hhelpscroll_y; id < hhelpscroll_y + 200; id++) {
        horline(id - hhelpscroll_y, 0, terrainy[id]);
        plotpixel(terrainy[id] + 1, id - hhelpscroll_y, 106);
        horline(id - hhelpscroll_y, 319 - terrainy[id] + 1, 319);
        plotpixel(319 - terrainy[id], id - hhelpscroll_y, 106);
    }
    int starty = hhelpscroll_y / 8;
    for (int id = starty; id < starty + 26; id++)
        drawstring(50, id * 8 - hhelpscroll_y, helplines[id], font);
}

void gamehelp()
{
    screenis(buffer);
    cleardevice();
    blackpal();
    hhelpscroll_y = 6;
    drawhelpscroll();
    drawscreen(buffer);
    fadein((char *)gamepal);
    done = 0;
    while (!done) {
        cleardevice();
        if (keydown(0xC8) && hhelpscroll_y > 6) hhelpscroll_y--;     // up arrow
        if (keydown(0xD0) && hhelpscroll_y < 700) hhelpscroll_y++;   // down arrow
        if (keydown(1)) done = 1;                                    // escape
        drawhelpscroll();
        waitretrace();
        drawscreen(buffer);
    }
    fadeout();
}
