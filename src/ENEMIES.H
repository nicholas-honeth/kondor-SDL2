// ENEMIES.H - ground enemies: turrets and tanks with several behaviours
// (reconstructed, ENEMIES.H lines 2-487)
//
// Behaviours (enemytag): TURRET rotates its cannon towards the player within 135 px;
// CIRCLE drives in arcs; FOLLOW path-finds to the player; RANDOM wanders to random map
// tiles (and hunts the player within 100 px); PATH_FIND patrols the waypoints from the map
// file. Within 95 px all moving enemies stop and aim (shootondist).

// Squared distance; while the player is sunk (height < 0) everything is "far away".
double dist(double x1, double y1, double x2, double y2)
{
    if (0 > tank.height) return 100000.0;
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

// Angle from (x1,y1) to (x2,y2) in 512ths of a circle; 0x900 if the points coincide.
int get_ang(int x1, int y1, int x2, int y2)
{
    if (x1 == x2 && y1 == y2) return 0x900;
    return ftol(atan2((double)(y2 - y1), (double)(x2 - x1)) / M_PI * 256.0) & 511;
}

// Turn the hull towards the player; when aimed, fire with a 2% chance per frame.
void enemy_type::shootondist()
{
    int x = ftol(turn_speed);
    temp = get_ang(ftol(layers.x), ftol(layers.y), ftol(tank.layers.x), ftol(tank.layers.y));
    layers.ang &= 511;
    int angdist = abs(layers.ang - temp);
    x = ftol(temp > layers.ang ? turn_speed : -turn_speed);
    if (angdist < 256) x = -x;
    if (angdist < turn_speed) {
        layers.ang = temp & 511;
        if (random(100) < 2) {
            double r = (layers.sprts[0].w >> 1) + 5;
            bullets.add(r * costbl[layers.ang & 511] + layers.x, r * sintbl[layers.ang & 511] + layers.y,
                        layers.ang, 5.0, ENEMY, 0);
        }
    } else {
        layers.ang -= x;
    }
}

// Re-plan around the vehicle 'res' we bumped into: a random tile near us (+-5 tiles).
void enemy_type::replan()
{
    notmyfault = 0;
    cur_point = 0;
    int ty = random(10) + (ftol(layers.y) >> 4) - 5;   // evaluated first, like the original
    int tx = random(10) + (ftol(layers.x) >> 4) - 5;
    findtheway(ftol(layers.x) >> 4, ftol(layers.y) >> 4, tx, ty, path, &path_len);
    temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[0].x, path[0].y) & 511;
}

// Step forward; if that hits another vehicle, step back and either wait (when the other
// one is a wandering/following enemy far from the player) or plan a detour.
void enemy_type::smartmove(int me)
{
    if (moveable) {
        layers.ang = temp2 & 511;
        layers.x = costbl[layers.ang & 511] * speed + layers.x;
        layers.y = sintbl[layers.ang & 511] * speed + layers.y;
    }
    int res = enemies.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius, me);
    if (res == 0 || enemies.data[res - 1].behavior == TURRET) {
        moveable = 1;
        notmyfault = 0;
        return;
    }
    enemy_type &other = enemies.data[res - 1];
    layers.x = layers.x - costbl[layers.ang & 511] * speed;
    layers.y = layers.y - sintbl[layers.ang & 511] * speed;
    if (((other.behavior == FOLLOW || other.behavior == RANDOM) && notmyfault == 0) || other.behavior == CIRCLE) {
        if (dist(other.layers.x, other.layers.y, tank.layers.x, tank.layers.y) > 9025.0) {
            moveable = 0;
            other.notmyfault = 1;
        } else {
            replan();
        }
    } else {
        replan();
    }
}

// Turn by turn_speed towards temp2 (the short way round).
int enemy_type::turnstep()
{
    int x = 0;
    if (layers.ang > temp2) x = ftol(turn_speed);
    if (layers.ang < temp2) x = ftol(-turn_speed);
    if (abs(layers.ang - temp2) < 256) x = -x;
    return x;
}

// Arrived at the current waypoint (within doublespeed)?
int enemy_type::atwaypoint()
{
    return inrect(ftol(layers.x), ftol(layers.y), path[cur_point].x - doublespeed, path[cur_point].y - doublespeed,
                  path[cur_point].x + doublespeed, path[cur_point].y + doublespeed);
}

