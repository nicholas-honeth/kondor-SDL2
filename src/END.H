// END.H - credits after the last level (reconstructed, END.H lines 8-31)

void gameoverpage()
{
    int edone = 0;
    double y = 200.0;
    screenis(video_buffer);
    clearscreen();
    screenis(bkg);
    if (!loadpal("kondor20.dat")) filerr();
    if (!load32("kondor19.dat")) filerr();
    screenis(buffer);
    setcolor(0);
    static const char *lines[4] = {"THANK YOU FOR PLAYING", "KONDOR!", "QSOFT 1997-1999(C)",
                                   "BY GIL DABAH AND OREN BECKER"};
    static const double dy[4] = {0, 8, 30, 38};
    while (!edone) {
        copyscreen(buffer, bkg);
        for (int i = 0; i < 4; i++)   // scrolling up, centred
            drawstringreg(ftol(160.0 - strlen(lines[i]) * 2.5), ftol(y + dy[i]), lines[i], regfont);
        if (y > -43.0) y = y - 0.5;
        if (keydown(1)) edone = 1;
        waitretrace();
        drawscreen(buffer);
    }
}
