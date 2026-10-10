// PTANK.H - the player's tank (reconstructed, PTANK.H lines 1-207)
//
// height < 0 means the tank is sinking into / driving under water (heightspeed is the
// vertical speed). layers.sprts[0] is the hull, sprts[1] the cannon (angle relative to hull).

void tank_type::draw()
{
    if (height <= -1.0f) {
        // partly submerged: only the part above the water line is drawn normally
        int s = ftol(-height);
        drawsink(ftol(layers.x - scroll_x), ftol(layers.y - scroll_y), ftol(layers.sprts[0].cx),
                 ftol(layers.sprts[0].cy), layers.ang, layers.sprts[0].w, layers.sprts[0].h, layers.sprts[0].data, s, s,
                 s, s);
        drawsink(ftol(tank.layers.x - scroll_x), ftol(tank.layers.y - scroll_y), ftol(tank.layers.sprts[1].cx),
                 ftol(tank.layers.sprts[1].cy), tank.layers.ang + tank.layers.sprts[1].ang, tank.layers.sprts[1].w,
                 tank.layers.sprts[1].h, tank.layers.sprts[1].data, 0, ftol(-(tank.height + 2.0f)),
                 ftol(-(tank.height + 9.0f)), ftol(-(tank.height + 9.0f)));
    } else {
        layers.draw();
    }
    if (shieldhits) {   // shield grows to radius 20
        setcolor(4);
        circle(ftol(layers.x - scroll_x), ftol(layers.y - scroll_y), ftol(shieldrad));
        if (shieldrad < 20.0f) shieldrad = shieldrad + 0.5f;
    } else if (0 < shieldrad) {   // and shrinks away when used up
        setcolor(4);
        circle(ftol(layers.x - scroll_x), ftol(layers.y - scroll_y), ftol(shieldrad));
        shieldrad = shieldrad - 0.5f;
    }
    if (poweractivated && powericon)
        kdrawsprite(ftol(layers.x - scroll_x - 8.0), ftol(layers.y - scroll_y - 8.0), 16, 15, skey_img);
}

// Damage (absorbed by an active shield) and the power bar on the side bar (power 0..10200).
void tank_type::hit(unsigned damage)
{
    if (power < damage)
        power = 0;
    else if (shieldrad > Radius && shieldhits != 0)
        shieldhits--;
    else
        power -= damage;
    screenis(bkg);
    setcolor(0);
    bar(power / 204 + 263, 131, 312, 134);
    setcolor(45);
    bar(263, 131, power / 204 + 263, 134);
    screenis(buffer);
}

// power 0 (or wrapped below 0) costs a life; the last one ends the game.
void tank_type::isdead()
{
    if (power != 0 && power <= 10000) return;
    lives--;
    if (lives == 0) {
        gameloop = 1;
        resumegame = false;
        optionmenu = start;
        screenis(video_buffer);
        drawsprite(52, 80, 216, 40, gameover_s);
        delay(3500);
        screenis(buffer);
        fadeout();
        clearscreen();
        clearskeys();
        int hscresult = getscoreplace();
        if (hscresult >= 0) enterhsc(hscresult);
    } else {
        fadeout();
        loadmap();
        load_side_bar();
        drawNhandle();
        fadein((char *)gamepal);
    }
}

// 1 if every map cell under the tank is water.
int checksink()
{
    int x = ftol(tank.layers.x) >> 4;
    int y = ftol(tank.layers.y) >> 4;
    for (int ix = x - 1; ix <= x + 1; ix++)
        for (int iy = y - 1; iy <= y + 1; iy++)
            if (krectinrot(ix << 4, iy << 4, (ix + 1) << 4, (iy + 1) << 4, tank.layers.x, tank.layers.y, tank.layers.ang,
                           tank.Alpha, tank.Radius) &&
                !waterxy(ix, iy))
                return 0;
    return 1;
}