// Snap to the waypoint (or, after mode 1, to the current tile centre) and aim at the next.
void enemy_type::nextwaypoint(int mask)
{
    if (mode == 1) {
        mode = 0;
        layers.x = (uint16_t)((ftol(layers.x) & 0xFFF0) + 8);
        layers.y = (uint16_t)((ftol(layers.y) & 0xFFF0) + 8);
    } else {
        layers.x = path[cur_point].x;
        layers.y = path[cur_point].y;
    }
    cur_point++;
    if (cur_point < path_len) {
        temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[cur_point].x, path[cur_point].y);
        if (mask) temp2 &= 511;
    }
}

// Turn towards temp2, or move along it once aligned.
void enemy_type::steer(int me)
{
    int angdist = abs(layers.ang - temp2);
    if (angdist > turn_speed) {
        layers.ang += turnstep();
        layers.ang &= 511;
    } else {
        smartmove(me);
    }
}

void enemy_type::move(int me)
{
    if (behavior == EMPTY) return;

    if (behavior == TURRET) {
        if (dist(layers.x, layers.y, tank.layers.x, tank.layers.y) <= 18225.0) {   // 135 px
            sprite_type &cannon = layers.sprts[1];
            temp = get_ang(ftol(layers.x), ftol(layers.y), ftol(tank.layers.x), ftol(tank.layers.y));
            cannon.ang &= 511;
            int angdist = abs(cannon.ang - temp);
            int x = ftol(temp > cannon.ang ? turn_speed : -turn_speed);
            if (angdist < 256) x = -x;
            if (angdist < turn_speed) {
                cannon.ang = temp & 511;
                if (random(100) < 2) {
                    double r = (layers.sprts[0].w >> 1) + 5;
                    bullets.add(r * costbl[cannon.ang & 511] + layers.x, r * sintbl[cannon.ang & 511] + layers.y,
                                cannon.ang, 5.0, ENEMY, 0);
                }
            } else {
                cannon.ang -= x;
            }
        }
    }

    if (behavior == CIRCLE) {
        if (!(dist(layers.x, layers.y, tank.layers.x, tank.layers.y) > 9025.0)) {
            shootondist();
        } else {
            layers.ang &= 511;
            int x = turnstep();
            int angdist = abs(layers.ang - temp2);
            if (angdist < turn_speed) {
                layers.ang = temp2 & 511;
                layers.x = costbl[layers.ang & 511] * speed + layers.x;
                layers.y = sintbl[layers.ang & 511] * speed + layers.y;
                layers.ang = ftol(dir * turn_speed + layers.ang);
                temp2 = layers.ang & 511;
                if (enemies.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius, me)) {
                    // (original: backs out along the new heading, not the one it moved on)
                    layers.x = layers.x - costbl[layers.ang & 511] * speed;
                    layers.y = layers.y - sintbl[layers.ang & 511] * speed;
                    layers.ang = ftol(layers.ang - dir * turn_speed);
                    dir = -dir;
                    temp2 = (temp2 + 256) & 511;
                }
            } else {
                layers.ang += x;
            }
        }
    }

    if (behavior == FOLLOW) {
        if (!(dist(layers.x, layers.y, tank.layers.x, tank.layers.y) > 9025.0) && mode == 0) {
            shootondist();
        } else if (path_len == 0 || cur_point >= path_len) {
            cur_point = 0;
            findtheway(ftol(layers.x) >> 4, ftol(layers.y) >> 4, ftol(tank.layers.x) >> 4,
                       ftol(tank.layers.y) >> 4, path, &path_len);
            if (path_len == 0) {
                int ty = random(10) + (ftol(layers.y) >> 4) - 5;
                int tx = random(10) + (ftol(layers.x) >> 4) - 5;
                findtheway(ftol(layers.x) >> 4, ftol(layers.y) >> 4, tx, ty, path, &path_len);
            }
            temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[0].x, path[0].y) & 511;
        } else if (atwaypoint()) {
            nextwaypoint(1);
        } else {
            steer(me);
        }
        layers.ang &= 511;
        temp2 &= 511;
    }

    if (behavior == RANDOM) {
        double range = dist(layers.x, layers.y, tank.layers.x, tank.layers.y);
        if (!(range > 9025.0) && mode == 0) {
            shootondist();
        } else {
            if (!(range > 10000.0) && mode == 0) {   // hunt the player when close
                cur_point = 0;
                findtheway(ftol(layers.x) >> 4, ftol(layers.y) >> 4, ftol(tank.layers.x) >> 4,
                           ftol(tank.layers.y) >> 4, path, &path_len);
                temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[0].x, path[0].y);
            }
            if (path_len == 0 || cur_point >= path_len) {
                cur_point = 0;
                int ty = random(96);
                int tx = random(96);
                findtheway(ftol(layers.x) >> 4, ftol(layers.y) >> 4, tx, ty, path, &path_len);
                if (path_len == 0) {
                    ty = random(10) + (ftol(layers.y) >> 4) - 5;
                    tx = random(10) + (ftol(layers.x) >> 4) - 5;
                    findtheway(ftol(layers.x) >> 4, ftol(layers.y) >> 4, tx, ty, path, &path_len);
                }
                temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[0].x, path[0].y);
            } else if (atwaypoint()) {
                nextwaypoint(0);
            } else {
                steer(me);
            }
        }
        layers.ang &= 511;
        temp2 &= 511;
    }

    if (behavior == PATH_FIND) {
        if (!(dist(layers.x, layers.y, tank.layers.x, tank.layers.y) > 9025.0)) {
            shootondist();
        } else {
            temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[cur_point].x, path[cur_point].y) & 511;
            if (atwaypoint()) {
                layers.x = path[cur_point].x;
                layers.y = path[cur_point].y;
                cur_point++;
                if (cur_point >= path_len) cur_point = 0;
                temp2 = get_ang(ftol(layers.x), ftol(layers.y), path[cur_point].x, path[cur_point].y) & 511;
            }
            layers.ang &= 511;
            int x = turnstep();
            int angdist = abs(layers.ang - temp2);
            if (angdist < turn_speed) {
                layers.ang = temp2 & 511;
                layers.x = costbl[layers.ang & 511] * speed + layers.x;
                layers.y = sintbl[layers.ang & 511] * speed + layers.y;
            } else {
                layers.ang += x;
            }
        }
    }

    // Off the map: destroyed.
    if (0 > layers.x || 0 > layers.y || layers.x > 1536.0 || layers.y > 1536.0) power = 0;
}

