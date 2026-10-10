// AI.H - path finding for ground enemies (reconstructed, AI.H lines 6-112)
//
// findtheway() floods the 96x96 tile grid breadth-first from the target (tx,ty), writing in
// dirmap the direction code by which each cell was reached (W=1 N=2 NW=3 E=4 NE=6 S=8 SW=9
// SE=12). When the start cell (sx,sy) is reached it walks the codes back to the target and
// stores a waypoint (tile centre, in pixels) wherever the direction changes.
// Original quirk kept: the neighbour loop runs 7 times, so the SW neighbour is never tested.

static const int PLIST_MAX = 96 * 96;   // port: original lists held 500 points (2000 bytes)

void findtheway(int sx, int sy, int tx, int ty, pointtypei *path, int *steps_no)
{
    memset(dirmap, 0, 0x2400);
    if (waterxy(sx, sy) || waterxy(tx, ty) || minblockingxy(sx, sy) || minblockingxy(tx, ty)) {
        *steps_no = 0;
        return;
    }
    boolean goalreached = false;
    int PCnt[2] = {1, 0};
    PList[0][0].x = tx;
    PList[0][0].y = ty;
    uchar OutL = 0, InL = 1;
    uchar dir = 0;
    pointtypei newpos = {tx, ty};
    step = 1;
    do {
        step++;
        PCnt[InL] = 0;
        for (int ai = 0; ai < PCnt[OutL]; ai++) {
            newpos = PList[OutL][ai];
            dir = 2;
            newpos.y--;
            for (int bi = 0; bi < 7; bi++) {
                // port: bounds are checked first; the original looked the cell up before checking
                if (newpos.x >= 0 && newpos.y >= 0 && newpos.x < 96 && newpos.y < 96 &&
                    dirmap[newpos.y * 96 + newpos.x] == 0 && !minblockingxy(newpos.x, newpos.y) &&
                    !waterxy(newpos.x, newpos.y)) {
                    dirmap[newpos.y * 96 + newpos.x] = dir;
                    if (PCnt[InL] < PLIST_MAX) PList[InL][PCnt[InL]++] = newpos;
                }
                if (newpos.x == sx && newpos.y == sy) {
                    // walk back from the start to the target
                    step = 0;
                    newpos.x = sx;
                    newpos.y = sy;
                    int guard = 96 * 96;
                    while (!(newpos.x == tx && newpos.y == ty) && guard-- > 0) {
                        uchar d = dirmap[newpos.y * 96 + newpos.x];
                        int changed = d != dir;
                        dir = d;
                        if (changed && step < PATH_MAX) {
                            path[step].x = newpos.x * 16 + 8;
                            path[step].y = newpos.y * 16 + 8;
                            step++;
                        }
                        switch (dir) {
                        case 1: newpos.x++; break;
                        case 2: newpos.y++; break;
                        case 3: newpos.x++; newpos.y++; break;
                        case 4: newpos.x--; break;
                        case 6: newpos.x--; newpos.y++; break;
                        case 8: newpos.y--; break;
                        case 9: newpos.x++; newpos.y--; break;
                        case 12: newpos.x--; newpos.y--; break;
                        default: guard = 0; break;   // port: the original spun forever here
                        }
                    }
                    if (step < PATH_MAX) {
                        path[step].x = newpos.x * 16 + 8;
                        path[step].y = newpos.y * 16 + 8;
                        step++;
                    }
                    *steps_no = step;
                    return;
                }
                switch (dir) {   // next neighbour, clockwise from N
                case 2: newpos.x++; newpos.y++; dir = 4; break;     // N -> E
                case 4: newpos.x--; newpos.y++; dir = 8; break;     // E -> S
                case 8: newpos.x--; newpos.y--; dir = 1; break;     // S -> W
                case 1: newpos.y--; dir = 3; break;                 // W -> NW
                case 3: newpos.x += 2; dir = 6; break;              // NW -> NE
                case 6: newpos.y += 2; dir = 12; break;             // NE -> SE
                case 12: newpos.x -= 2; dir = 9; break;             // SE -> SW
                case 9: newpos.x++; newpos.y -= 2; dir = 2; break;  // SW -> N (never reached)
                }
            }
        }
        OutL = OutL == 0;
        InL = InL == 0;
    } while (!goalreached && PCnt[OutL] != 0);
    *steps_no = 0;
}

void init_ai()
{
    dirmap = (uchar *)malloc(0x2400);
    if (dirmap == NULL) memerr("AI'S Map[1]");
    memset(dirmap, 0, 0x2400);
    PList[0] = (pointtypei *)malloc(PLIST_MAX * sizeof(pointtypei));
    if (PList[0] == NULL) memerr("AI'S Map[2]");
    PList[1] = (pointtypei *)malloc(PLIST_MAX * sizeof(pointtypei));
    if (PList[1] == NULL) memerr("AI'S Map[3]");
    if (PList[0] == NULL || PList[1] == NULL) memerr("AI'S Map[4]");
}

void close_ai()
{
    free(dirmap);
    free(PList[0]);
    free(PList[1]);
}
