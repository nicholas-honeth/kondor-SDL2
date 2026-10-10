// BONUS.H - inventory items (reconstructed, BONUS.H lines 12-89)
// Item types: 0 rockets, 1 nitro (speed), 2 repair kit, 3 extra life (never stored), 4 shield.

void setbonussprts()
{
    bonussprts[0].w = 15; bonussprts[0].h = 9;  bonussprts[0].sprt = rocket_img;
    bonussprts[1].w = 17; bonussprts[1].h = 8;  bonussprts[1].sprt = fshoe_img;
    bonussprts[2].w = 15; bonussprts[2].h = 15; bonussprts[2].sprt = cross_img;
    bonussprts[3].w = 16; bonussprts[3].h = 16; bonussprts[3].sprt = rock_img;
    bonussprts[4].w = 12; bonussprts[4].h = 13; bonussprts[4].sprt = shield_img;
}

// One item used up: empty slots are cleared from the side bar.
static void usedone()
{
    slots.data[current_slot].amount--;
    if (slots.data[current_slot].amount == 0) {
        slots.data[current_slot].type = -1;
        slots.erase(current_slot);
    }
}

void usebonus()
{
    if (!canuseitem) return;
    canuseitem = false;
    int type = slots.use();
    if (type == -1) return;
    switch (type) {
    case 0:   // fire a rocket from the cannon (not while sinking)
        if (!(0 > tank.height)) {
            int ang = tank.layers.ang + tank.layers.sprts[1].ang;
            bullets.add(costbl[ang & 511] * 18.0 + tank.layers.x, sintbl[ang & 511] * 18.0 + tank.layers.y, ang, 5.0,
                        PLAYER, 1);
            usedone();
            MixVoice(&VoiceData[3]);
        }
        break;
    case 1:   // nitro: double top speed for a while (see handlecounter)
        if (speedactivated == 0) {
            speedactivated = 1;
            speedcounter = 30;
            tank.maxspeed = 2.0 * tank.maxspeed;
            usedone();
        }
        break;
    case 2:   // repair (the kit is consumed in handlecounter)
        if (tank.power < 10000 && !poweractivated) {
            poweractivated = 1;
            if (!isplaying(&VoiceData[6])) MixVoice(&VoiceData[6]);
        }
        break;
    case 4:   // shield absorbs 10-19 hits
        if (tank.shieldhits == 0) {
            slots.data[current_slot].amount--;
            tank.shieldhits = random(10) + 10;
            if (slots.data[current_slot].amount == 0) {
                slots.data[current_slot].type = -1;
                slots.erase(current_slot);
            }
            if (!isplaying(&VoiceData[8])) MixVoice(&VoiceData[8]);
        }
        break;
    }
}