static inline int onmap(double x, double y)
{
    return !(1 > x) && !(1 > y) && x < 1535.0 && y < 1535.0;
}

void enemy_type::handle(int me)
{
    if (onmap(layers.x, layers.y)) clearminmap(ftol(layers.x), ftol(layers.y));
    move(me);
    if (onmap(layers.x, layers.y)) drawonminmap(ftol(layers.x), ftol(layers.y), 0x48, 0);
    int res = bullets.checkvehiclehit(ftol(layers.x), ftol(layers.y), layers.ang, Alpha, Radius, ENEMY);
    switch (res) {
    case 1: power -= 3; break;   // bullet
    case 2: power -= 8; break;   // rocket
    }
}

void enemy_type::drawpowerbar()
{
    setcolor(0);
    int layersy = ftol(size2 + layers.y - scroll_y);
    int layersx = ftol(layers.x - size2 - scroll_x);
    int layersx2 = ftol(layers.x + 20.0 - scroll_x - size2);
    int layersx3 = ftol((double)power / power2 * 20.0 + layers.x - scroll_x - size2);
    khorline(layersy - 1, layersx, layersx2);
    khorline(layersy + 2, layersx, layersx2);
    kverline(layersx - 1, layersy, layersy + 1);
    kverline(layersx2 + 1, layersy, layersy + 1);
    setcolor(4);
    khorline(layersy, layersx, layersx2);
    khorline(layersy + 1, layersx, layersx2);
    setcolor(80);
    khorline(layersy, layersx, layersx3);
    khorline(layersy + 1, layersx, layersx3);
}

void enemy_type::draw()
{
    if (-size < layers.x - scroll_x && size + 256 > layers.x - scroll_x && -size < layers.y - scroll_y &&
        size + 192 > layers.y - scroll_y) {
        if (showpb) drawpowerbar();
        layers.draw();
    }
}

int enemy_type::dead()
{
    return power <= 0 ? 1 : 0;
}

// Returns index+1 of the enemy hit (other than 'me'). me == -1 is the player's tank,
// which also rams the enemy for |speed| damage.
int enemies_type::checkvehiclehit(int x, int y, int ang, int Alpha, double Radius, int me)
{
    for (int e_index = 0; e_index < enemies_no; e_index++) {
        if (e_index == me) continue;
        enemy_type &e = data[e_index];
        if (krotinrot(x, y, ang, ftol(e.layers.x), ftol(e.layers.y), e.layers.ang, Alpha, Radius, e.Alpha,
                      e.Radius)) {
            if (me == -1) e.power -= abs(ftol(tank.speed));
            return e_index + 1;
        }
    }
    return 0;
}

