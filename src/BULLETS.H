// BULLETS.H - bullets and rockets (reconstructed, BULLETS.H lines 16-164)
// type 0 = bullet (5x5 sprite), type 1 = rocket (rotated sprite). Hit tests return type+1.

int bullet_type::dead()
{
    return age == 0 ? 1 : 0;
}

void bullet_type::move()
{
    sprt.x = costbl[sprt.ang & 511] * speed + sprt.x;
    sprt.y = sintbl[sprt.ang & 511] * speed + sprt.y;
    age--;
}

void bullet_type::draw()
{
    int tx = ftol(sprt.x - scroll_x);
    int ty = ftol(sprt.y - scroll_y);
    if (tx > -5 && ty > -5 && tx < 256 && ty < 192)
        kdrawsprite(ftol(sprt.x - scroll_x - 2.0), ftol(sprt.y - scroll_y - 2.0), 5, 5, bullet_img);
}

// Bullets against a rotated vehicle. The player's own bullets don't hit the player, and
// air-enemy bullets don't hit enemies.
int bullets_type::checkvehiclehit(int x, int y, double ang, int Alpha, double Radius, owner_type owner)
{
    for (unsigned b_index = 0; b_index < bullets_no; b_index++) {
        if (owner == PLAYER && data[b_index].owner == PLAYER) continue;
        if (owner != PLAYER && data[b_index].owner == AIRENEMY) continue;
        if (krotinrot(ftol(data[b_index].sprt.x), ftol(data[b_index].sprt.y), data[b_index].sprt.ang, x, y,
                      ftol(ang), Alpha, Radius, data[b_index].Alpha, data[b_index].Radius)) {
            data[b_index].age = 0;
            return data[b_index].type + 1;
        }
    }
    return 0;
}

int bullets_type::checkcirclehit(int x, int y, double radius, owner_type owner)
{
    for (unsigned b_index = 0; b_index < bullets_no; b_index++) {
        if (owner == PLAYER && data[b_index].owner == PLAYER) continue;
        if (krectincircle(ftol(data[b_index].sprt.x - 2.0), ftol(data[b_index].sprt.y - 2.0),
                          ftol(data[b_index].sprt.x + 2.0), ftol(data[b_index].sprt.y + 2.0), x, y, radius)) {
            data[b_index].age = 0;
            return data[b_index].type + 1;
        }
    }
    return 0;
}

// Bases are 43x21 rectangles.
int bullets_type::checkbasehit(int x, int y)
{
    for (unsigned b_index = 0; b_index < bullets_no; b_index++) {
        if (krectinrot(x, y, x + 43, y + 21, data[b_index].sprt.x, data[b_index].sprt.y, data[b_index].sprt.ang,
                       data[b_index].Alpha, data[b_index].Radius)) {
            data[b_index].age = 0;
            return data[b_index].type + 1;
        }
    }
    return 0;
}

// Bullets stop at blocking map tiles (air-enemy bullets fly over them).
void bullets_type::checkscrollhit()
{
    for (unsigned b_index = 0; b_index < bullets_no; b_index++) {
        if (data[b_index].owner == AIRENEMY) continue;
        int x = ftol(data[b_index].sprt.x);
        int y = ftol(data[b_index].sprt.y);
        int tx = x >> 4, ty = y >> 4;
        int sx = (x + 5) >> 4, sy = (y + 5) >> 4;
        if (blockingxy(tx, ty) || blockingxy(sx, ty) || blockingxy(sx, sy) || blockingxy(tx, sy))
            data[b_index].age = 0;
    }
}

// Dead bullets are replaced by the last one (which then skips its move this frame).
void bullets_type::handle()
{
    checkscrollhit();
    for (unsigned b_index = 0; b_index < bullets_no; b_index++) {
        if (data[b_index].dead()) {
            bullets_no--;
            data[b_index] = data[bullets_no];
        } else {
            data[b_index].move();
        }
    }
}

void bullets_type::draw()
{
    for (unsigned b_index = 0; b_index < bullets_no; b_index++) {
        if (data[b_index].type == 0)
            data[b_index].draw();
        else
            data[b_index].sprt.draw(-scroll_x, -scroll_y, 0.0);
    }
}

void bullets_type::add(double x, double y, int ang, double speed, owner_type owner, int type)
{
    if (bullets_no >= 100) return;
    bullet_type &b = data[bullets_no];
    sprite_type &s = bullets_sprts[type];
    b.sprt.set(x, y, (s.w >> 1) + 1, (s.h >> 1) + 1, ang, s.w, s.h, s.data, true);
    b.type = type;
    b.speed = speed;
    b.age = bullet_max_age;
    b.owner = owner;
    // corner angle and radius of the sprite's bounding box, for krotinrot()
    b.Alpha = ftol(atan2((double)-(b.sprt.h >> 1), (double)(b.sprt.w >> 1)) / M_PI * 256.0);
    b.Radius = ftol(sqrt((b.sprt.w * b.sprt.w + b.sprt.h * b.sprt.h) * 0.25));
    bullets_no++;
}

bullets_type::bullets_type()
{
    bullets_no = 0;
}

// The player fires from the cannon tip, 18 px from the tank centre (not while sinking).
void shoot()
{
    if (!canshoot) return;
    if (0 > tank.height) return;
    MixVoice(&VoiceData[2]);
    int ang = tank.layers.ang + tank.layers.sprts[1].ang;
    bullets.add(tank.layers.x + costbl[ang & 511] * 18.0, tank.layers.y + sintbl[ang & 511] * 18.0, ang, 5.0,
                PLAYER, 0);
    canshoot = false;
}
