// MENU.H - main menu (reconstructed, MENU.H lines 4-145)

void drawmenu()
{
    resumeyplace = resumegame ? 25 : 0;   // "resume" pushes the items below it down
    drawsprite(127, 50, 67, 17, start_s);
    if (resumegame) drawsprite(118, 75, 84, 17, resume_s);
    drawsprite(114, resumeyplace + 75, 92, 17, options_s);
    drawsprite(121, resumeyplace + 100, 78, 17, scores_s);
    drawsprite(135, resumeyplace + 125, 50, 16, help_s);
    drawsprite(134, resumeyplace + 150, 52, 16, exit_s);
    switch (optionmenu) {   // highlighted item
    case start: drawsprite(127, 50, 67, 17, startc_s); break;
    case resume: drawsprite(118, 75, 84, 17, resumec_s); break;
    case option: drawsprite(114, resumeyplace + 75, 92, 17, optionsc_s); break;
    case hghscores: drawsprite(121, resumeyplace + 100, 78, 17, scoresc_s); break;
    case help: drawsprite(135, resumeyplace + 125, 50, 16, helpc_s); break;
    case exitgame: drawsprite(134, resumeyplace + 150, 52, 16, exitc_s); break;
    }
}

void clearscreen()
{
    screenis(video_buffer);
    cleardevice();
    screenis(buffer);
    cleardevice();
    screenis(bkg);
    if (!load32("kondor4.dat")) filerr();
    setcolor(86);
    drawstringreg(0, 194, MENU_MSG, regfont);
    set_game_pal();
    screenis(buffer);
}

void menu()
{
    int quitmenu = 0;
    optionmenu = start;
    screenis(buffer);
    while (!quitmenu) {
        copyscreen(buffer, bkg);
        if (keydown(0xC8)) {   // up
            switch (optionmenu) {
            case start: optionmenu = exitgame; break;
            case resume: optionmenu = start; break;
            case option: optionmenu = resumegame ? resume : start; break;
            case hghscores: optionmenu = option; break;
            case help: optionmenu = hghscores; break;
            case exitgame: optionmenu = help; break;
            }
            MixVoice(&VoiceData[0]);
            clearskeys();
        }
        if (keydown(0xD0)) {   // down
            switch (optionmenu) {
            case start: optionmenu = resumegame ? resume : option; break;
            case resume: optionmenu = option; break;
            case option: optionmenu = hghscores; break;
            case hghscores: optionmenu = help; break;
            case help: optionmenu = exitgame; break;
            case exitgame: optionmenu = start; break;
            }
            MixVoice(&VoiceData[0]);
            clearskeys();
        }
        if (keydown(0x1C)) {   // enter
            switch (optionmenu) {
            case start: {
                if (resumegame) {   // abandoning a running game still records its score
                    int hscresult = getscoreplace();
                    if (hscresult >= 0) enterhsc(hscresult);
                    clearscreen();
                }
                current_level = 1;
                strcpy(current_scores.name, "");
                current_scores.scores = 0;
                current_scores.enemies = 0;
                tank.lives = 5;
                resumegame = true;
                loadmap();
                start_game();
                clearscreen();
                clearskeys();
                break;
            }
            case resume: start_game(); clearscreen(); clearskeys(); break;
            case option: gameoptions(); clearscreen(); clearskeys(); break;
            case hghscores: showhighscr(); clearscreen(); clearskeys(); break;
            case help: gamehelp(); clearscreen(); clearskeys(); break;
            case exitgame: quitmenu = 1; break;
            }
        }
        drawmenu();
        drawscreen(buffer);
    }
}