void tank_type::handle()
{
    if (!(0 < heightspeed) && checksink()) {   // over water: sink and slow down
        if (height == -0.25 && !isplaying(&VoiceData[5])) MixVoice(&VoiceData[5]);
        heightspeed = -0.25f;
        speed = speed - speed * 0.18;
    }
    if (!(-(layers.sprts[0].w * 4) < height) && 0 > heightspeed)
        heightspeed = 0;
    else if (!(0 > height) && !(0 >= heightspeed))
        heightspeed = 0;
    height = heightspeed + height;
    clearminmap(ftol(layers.x), ftol(layers.y));
    int ang = layers.ang;
    double x = layers.x, y = layers.y;
    if (keydown(arrowright)) tank.turn(ftol(turn_speed));
    if (keydown(arrowleft)) tank.turn(ftol(-turn_speed));
    move();
    if (enemies.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius, -1)) {
        // bounce off the enemy
        layers.x = x;
        layers.y = y;
        layers.ang = ang;
        layers.x = layers.x - costbl[layers.ang & 511] * speed;
        layers.y = layers.y - sintbl[layers.ang & 511] * speed;
        speed = speed * -0.4;
        if (speed <= -0.2 || !(speed < 0.2)) {
            hit(abs(ftol(speed * 5.0)));
            expls.add(cur_coll.x, cur_coll.y, 45, 0);
        }
    }
    if (shieldrad > Radius) {
        if (bullets.checkcirclehit(ftol(layers.x), ftol(layers.y), (double)shieldrad * shieldrad, PLAYER)) {
            hit(200);
            isdead();
        }
    } else if (bullets.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius, PLAYER)) {
        hit(200);
        isdead();
    }
    int type = objects.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius);
    if (type != -1) {   // picked up a bonus
        MixVoice(&VoiceData[9]);
        if (type == 3)
            lives += random(3) + 1;
        else
            slots.add(type);
    }
    if (bases.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius)) {
        layers.x = x;
        layers.y = y;
        layers.ang = ang;
        layers.x = layers.x - costbl[layers.ang & 511] * speed;
        layers.y = layers.y - sintbl[layers.ang & 511] * speed;
        speed = speed * -0.4;
        hit(abs(ftol(speed * 2.0)));
    }
    drawonminmap(ftol(layers.x), ftol(layers.y), 11, 11);
}

// Under water only water is passable; on land everything but blocking tiles.
int tank_type::moveable(int x, int y)
{
    if (0 > height) return waterxy(x, y);
    return !blockingxy(x, y);
}

// Collision with impassable map cells around the tank. Bumping into the shore while
// submerged makes the tank surface again.
int tank_type::checkhit()
{
    int x = ftol(layers.x) >> 4;
    int y = ftol(layers.y) >> 4;
    for (int ix = x - 1; ix <= x + 1; ix++) {
        for (int iy = y - 1; iy <= y + 1; iy++) {
            if (moveable(ix, iy)) continue;
            int r = krectinrot(ix << 4, iy << 4, (ix + 1) << 4, (iy + 1) << 4, layers.x, layers.y, layers.ang, Alpha,
                               Radius);
            if (r) {
                if (0 > height && maptile(ix, iy) < 0x65) heightspeed = 0.5f;
                return r;
            }
        }
    }
    return 0;
}

void tank_type::turn(int direction)
{
    layers.ang += direction;
    if (checkhit()) layers.ang -= direction;
    tank.layers.ang &= 511;
}

void tank_type::move()
{
    float s = speed;
    tank.layers.ang &= 511;
    layers.x = s * costbl[layers.ang & 511] + layers.x;
    layers.y = s * sintbl[layers.ang & 511] + layers.y;
    if (checkhit()) {
        layers.x = layers.x - s * costbl[layers.ang & 511];
        layers.y = layers.y - s * sintbl[layers.ang & 511];
        if (!(0 > height)) speed = speed * -0.04;
    }
}

// (Original: masks the hull angle, not the cannon's.)
void tank_type::turn_cannon(int direction)
{
    layers.sprts[1].ang += direction;
    tank.layers.ang &= 511;
}
