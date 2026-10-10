// MISC.H - level progression and the once-a-second housekeeping (reconstructed, MISC.H lines 2-87)

void gotonextlvl()
{
    if (tank.lives > 99) tank.lives = 99;
    if (current_level < 12) {
        screenis(video_buffer);
        drawsprite(61, 55, 198, 90, missacco_s);   // "mission accomplished"
        delay(3500);
        screenis(buffer);
        fadeout();
        current_level++;
        loadmap();
        load_side_bar();
        drawNhandle();
        fadein((char *)gamepal);
    } else {
        gameoverpage();
        resumegame = false;
        optionmenu = start;
        gameloop = 1;
        clearskeys();
        screenis(video_buffer);
        cleardevice();
        set_game_pal();
        screenis(buffer);
        int hscresult = getscoreplace();
        if (hscresult >= 0) enterhsc(hscresult);
    }
}

// Runs every frame; every 26th frame animates water, blinks the repair icon, times the
// nitro, checks for level completion and launches aircraft. Repair kits heal every 5th frame.
void handlecounter()
{
    if (counter > 0) {
        counter--;
    } else {
        movewater();
        if (scrollmove == 0) drawscroll();
        counter = 25;
        powericon = !powericon;
        if (speedactivated && speedcounter-- == 0) {
            tank.maxspeed = tank.maxspeed / 2.0;
            speedactivated = 0;
        }
        if (enemies.enemies_no == 0) gotonextlvl();
        if (air_enemies.enemies_no < planesperlevel) {
            // aircraft enter from a random map corner (argument order of the original)
            unsigned pwr = random(1) + 1;
            double turn = random(3) + 1;
            double spd = random(2) + 1;
            double y = random(2) * 1636 - 50;
            double x = random(2) * 1636 - 50;
            air_enemies.add(x, y, 0, spd, turn, pwr, 0, PIPER);
        }
    }
    if (poweractivated) {
        int c_s = slots.getslotn(2);
        if (c_s < 0) {   // port: the original indexed slot -1 here
            poweractivated = 0;
        } else if (counter % 5 == 0) {
            if (slots.data[c_s].amount != 0) {
                if (tank.power < 10000) {
                    slots.data[c_s].amount--;
                    tank.power += 100;
                    if (tank.power > 10000) tank.power = 10000;
                    screenis(bkg);
                    setcolor(45);
                    bar(263, 131, tank.power / 204 + 263, 134);
                    screenis(buffer);
                } else {
                    poweractivated = 0;
                }
            } else {
                poweractivated = 0;
                slots.data[c_s].amount = 0;
                slots.data[c_s].type = -1;
                slots.erase(c_s);
            }
        }
    }
}