void enemies_type::handle()
{
    for (int e_index = 0; e_index < enemies_no; e_index++) {
        data[e_index].handle(e_index);
        if (data[e_index].dead()) {
            current_scores.scores += data[e_index].power2 * 2;
            current_scores.enemies++;
            expls.add(ftol(data[e_index].layers.x), ftol(data[e_index].layers.y), 45, 1);
            MixVoice(&VoiceData[4]);
            enemies_no--;
            clearminmap(ftol(data[e_index].layers.x), ftol(data[e_index].layers.y));
            data[e_index] = data[enemies_no];
            e_index--;
        }
    }
}

void enemies_type::draw()
{
    for (int e_index = 0; e_index < enemies_no; e_index++) {
        if (data[e_index].dead()) {   // debugging beep left in the original
            sound(1000);
            delay(500);
            nosound();
        }
        data[e_index].draw();
    }
}

void enemies_type::add(double x, double y, int ang, double speed, double turn_speed, unsigned power, int path_len,
                       enemytag behav, int mode)
{
    if (enemies_no >= 55) return;
    enemy_type &e = data[enemies_no];
    sprt_layers_type &src = enemy_sprts[behav];
    e.layers.close();
    e.layers.set(x, y, ang, src.sprts_no);
    for (unsigned li = 0; li < src.sprts_no; li++)
        e.layers.sprts[li].set(0, 0, src.sprts[li].cx, src.sprts[li].cy, src.sprts[li].ang, src.sprts[li].w,
                               src.sprts[li].h, src.sprts[li].data, true);
    e.mode = mode;
    e.speed = speed;
    e.turn_speed = turn_speed;
    e.power = power;
    e.power2 = power;
    e.doublespeed = ftol(speed * 2.0);
    e.size = e.layers.sprts[0].w > e.layers.sprts[0].h ? e.layers.sprts[0].w : e.layers.sprts[0].h;
    e.size2 = e.size >> 1;
    e.path_len = path_len;
    e.cur_point = 0;
    e.behavior = behav;
    e.notmyfault = 0;
    e.dir = 1;
    e.temp = ang;
    e.temp2 = ang;
    int w = e.layers.sprts[0].w, h = e.layers.sprts[0].h;
    e.Alpha = ftol(atan2((double)-(h >> 1), (double)(w >> 1)) / M_PI * 256.0);
    e.Radius = sqrt((w * w + h * h) * 0.25);
    if (e.behavior == TURRET) {   // the hull stays put, the cannon layer turns
        e.layers.sprts[1].ang = e.layers.ang;
        e.layers.ang = 0;
    }
    if (behav == PATH_FIND) {
        e.layers.ang = get_ang(ftol(x), ftol(y), e.path[1].x, e.path[1].y);
        e.cur_point = 1;
    }
    enemies_no++;
}

void enemies_type::close()
{
    enemies_no = 0;
}

enemies_type::enemies_type()
{
    enemies_no = 0;
}

void init_enemy_sprts()
{
    enemy_sprts[TURRET].set(0, 0, 0, 2);
    enemy_sprts[TURRET].sprts[0].set(0, 0, 9, 9, 0, 17, 17, turpad_img, true);
    enemy_sprts[TURRET].sprts[1].set(0, 0, 5, 4, 0, 18, 7, turcan_img, true);
    for (int b = CIRCLE; b <= FOLLOW; b++) {   // tanks share the built-in 20x17 sprite
        enemy_sprts[b].set(0, 0, 0, 1);
        enemy_sprts[b].sprts[0].set(0, 0, 10, 9, 0, 20, 17, enemy_sprt, true);
    }
    enemy_sprts[PIPER].set(0, 0, 0, 1);
    enemy_sprts[PIPER].sprts[0].set(0, 0, 8, 5, 0, 15, 9, fly_img, true);
    enemy_sprts[MIG].set(0, 0, 0, 1);
    enemy_sprts[MIG].sprts[0].set(0, 0, 10, 8, 0, 18, 15, mig_img, true);
}

void close_enemy_sprts()
{
    for (int si = 0; si < 7; si++) enemy_sprts[si].close();   // (original: 7 of 9)
}
