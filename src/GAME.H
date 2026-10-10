// GAME.H - side bar, the per-frame update/draw and the game loop (reconstructed, GAME.H lines 23-109)

void load_side_bar()
{
    screenis(bkg);
    if (!load32("kondor5.dat")) filerr();
    setcolor(0);
    drawslotrect(262, 70, 284, 90);
    drawslotrect(291, 70, 313, 90);
    drawslotrect(262, 100, 284, 120);
    drawslotrect(291, 100, 313, 120);
    drawstringreg((unsigned)(256 - strlen(MSG) * 6) >> 1, 194, MSG, regfont);   // level message
    setcolor(45);
    bar(263, 131, tank.power / 204 + 263, 134);
    screenis(buffer);
    for (int i = 0; i < 4; i++)
        if (slots.data[i].amount != 0) slots.drawsprt(i);
    drawslots(80);
}

// One frame: update everything, restore the background, draw, wait for the retrace, show.
void drawNhandle()
{
    tank.handle();
    check4scroll();
    handlecounter();
    expls.handle();
    copyscreen(buffer, bkg);
    bullets.handle();
    air_enemies.handle();
    bases.handle();
    enemies.handle();
    objects.draw();
    bullets.draw();
    enemies.draw();
    bases.draw();
    tank.draw();
    expls.draw();
    air_enemies.draw();
    drawminmap();
    slots.draw();
    setcolor(86);
    drawstringreg(284, 145, (unsigned long)tank.lives, regfont);
    waitretrace();
    drawscreen(buffer);
}

void start_game()
{
    clearskeys();
    set_game_pal();
    load_side_bar();
    drawscroll();
    gameloop = 0;
    while (!gameloop) {
        if (keydown(arrowdown)) tank.speed = tank.speed - tank.accel;
        if (keydown(arrowup)) tank.speed = tank.accel + tank.speed;
        if (!keydown(arrowdown) && !keydown(arrowup)) tank.speed = tank.speed - 0.03 * tank.speed;
        if (tank.speed > tank.maxspeed) tank.speed = tank.maxspeed;
        if (-tank.maxspeed > tank.speed) tank.speed = -tank.maxspeed;
        if (keydown(keylside)) {
            drawslots(0);
            current_slot--;
            if (current_slot == -1) current_slot = 3;
            drawslots(80);
            keypress[keylside] = 0;
        }
        if (keydown(keyrside)) {
            drawslots(0);
            current_slot++;
            if (current_slot == 4) current_slot = 0;
            drawslots(80);
            keypress[keyrside] = 0;
        }
        if (keydown(keyrotcanr)) tank.turn_cannon(ftol(tank.cannon_turn_speed));
        if (keydown(keyrotcanl)) tank.turn_cannon(ftol(-tank.cannon_turn_speed));
        if (keydown(breakskey)) tank.speed = 0.9 * tank.speed;
        if (keydown(firekey))
            shoot();
        else
            canshoot = true;
        if (!(-tank.halfwidth_3 < tank.height)) tank.height = -tank.halfwidth_3;
        if (keydown(1)) gameloop = 1;   // escape: back to the menu (resumable)
        if (keydown(itemkey))
            usebonus();
        else
            canuseitem = true;
        drawNhandle();
    }
}
