// COLL.H - collision tests (reconstructed, COLL.H lines 2-128)
//
// Rotated rectangles are described by a centre, an angle (512 steps per circle), a corner
// angle Alpha and a corner Radius: the corners are centre + R*(cos,sin)(ang+Alpha),
// (ang+256-Alpha), (ang+Alpha+256), (ang+512-Alpha).

int inrect(int x, int y, int x1, int y1, int x2, int y2)
{
    return x >= x1 && y >= y1 && x <= x2 && y <= y2 ? 1 : 0;
}

// Port: the original computed the squared distance in 16-bit ints, so distances above
// ~181 px wrapped around (possibly negative -> "inside"). Plain ints don't wrap.
int kincircle(int ix, int iy, int x, int y, double radius_pb2)
{
    return (ix - x) * (ix - x) + (iy - y) * (iy - y) < radius_pb2 ? 1 : 0;
}

int krectincircle(int x1, int y1, int x2, int y2, int ox, int oy, double radius_pb2)
{
    return kincircle(x1, y1, ox, oy, radius_pb2) || kincircle(x2, y1, ox, oy, radius_pb2) ||
                   kincircle(x2, y2, ox, oy, radius_pb2) || kincircle(x1, y2, ox, oy, radius_pb2)
               ? 1
               : 0;
}

int kcheckslope(int x, int y, int x1, int y1, int x2, int y2)
{
    double slope = (y1 - y2) / ((x1 - x2) + 0.000001);
    double xmin = x1 + (y1 - y) * slope;
    double xmax = x2 + (y2 - y) * slope;
    if (xmin > xmax) swapdouble(&xmin, &xmax);
    return x >= xmin && x <= xmax ? 1 : 0;
}

int kinrotrect2(int x, int y, double Ax, double Ay, double Bx, double By, double Cx, double Cy)
{
    return kcheckslope(x, y, ftol(Ax), ftol(Ay), ftol(Bx), ftol(By)) &&
                   kcheckslope(x, y, ftol(Bx), ftol(By), ftol(Cx), ftol(Cy))
               ? 1
               : 0;
}

// Axis-aligned rect (x1,y1)-(x2,y2) against a rotated rect.
// Returns 1 if a rotated corner lies in the rect, 2 if a rect corner lies in the rotated rect.
int krectinrot(int x1, int y1, int x2, int y2, double Ox, double Oy, int ang, int Alpha, double Radius)
{
    double Ax = costbl[(ang + Alpha) & 511] * Radius + Ox;
    double Ay = sintbl[(ang + Alpha) & 511] * Radius + Oy;
    double Bx = costbl[(ang + 256 - Alpha) & 511] * Radius + Ox;
    double By = sintbl[(ang + 256 - Alpha) & 511] * Radius + Oy;
    double Cx = costbl[(ang + Alpha + 256) & 511] * Radius + Ox;
    double Cy = sintbl[(ang + Alpha + 256) & 511] * Radius + Oy;
    double Dx = costbl[(ang + 512 - Alpha) & 511] * Radius + Ox;
    double Dy = sintbl[(ang + 512 - Alpha) & 511] * Radius + Oy;
    if (inrect(ftol(Ax), ftol(Ay), x1, y1, x2, y2) || inrect(ftol(Bx), ftol(By), x1, y1, x2, y2) ||
        inrect(ftol(Cx), ftol(Cy), x1, y1, x2, y2) || inrect(ftol(Dx), ftol(Dy), x1, y1, x2, y2))
        return 1;
    if (kinrotrect2(x1, y1, Ax, Ay, Bx, By, Cx, Cy) || kinrotrect2(x2, y1, Ax, Ay, Bx, By, Cx, Cy) ||
        kinrotrect2(x2, y2, Ax, Ay, Bx, By, Cx, Cy) || kinrotrect2(x1, y2, Ax, Ay, Bx, By, Cx, Cy))
        return 2;
    return 0;
}

// Rotated rect against rotated rect; the first corner found inside is stored in cur_coll.
int krotinrot(int x1, int y1, int ang1, int x2, int y2, int ang2, int Alpha1, double Radius1, int Alpha2,
              double Radius2)
{
    double Ax = x1 + costbl[(ang1 + Alpha1) & 511] * Radius1;
    double Ay = y1 + sintbl[(ang1 + Alpha1) & 511] * Radius1;
    double Bx = x1 + costbl[(ang1 + 256 - Alpha1) & 511] * Radius1;
    double By = y1 + sintbl[(ang1 + 256 - Alpha1) & 511] * Radius1;
    double Cx = x1 + costbl[(ang1 + Alpha1 + 256) & 511] * Radius1;
    double Cy = y1 + sintbl[(ang1 + Alpha1 + 256) & 511] * Radius1;
    double Dx = x1 + costbl[(ang1 + 512 - Alpha1) & 511] * Radius1;
    double Dy = y1 + sintbl[(ang1 + 512 - Alpha1) & 511] * Radius1;
    double Ex = x2 + costbl[(ang2 + Alpha2) & 511] * Radius2;
    double Ey = y2 + sintbl[(ang2 + Alpha2) & 511] * Radius2;
    double Fx = x2 + costbl[(ang2 + 256 - Alpha2) & 511] * Radius2;
    double Fy = y2 + sintbl[(ang2 + 256 - Alpha2) & 511] * Radius2;
    double Gx = x2 + costbl[(ang2 + Alpha2 + 256) & 511] * Radius2;
    double Gy = y2 + sintbl[(ang2 + Alpha2 + 256) & 511] * Radius2;
    double Hx = x2 + costbl[(ang2 + 512 - Alpha2) & 511] * Radius2;
    double Hy = y2 + sintbl[(ang2 + 512 - Alpha2) & 511] * Radius2;
    const double P1[4][2] = {{Ax, Ay}, {Bx, By}, {Cx, Cy}, {Dx, Dy}};
    const double P2[4][2] = {{Ex, Ey}, {Fx, Fy}, {Gx, Gy}, {Hx, Hy}};
    for (int i = 0; i < 4; i++) {
        if (kinrotrect2(ftol(P1[i][0]), ftol(P1[i][1]), Ex, Ey, Fx, Fy, Gx, Gy)) {
            cur_coll.x = ftol(P1[i][0]);
            cur_coll.y = ftol(P1[i][1]);
            return 1;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (kinrotrect2(ftol(P2[i][0]), ftol(P2[i][1]), Ax, Ay, Bx, By, Cx, Cy)) {
            cur_coll.x = ftol(P2[i][0]);
            cur_coll.y = ftol(P2[i][1]);
            return 1;
        }
    }
    return 0;
}
