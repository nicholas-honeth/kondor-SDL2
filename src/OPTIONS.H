// OPTIONS.H - key configuration screen and KONDOR.CFG (reconstructed, OPTIONS.H lines 32-280)
//
// KONDOR.CFG is 13 little-endian 16-bit words: fire, break, up, down, left, right, item,
// inventory left/right, cannon left/right (scancodes, +0x80 = extended), powerbar, sound.

static int *const cfg_order[13] = {&firekey, &breakskey, &arrowup, &arrowdown, &arrowleft, &arrowright,
                                   &itemkey, &keylside, &keyrside, &keyrotcanl, &keyrotcanr, &showpb, &soundfx};

void saveoptions()
{
    FILE *fs = fopen_ci("kondor.cfg", "wb");
    if (fs == NULL) filerr();
    for (int i = 0; i < 13; i++) write_le16(fs, *cfg_order[i]);
    fclose(fs);
}

int loadoptions()
{
    FILE *fl = fopen_ci("kondor.cfg", "rb");
    if (fl == NULL) filerr();
    for (int i = 0; i < 13; i++) *cfg_order[i] = read_le16(fl);
    fclose(fl);
    if (!soundblaster) {
        tempsoundfx = soundfx;
        soundfx = 0;
    }
    return 1;
}

void drawrect(int x, int y, int x2, int y2)
{
    horline(y, x + 2, x2 - 2);
    horline(y2, x + 2, x2 - 2);
    verline(x, y + 2, y2 - 2);
    verline(x2, y + 2, y2 - 2);
    plotpixel(x + 1, y + 1, gcol);
    plotpixel(x2 - 1, y + 1, gcol);
    plotpixel(x2 - 1, y2 - 1, gcol);
    plotpixel(x + 1, y2 - 1, gcol);
}

int textplace(int which)
{
    return which * 13 + 15;
}

void drawtext()
{
    char textline[255];
    static const struct { const char *label; int *key; } rows[11] = {
        {"FIRE: ", &firekey}, {"BREAK: ", &breakskey}, {"MOVE LEFT: ", &arrowleft},
        {"MOVE RIGHT: ", &arrowright}, {"MOVE FORWARD: ", &arrowup}, {"MOVE BACKWARD: ", &arrowdown},
        {"USE INVENTORY: ", &itemkey}, {"INVENTORY LEFT: ", &keylside}, {"INVENTORY RIGHT: ", &keyrside},
        {"ROTATE CANNON LEFT: ", &keyrotcanl}, {"ROTATE CANNON RIGHT: ", &keyrotcanr}};
    for (int i = 0; i < 11; i++) {
        strcpy(textline, rows[i].label);
        strcat(textline, getkeyname(*rows[i].key));
        drawstring(45, textplace(i + 1), textline, font);
    }
    strcpy(textline, "POWERBAR: ");
    strcat(textline, showpb ? "ON" : "OFF");
    drawstring(45, textplace(12), textline, font);
    strcpy(textline, "SOUND: ");
    strcat(textline, soundfx ? "ON" : "OFF");
    drawstring(45, textplace(13), textline, font);
}

void drawwindow(int chosen)
{
    // colour 255 pulses red
    fadered += faderedp;
    if (fadered > 62 || fadered < 2) faderedp = -faderedp;
    setpal(-1, fadered, 0, 0);
    drawsprite(114, 5, 92, 17, optionsc_s);
    for (int index = 0; index <= 12; index++) {
        setcolor(index == chosen ? 255 : 107);
        drawrect(40, index * 13 + 25, 280, index * 13 + 35);
    }
    drawtext();
}

int *ptrkey(int which)
{
    switch (which) {
    default:
    case 0: return &firekey;
    case 1: return &breakskey;
    case 2: return &arrowleft;
    case 3: return &arrowright;
    case 4: return &arrowup;
    case 5: return &arrowdown;
    case 6: return &itemkey;
    case 7: return &keylside;
    case 8: return &keyrside;
    case 9: return &keyrotcanl;
    case 10: return &keyrotcanr;
    }
}

// Assigning a key that is already bound to another action swaps the two bindings.
int checkkey(int *keyn, int which)
{
    if (*keyn == 0xAA) return 0;   // E0 2A: fake shift sent with extended keys
    if (*keyn == firekey) swapint(ptrkey(which), &firekey);
    if (*keyn == breakskey) swapint(ptrkey(which), &breakskey);
    if (*keyn == arrowleft) swapint(ptrkey(which), &arrowleft);
    if (*keyn == arrowright) swapint(ptrkey(which), &arrowright);
    if (*keyn == arrowup) swapint(ptrkey(which), &arrowup);
    if (*keyn == arrowdown) swapint(ptrkey(which), &arrowdown);
    if (*keyn == itemkey) swapint(ptrkey(which), &itemkey);
    if (*keyn == keylside) swapint(ptrkey(which), &keylside);
    if (*keyn == keyrside) swapint(ptrkey(which), &keyrside);
    if (*keyn == keyrotcanl) swapint(ptrkey(which), &keyrotcanl);
    if (*keyn == keyrotcanr) swapint(ptrkey(which), &keyrotcanr);
    return 1;
}

// Note (original behaviour): ESC returns without assigning, and without clearing the key
// table, so a held ESC skips all remaining entries.
void waitforkey(int *key, int which)
{
    int finished = 0;
    int index = 0;
    clearskeys();
    while (!finished) {
        copyscreen(buffer, bkg);
        for (index = 1; index < 256; index++) {
            if (keydown(index)) {
                if (index == 1) {
                    MixVoice(&VoiceData[1]);
                    return;
                }
                if (checkkey(&index, which)) {
                    MixVoice(&VoiceData[1]);
                    finished = 1;
                    break;
                }
            }
        }
        drawwindow(which);
        drawscreen(buffer);
    }
    *key = index;
    clearskeys();
}

void waitforspace(int *val, int win)
{
    int finished = 0;
    delay(35);
    clearskeys();
    while (!finished) {
        copyscreen(buffer, bkg);
        if (keydown(0x39)) {   // space toggles
            *val = *val ? 0 : 1;
            MixVoice(&VoiceData[1]);
            clearskeys();
        }
        if (keydown(1) || keydown(0x1C)) {   // escape / enter
            MixVoice(&VoiceData[1]);
            return;
        }
        drawwindow(win);
        drawscreen(buffer);
    }
}

void gameoptions()
{
    getpal(-1, &or_, &og, &ob);
    blackpal();
    screenis(bkg);
    cleardevice();
    screenis(buffer);
    cleardevice();
    copyscreen(buffer, bkg);
    drawwindow(0);
    drawscreen(buffer);
    fadein((char *)gamepal);
    waitforkey(&firekey, 0);
    waitforkey(&breakskey, 1);
    waitforkey(&arrowleft, 2);
    waitforkey(&arrowright, 3);
    waitforkey(&arrowup, 4);
    waitforkey(&arrowdown, 5);
    waitforkey(&itemkey, 6);
    waitforkey(&keylside, 7);
    waitforkey(&keyrside, 8);
    waitforkey(&keyrotcanl, 9);
    waitforkey(&keyrotcanr, 10);
    waitforspace(&showpb, 11);
    if (soundblaster) waitforspace(&soundfx, 12);
    saveoptions();
    setpal(-1, or_, og, ob);
    fadeout();
}
