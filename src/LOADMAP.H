// LOADMAP.H - level files KONDORn.MAP (reconstructed, LOADMAP.H lines 32-159)
//
// Format (little-endian, Borland packing):
//   char  id = 'S'                  (masterid)
//   int16 level                     must equal the level being loaded
//   char  message[256]              shown under the play field
//   int16 planesperlevel            hawks kept in the air
//   uint8 map[96*96]                tile numbers
//   loadtank_type tank              (40 bytes)
//   int16 n; loadenemy_type[n+1]    (147 bytes each; PIPER/MIG go to the air force)
//   int16 n; loadobject_type[n+1]   (6 bytes each)
//   int16 n; loadbase_type[n+1]     (8 bytes each)
// Note the counts are the last index, so n+1 records follow.

// Returns 1 = ok, -1 = file missing, 2 = wrong file/level, 0 = no map buffer.
int loadfmap(const char *lfname, uchar **MAP)
{
    int g_index = 0, a_index = 0;
    FILE *lfile = fopen_ci(lfname, "rb");
    if (lfile == NULL) return -1;
    if (fgetc(lfile) != masterid) {
        fclose(lfile);
        return 2;
    }
    LEVEL = read_le16(lfile);
    if (current_level != LEVEL) {
        fclose(lfile);
        return 2;
    }
    fread_z(MSG, 256, 1, lfile);
    MSG[255] = 0;   // port: make sure the message is terminated
    planesperlevel = read_le16(lfile);
    if (*MAP == NULL) {
        fclose(lfile);
        return 0;
    }
    fread_z(*MAP, 0x2400, 1, lfile);
    fread_z(&loadtank, sizeof loadtank, 1, lfile);
    int en = read_le16(lfile);
    enemies.close();
    air_enemies.close();
    for (int il = 0; il <= en; il++) {
        fread_z(&loadenemy, sizeof loadenemy, 1, lfile);
        if (loadenemy.behavior != PIPER && loadenemy.behavior != MIG) {
            if (g_index < 55)
                for (int p = 0; p < FILE_PATH_POINTS; p++) {
                    enemies.data[g_index].path[p].x = loadenemy.PATH[p].x;
                    enemies.data[g_index].path[p].y = loadenemy.PATH[p].y;
                }
            enemies.add(loadenemy.x, loadenemy.y, loadenemy.ang, loadenemy.speed, loadenemy.turn_speed,
                        loadenemy.power, loadenemy.steps, (enemytag)loadenemy.behavior, 0);
            g_index++;
        } else {
            if (a_index < 30)
                for (int p = 0; p < FILE_PATH_POINTS; p++) {
                    air_enemies.data[a_index].path[p].x = loadenemy.PATH[p].x;
                    air_enemies.data[a_index].path[p].y = loadenemy.PATH[p].y;
                }
            air_enemies.add(loadenemy.x, loadenemy.y, loadenemy.ang, loadenemy.speed, loadenemy.turn_speed,
                            loadenemy.power, loadenemy.steps, (enemytag)loadenemy.behavior);
            a_index++;
        }
    }
    objects.objects_no = 0;
    int on = read_le16(lfile);
    for (int il = 0; il <= on; il++) {
        fread_z(&loadobject, sizeof loadobject, 1, lfile);
        objects.add(loadobject.x, loadobject.y, loadobject.type);
    }
    bases.bases_no = 0;
    int bn = read_le16(lfile);
    for (int il = 0; il <= bn; il++) {
        fread_z(&loadbase, sizeof loadbase, 1, lfile);
        bases.add(loadbase.x, loadbase.y, loadbase.gap, loadbase.power);
    }
    fclose(lfile);
    return 1;
}

void loadmap()
{
    char mapfn[32], str[12];
    strcpy(mapfn, "kondor");
    itoa(current_level, str, 10);
    strcat(mapfn, str);
    strcat(mapfn, ".map");
    int result = loadfmap(mapfn, &map);
    switch (result) {
    case -1: filerr();
    case 2:
        writemsg("FILE DECODING ERROR!", "");
        closeall();
        exit(0);
    case 0: memerr("Memory For Map");
    }
    tank.layers.close();
    tank.layers.set(loadtank.x, loadtank.y, loadtank.ang, 2);
    tank.layers.sprts[0].set(0, 0, 11, 12, 0, 23, 24, tank_img, true);    // hull
    tank.layers.sprts[1].set(0, 0, 4, 3, 0, 20, 6, cannon_img, true);    // cannon
    int w = tank.layers.sprts[0].w, h = tank.layers.sprts[0].h;
    tank.Alpha = ftol(atan2((double)((-h) >> 1), (double)(w >> 1)) / M_PI * 256.0);
    tank.Radius = sqrt((w * w + h * h) * 0.25);
    tank.accel = loadtank.accel;
    tank.speed = 0;
    tank.height = 0;
    tank.halfwidth_3 = (w >> 1) + 3;
    tank.turn_speed = loadtank.turn_speed;
    tank.cannon_turn_speed = loadtank.cannon_turn_speed;
    tank.maxspeed = loadtank.maxspeed;
    tank.power = 10000;
    setminmap();
    setbonussprts();
    for (int i = 0; i < 4; i++) {
        slots.data[i].amount = 0;
        slots.data[i].type = -1;
    }
    expls.expls_no = 0;
    bullets.bullets_no = 0;
    current_slot = 0;
    powericon = 0;
    poweractivated = 0;
    speedactivated = 0;
    speedcounter = 0;
    tank.shieldhits = 0;
    tank.shieldrad = 0;
}

// Port: two spare rows, because drawmap() reads one row (and column) past the map
// when the view is scrolled to the bottom/right edge.
void initmap()
{
    map = (uchar *)calloc(96 * 98, 1);
    if (map == NULL) memerr("Map");
}

void closemap()
{
    free(map);
}
