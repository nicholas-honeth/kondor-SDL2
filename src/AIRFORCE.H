// AIRFORCE.H - aircraft: PIPER (hawk) and MIG (jet fighter) (reconstructed, AIRFORCE.H lines 2-219)
//
// PIPER circles the player: it heads for a random point 150-450 px from the tank, then
// back towards the tank, and fires between 50 and 95 px. MIG flies its waypoint path; near
// the player it weaves (mode = +-128 angle offset, flipped every 32-63 frames) and fires.
// Aircraft cast a shadow: drawsink() with half-size margins darkens the background +5,+5.

static int air_turn(int &ang, int target, double turn_speed)
{
    int angdist = abs(target - ang);
    if (angdist < turn_speed) {
        ang = target & 511;
    } else {
        int x = ftol(target > ang ? turn_speed : -turn_speed);
        if (angdist < 256) x = -x;
        ang -= x;
    }
    return angdist;
}

void air_enemy_type::targetondist()
{
    if (mode2 == 0) {
        tempx = path[cur_point].x + 400;
        tempy = path[cur_point].y + 400;
        mode2 = 1;
    }
    sprt.ang &= 511;
    temp2 = get_ang(ftol(sprt.x), ftol(sprt.y), ftol(tank.layers.x), ftol(tank.layers.y));
    air_turn(sprt.ang, temp2, turn_speed);
    if (temp-- == 0) {
        mode = -mode;
        temp = random(32) + 32;
    }
    sprt.x = costbl[(sprt.ang + mode) & 511] * 0.5 + sprt.x;
    sprt.y = sintbl[(sprt.ang + mode) & 511] * 0.5 + sprt.y;
    if (random(100) < 2) {
        double r = halfwidth + 5;
        bullets.add(r * costbl[sprt.ang & 511] + sprt.x, r * sintbl[sprt.ang & 511] + sprt.y, sprt.ang, speed + 5.0,
                    AIRENEMY, 0);
    }
}

void air_enemy_type::move()
{
    if (behavior == EMPTY) return;

    if (behavior == PIPER) {
        double range = dist(sprt.x, sprt.y, tank.layers.x, tank.layers.y);
        if (mode == 0) {
            temp2 = get_ang(ftol(sprt.x), ftol(sprt.y), ftol(tank.layers.x), ftol(tank.layers.y)) & 511;
            if (range < 2500.0) {   // passed the player: pick a point to swing out to
                mode = 1;
                temp = random(512);
                int r1 = random(300) + 150;
                tempx = ftol(r1 * costbl[temp & 511] + tank.layers.x);
                int r2 = random(300) + 150;
                tempy = ftol(r2 * sintbl[temp & 511] + tank.layers.y);
            }
        }
        if (mode == 1) {
            temp2 = get_ang(ftol(sprt.x), ftol(sprt.y), tempx, tempy) & 511;
            if (dist(sprt.x, sprt.y, tempx, tempy) < 2500.0) mode = 0;
        }
        sprt.ang &= 511;
        temp2 &= 511;
        air_turn(sprt.ang, temp2, turn_speed);
        sprt.x = costbl[sprt.ang & 511] * speed + sprt.x;
        sprt.y = sintbl[sprt.ang & 511] * speed + sprt.y;
        if (mode == 0 && range < 9025.0 && range > 2500.0 && random(100) < 5) {
            double r = halfwidth + 5;
            bullets.add(r * costbl[sprt.ang & 511] + sprt.x, r * sintbl[sprt.ang & 511] + sprt.y, sprt.ang,
                        speed + 5.0, AIRENEMY, 0);
        }
    }

    if (behavior == MIG) {
        if (!(dist(sprt.x, sprt.y, tank.layers.x, tank.layers.y) > 4900.0)) {
            targetondist();
        } else {
            int tx, ty;
            if (mode2) {
                tx = tempx;
                ty = tempy;
            } else {
                tx = path[cur_point].x;
                ty = path[cur_point].y;
            }
            if (inrect(ftol(sprt.x), ftol(sprt.y), ftol(tx - speed2), ftol(ty - speed2), ftol(tx + speed2),
                       ftol(ty + speed2))) {
                sprt.x = tx;
                sprt.y = ty;
                if (mode2) {
                    mode2 = 0;
                } else {
                    cur_point++;
                    if (cur_point >= path_len) cur_point = 0;
                }
            }
            temp2 = get_ang(ftol(sprt.x), ftol(sprt.y), tx, ty);
            sprt.ang &= 511;
            int angdist = abs(temp2 - sprt.ang);
            if (angdist < turn_speed) {
                sprt.ang = temp2;
            } else {
                int x = ftol(temp2 > sprt.ang ? turn_speed : -turn_speed);
                if (angdist < 256) x = -x;
                sprt.ang -= x;
            }
            sprt.x = costbl[sprt.ang & 511] * speed + sprt.x;
            sprt.y = sintbl[sprt.ang & 511] * speed + sprt.y;
        }
    }
}

