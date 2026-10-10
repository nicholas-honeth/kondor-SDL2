// HIGHSCR.H - high score table KONDOR.HSC (reconstructed, HIGHSCR.H lines 12-129)
// KONDOR.HSC: 9 records of {char name[14]; uint16 enemies; uint16 scores;} (18 bytes each).

int getscoreplace()
{
    for (int i = 0; i < 9; i++) {
        if (highscores[i].scores < current_scores.scores) {
            for (int j = 8; j > i; j--) highscores[j] = highscores[j - 1];
            highscores[i] = current_scores;
            highscores[i].name[0] = 0;
            return i;
        }
    }
    return -1;
}

void drawscores()
{
    for (int id = 0; id < 9; id++) {
        if (highscores[id].scores != 0) {
            drawstring(46, id * 14 + 45, highscores[id].name, font);
            drawstring(171, id * 14 + 45, (unsigned long)highscores[id].enemies, font);
            drawstring(234, id * 14 + 45, (unsigned long)highscores[id].scores, font);
        }
    }
}

void savescores()
{
    FILE *hsfs = fopen_ci("kondor.hsc", "wb");
    if (hsfs == NULL) filerr();
    for (int is = 0; is < 9; is++) fwrite(&highscores[is], 18, 1, hsfs);
    fclose(hsfs);
}

void loadscores()
{
    FILE *hsfl = fopen_ci("kondor.hsc", "rb");
    if (hsfl == NULL) filerr();
    for (int il = 0; il < 9; il++) fread_z(&highscores[il], 18, 1, hsfl);   // shipped file is empty
    fclose(hsfl);
}

void showhighscr()
{
    blackpal();
    screenis(bkg);
    if (!load32("kondor6.dat")) filerr();
    screenis(buffer);
    cleardevice();
    copyscreen(buffer, bkg);
    drawscores();
    drawscreen(buffer);
    fadein((char *)gamepal);
    int scrdone = 0;
    while (!scrdone) {
        copyscreen(buffer, bkg);
        if (keydown(1)) scrdone = 1;
        drawscores();
        drawscreen(buffer);
    }
    fadeout();
}

// Wobbling underline cursor under the name being typed.
void drawcursor(int x, int y, int pos)
{
    setcolor(106);
    if (cx > 3.0f) {
        dx = -1.0f * dx;
        cy = 0;
    }
    if (cx < 0) {
        dx = -1.0f * dx;
        cy = 1;
    }
    cx = dx + cx;
    horline(ftol(y + cy), ftol(x + cx + pos * 6), ftol(x + 3 + cx + pos * 6));
}

void enterhsc(int place)
{
    int y = place * 14 + 50;
    int x = 46;
    int strpos = 0;
    blackpal();
    screenis(bkg);
    if (!load32("kondor6.dat")) filerr();
    screenis(buffer);
    cleardevice();
    copyscreen(buffer, bkg);
    drawscores();
    drawcursor(x, y, strpos);
    drawscreen(buffer);
    fadein((char *)gamepal);
    for (;;) {
        copyscreen(buffer, bkg);
        if (keydown(0x1C) && highscores[place].name[0] != 0) break;   // enter
        if (keydown(0x0E)) {                                           // backspace
            if (strpos > 0) {
                strpos--;
                highscores[place].name[strpos] = 0;
            }
            keypress[0x0E] = 0;
        } else if (skeypressed() && strpos < 13) {
            char ch = skey2char(skeypressed());
            if (ch) {
                highscores[place].name[strpos] = ch;
                highscores[place].name[strpos + 1] = 0;
                keypress[skeypressed()] = 0;
                strpos++;
            }
        }
        drawstring(x, y - 5, highscores[place].name, font);
        drawscores();
        drawcursor(x, y, strpos);
        drawscreen(buffer);
    }
    savescores();
    fadeout();
}