void air_enemy_type::handle()
{
    if (onmap(sprt.x, sprt.y)) clearminmap(ftol(sprt.x), ftol(sprt.y));
    move();
    if (onmap(sprt.x, sprt.y)) drawonminmap(ftol(sprt.x), ftol(sprt.y), 0x48, 0);
    int res = bullets.checkvehiclehit(ftol(sprt.x), ftol(sprt.y), sprt.ang, Alpha, Radius, ENEMY);
    switch (res) {
    case 1: power -= 3; break;
    case 2: power -= 8; break;
    }
}

void air_enemy_type::draw()
{
    if (-size < sprt.x - scroll_x && size + 256 > sprt.x - scroll_x && -size < sprt.y - scroll_y &&
        size + 192 > sprt.y - scroll_y) {
        drawsink(ftol(sprt.x - scroll_x + 5.0), ftol(sprt.y - scroll_y + 5.0), ftol(sprt.cx), ftol(sprt.cy), sprt.ang,
                 sprt.w, sprt.h, sprt.data, halfwidth, halfheight, halfwidth, halfheight);
        sprt.draw(-scroll_x, -scroll_y, 0.0);
    }
}

int air_enemy_type::dead()
{
    return power <= 0 ? 1 : 0;
}

void air_enemies_type::handle()
{
    for (int e_index = 0; e_index < enemies_no; e_index++) {
        data[e_index].handle();
        if (data[e_index].dead()) {
            expls.add(ftol(data[e_index].sprt.x), ftol(data[e_index].sprt.y), 45, 1);
            MixVoice(&VoiceData[4]);
            enemies_no--;
            clearminmap(ftol(data[e_index].sprt.x), ftol(data[e_index].sprt.y));
            data[e_index] = data[enemies_no];
            e_index--;
        }
    }
}

void air_enemies_type::draw()
{
    for (int e_index = 0; e_index < enemies_no; e_index++) data[e_index].draw();
}

void air_enemies_type::add(double x, double y, int ang, double speed, double turn_speed, unsigned power, int path_len,
                           enemytag behav)
{
    if (enemies_no >= 30) return;
    air_enemy_type &e = data[enemies_no];
    sprite_type &src = enemy_sprts[behav].sprts[0];
    e.sprt.set(x, y, src.cx, src.cy, ang, src.w, src.h, src.data, true);
    e.cur_point = 0;
    e.path_len = path_len;
    e.speed = speed;
    e.turn_speed = turn_speed;
    e.power = power;
    e.speed2 = speed * 2.0;
    e.halfwidth = e.sprt.w >> 1;
    e.halfheight = e.sprt.h >> 1;
    e.size = e.sprt.w > e.sprt.h ? e.sprt.w : e.sprt.h;
    e.behavior = behav;
    e.Alpha = ftol(atan2((double)-(e.sprt.h >> 1), (double)(e.sprt.w >> 1)) / M_PI * 256.0);
    e.Radius = sqrt((e.sprt.w * e.sprt.w + e.sprt.h * e.sprt.h) * 0.25);
    if (behav == MIG) {
        e.sprt.ang = get_ang(ftol(x), ftol(y), e.path[1].x, e.path[1].y);
        e.cur_point = 1;
        e.mode = 128;
        e.mode2 = 0;
        e.temp = random(32) + 32;
    } else {
        e.mode = 0;
    }
    enemies_no++;
}

void air_enemies_type::close()
{
    enemies_no = 0;
}

air_enemies_type::air_enemies_type()
{
    enemies_no = 0;
}
