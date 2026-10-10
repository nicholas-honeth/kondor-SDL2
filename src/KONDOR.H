// KONDOR.H - types, globals and prototypes of Kondor (QSoft, 1999), recovered from the
// Turbo Debugger information left in KONDOR.EXE (all 31 struct/class layouts, names and
// enumerators are the original ones).
//
// Port notes:
//  * Borland's 16-bit int is a 32-bit int here, far pointers are plain pointers.
//  * Data read from or written to files uses explicit 16-bit little-endian layouts
//    (the load*_type structs below, KONDOR.CFG, KONDOR.HSC), so files stay compatible.
//  * Borland's 'enum boolean {false, true}' is an int typedef (C++ has its own true/false).
#pragma once

#include "compat/borland.h"
#include "platform/platform.h"

typedef int boolean;
enum owner_type { PLAYER = 0, ENEMY = 1, AIRENEMY = 2 };
enum enemytag { EMPTY = 0, TURRET = 1, CIRCLE = 2, RANDOM = 3, PATH_FIND = 4, ANGLE = 5, FOLLOW = 6, PIPER = 7, MIG = 8 };
enum options { start = 0, resume = 1, option = 2, hghscores = 3, help = 4, exitgame = 5 };

struct pointtypei {
    int x;
    int y;
};

// Enemy waypoints: 30 in the map files; findtheway() may produce more (the original then
// overwrote the next fields), so the in-memory list of ground enemies is larger.
static const int FILE_PATH_POINTS = 30;
static const int PATH_MAX = 256;

struct WaveData {
    unsigned long length;
    uchar *data;
    unsigned long finishing;
};

class sprite_type {
public:
    double x, y;      // offset from the owner
    double cx, cy;    // rotation centre inside the sprite
    int ang;
    int w, h;
    uchar *data;
    boolean visible;
    void set(double nx, double ny, double ncx, double ncy, int nang, int nw, int nh, uchar *ndata, boolean nvisible);
    void draw(double px, double py, double pang);
};

class sprt_layers_type {
public:
    double x, y;
    int ang;
    unsigned sprts_no;
    sprite_type sprts[5];
    void draw();
    void set(double nx, double ny, int nang, unsigned nsprts_no);
    void close();
};

struct slot {
    int amount;
    int type;
    uchar *sprt;
};

class slot_type {
public:
    slot data[4];
    int slotempty(int n);
    int getslotn(int type);
    int use();
    void add(int type);
    void drawsprt(int n);
    void draw();
    void erase(int n);
};

class bullet_type {
public:
    double speed;
    int Alpha;
    int Radius;
    owner_type owner;
    int type;
    sprite_type sprt;
    unsigned age;
    int dead();
    void move();
    void draw();
};

class bullets_type {
public:
    bullet_type data[100];
    unsigned bullets_no;
    int checkvehiclehit(int x, int y, double ang, int Alpha, double Radius, owner_type owner);
    int checkcirclehit(int x, int y, double radius, owner_type owner);
    int checkbasehit(int x, int y);
    void checkscrollhit();
    void handle();
    void draw();
    void add(double x, double y, int ang, double speed, owner_type owner, int type);
    bullets_type();
};

struct frame_type {
    int w, h;
    uchar *sprt;
};

class frames_type {
public:
    int frame;
    int maxframe;
    frame_type frames[10];
    void add(int w, int h, uchar *sprt);
    void draw(int x, int y);
    frames_type();
};

class expl_type {
public:
    int x, y;
    int age;
    frames_type frames;
    int dead();
    void draw();
    void handle();
};

class expls_type {
public:
    int expls_no;
    expl_type data[10];
    void handle();
    void draw();
    void add(int x, int y, int age, int type);
    expls_type();
};

class enemy_type {
public:
    enemytag behavior;
    int mode;
    int temp;
    int temp2;
    int dir;
    double speed;
    double turn_speed;
    int power;
    int size;
    int power2;
    int size2;
    int doublespeed;
    int Alpha;
    int moveable;
    int notmyfault;
    double Radius;
    sprt_layers_type layers;
    pointtypei path[PATH_MAX];
    int path_len;
    int cur_point;
    void shootondist();
    void smartmove(int me);
    void move(int me);
    void handle(int me);
    void drawpowerbar();
    void draw();
    int dead();
    // port: helpers factored out of move(), which repeats these blocks verbatim
    void replan();
    int turnstep();
    int atwaypoint();
    void nextwaypoint(int mask);
    void steer(int me);
};

class enemies_type {
public:
    enemy_type data[55];
    int enemies_no;
    int checkvehiclehit(int x, int y, int ang, int Alpha, double Radius, int me);
    void handle();
    void draw();
    void add(double x, double y, int ang, double speed, double turn_speed, unsigned power, int path_len, enemytag behav,
             int mode);
    void close();
    enemies_type();
};

class object_type {
public:
    int x, y, w, h;
    int type;
    uchar *sprt;
    void draw();
};

class objects_type {
public:
    object_type data[40];
    unsigned objects_no;
    int checkvehiclehit(int x, int y, int ang, int Alpha, double Radius);
    void draw();
    void handle();
    void add(double x, double y, int type);
    objects_type();
};

class tank_type {
public:
    sprt_layers_type layers;
    double accel;
    double speed;
    double maxspeed;
    double turn_speed;
    double cannon_turn_speed;
    int Alpha;
    double Radius;
    float height;
    float heightspeed;
    unsigned power;
    unsigned lives;
    int shieldhits;
    float shieldrad;
    int halfwidth_3;
    void hit(unsigned damage);
    void isdead();
    void handle();
    int moveable(int x, int y);
    int checkhit();
    void turn(int direction);
    void draw();
    void move();
    void turn_cannon(int direction);
};

class air_enemy_type {
public:
    pointtypei path[FILE_PATH_POINTS];
    int tempx, tempy;
    int mode, mode2;
    int cur_point;
    int path_len;
    enemytag behavior;
    int temp, temp2;
    double speed, speed2;
    double turn_speed;
    int power;
    int size;
    int halfwidth, halfheight;
    int Alpha;
    double Radius;
    sprite_type sprt;
    void targetondist();
    void move();
    void handle();
    void draw();
    int dead();
};

class air_enemies_type {
public:
    air_enemy_type data[30];
    int enemies_no;
    void handle();
    void draw();
    void add(double x, double y, int ang, double speed, double turn_speed, unsigned power, int path_len,
             enemytag behav);
    void close();
    air_enemies_type();
};

class base_type {
public:
    int x, y;
    int counter;
    int gap;
    int power;
    int size;
    int power2;
    int size2;
    void handle();
    void draw();
    void drawpowerbar();
    void drawmap();
    void clearmap();
};

class bases_type {
public:
    base_type data[5];
    int bases_no;
    void add(int x, int y, int gap, int power);
    int checkvehiclehit(int x, int y, int ang, int Alpha, double Radius);
    void handle();
    void draw();
};

struct bonus_type {
    int w, h;
    uchar *sprt;
};

// --- on-disk records (Borland byte packing, 16-bit ints, 8-byte doubles) -------------
#pragma pack(push, 1)
struct hsc_type {   // KONDOR.HSC
    char name[14];
    uint16_t enemies;
    uint16_t scores;
};
struct pointtype16 {
    int16_t x, y;
};
struct loadbase_type {
    int16_t x, y, gap, power;
};
struct loadobject_type {
    int16_t x, y, type;
};
struct loadenemy_type {
    int16_t x, y, ang;
    double speed;
    double turn_speed;
    uint16_t power;
    uint8_t steps;
    pointtype16 PATH[FILE_PATH_POINTS];
    int16_t behavior;
};
struct loadtank_type {
    int16_t x, y, ang;
    double accel;
    double turn_speed;
    double cannon_turn_speed;
    double maxspeed;
    uint16_t power;
};
#pragma pack(pop)
static_assert(sizeof(hsc_type) == 18, "hsc_type layout");
static_assert(sizeof(loadbase_type) == 8, "loadbase_type layout");
static_assert(sizeof(loadobject_type) == 6, "loadobject_type layout");
static_assert(sizeof(loadenemy_type) == 147, "loadenemy_type layout");
static_assert(sizeof(loadtank_type) == 40, "loadtank_type layout");

// --- globals (in DGROUP order) ----------------------------------------------------------
extern int planesperlevel;
extern int LEVEL;
extern uchar current_level;
extern char MSG[256];
extern char MENU_MSG[256];
extern options optionmenu;
extern uchar *tank_img, *cannon_img, *bullet_img, *expl1, *expl2, *expl3, *expl4;
extern uchar *texpl1, *texpl2, *texpl3, *texpl4, *texpl5, *skey_img, *cross_img, *rocket_img;
extern uchar *shield_img, *fly_img, *turpad_img, *mig_img, *fshoe_img, *turcan_img;
extern int powericon, poweractivated, speedactivated, speedcounter;
extern uchar *start_s, *startc_s, *resume_s, *resumec_s, *options_s, *optionsc_s, *help_s, *helpc_s;
extern uchar *exit_s, *exitc_s, *gameover_s, *missacco_s, *scores_s, *scoresc_s;
extern int nextisext;
extern uchar gcol;
extern char keypress[256];
extern unsigned qpixels[200];
extern unsigned Base, IRQ;
extern uchar DMA;
extern uchar *gbuffer;
extern uchar *video_buffer;
extern const uchar *rom_char_set;
extern double costbl[512], sintbl[512];
extern uchar *buffer, *bkg;
extern int scroll_x, scroll_y;
extern boolean resumegame;
extern int showpb, gameloop;
extern uchar gamepal[768];
extern uchar *font, *regfont;
extern unsigned CBuffer;
extern uchar *DMABuffer;
extern unsigned long bytes_played;
extern WaveData VoiceData[10];
extern int soundfx, soundblaster;
extern int terrainy[900];
extern int hhelpscroll_y, done;
extern char helplines[150][40];
extern int tempsoundfx;
extern int firekey, breakskey, arrowup, arrowdown, arrowleft, arrowright, itemkey, keylside, keyrside;
extern int keyrotcanl, keyrotcanr;
extern int fadered, faderedp;
extern char or_, og, ob;
extern hsc_type highscores[9];
extern hsc_type current_scores;
extern float cy, cx, dx;
extern uchar rock_img[256];
extern uchar enemy_sprt[340];
extern uchar gap_img[903];
extern pointtypei cur_coll;
extern tank_type tank;
extern enemies_type enemies;
extern air_enemies_type air_enemies;
extern bases_type bases;
extern bonus_type bonussprts[5];
extern float sheildrad;
extern slot_type slots;
extern int current_slot;
extern sprite_type bullets_sprts[2];
extern boolean scrollmove;
extern uchar *map, *minmap, *tiles;
extern int kpixels[128];
extern char IMAGES_FNAME[14];
extern boolean canshoot;
extern unsigned bullet_max_age;
extern bullets_type bullets;
extern int step;
extern uchar *dirmap;
extern pointtypei *PList[2];
extern expls_type expls;
extern sprt_layers_type enemy_sprts[9];
extern objects_type objects;
extern boolean canuseitem;
extern char masterid;
extern loadtank_type loadtank;
extern loadenemy_type loadenemy;
extern loadobject_type loadobject;
extern loadbase_type loadbase;
extern int counter;
extern int resumeyplace;

// --- functions (by original source file) ------------------------------------------------
// QUNIT.H
void init320x200();
void closegraph();
void qpixelstable();
void setpal(char col, char r, char g, char b);
void getpal(char col, char *red, char *green, char *blue);
void waitretrace();
void fadeout();
void fadein(char *savedpal);
void blackpal();
void plotpixel(unsigned x, unsigned y, uchar color);
void setcolor(uchar color);
void cleardevice();
void swapint(int *num1, int *num2);
void swapdouble(double *num1, double *num2);
void verline(int x, int y, int y2);
void horline(int y, int x, int x2);
void bar(int x, int y, int x2, int y2);
void drawsprite(int x, int y, int w, int h, uchar *sprt);
void drawsprites(int x, int y, int w, int h, uchar *sprt);
void circle(int x, int y, int radius);
void drawscreen(uchar *buf);
void copyscreen(uchar *dest, uchar *src);
void screenis(uchar *buf);
void closevirscr(uchar *buffer);
void maketbls();
int loadpal(const char *fname);
void getskeys();
int keydown(int nkey);
int skeypressed();
void clearskeys();
void closeskeys();
void WriteDSP(uchar value);
int ResetDSP(unsigned test);
uchar ReadDSP();
int initsb();
void closesb();
void plotchar(int x, int y, char c);
void writestrxy(int x, int y, const char *string);
void writestrxy(int x, int y, int number);
int getvirscr(uchar **buf);
int load32(const char *fname);
// PAL.H
void makeshadowpal();
int load_game_pal();
void set_game_pal();
// FONT.H
void convsprt(int w, int h, char *sprt);
void drawchar(int x, int y, char ch, uchar *fnt);
void drawcharreg(int x, int y, char ch, uchar *fnt);
void drawstring(int x, int y, const char *s, uchar *fnt);
void drawstring(int x, int y, unsigned long n, uchar *fnt);
void drawstringreg(int x, int y, const char *s, uchar *fnt);
void drawstringreg(int x, int y, unsigned long n, uchar *fnt);
void load_font(const char *filename);
void set_font_pal();
void init_font();
void close_font();
// SOUND.H
unsigned long DMAInCurrentBlock();
int isplaying(WaveData *Voice);
void StartPlayBack();
void ClearBuffer(unsigned Buffer);
void MixVoice(WaveData *Voice);
void LoadVoice(WaveData *Voice, const char *FileName);
void ConvertVoice(WaveData *Voice);
void AssignBuffer();
uchar getdma();
void getirq();
void getsb();
void initsound();
void closesound();
// ALLKEYS.H
const char *getkeyname(int keyn);
char skey2char(int keyn);
// HELP.H
void inithelpscroll();
void drawhelpscroll();
void gamehelp();
// OPTIONS.H
void saveoptions();
int loadoptions();
void drawrect(int x, int y, int x2, int y2);
int textplace(int which);
void drawtext();
void drawwindow(int chosen);
int *ptrkey(int which);
int checkkey(int *keyn, int which);
void waitforkey(int *key, int which);
void waitforspace(int *val, int win);
void gameoptions();
// HIGHSCR.H
int getscoreplace();
void drawscores();
void savescores();
void loadscores();
void showhighscr();
void drawcursor(int x, int y, int pos);
void enterhsc(int place);
// COLL.H
int inrect(int x, int y, int x1, int y1, int x2, int y2);
int kincircle(int ix, int iy, int x, int y, double radius_pb2);
int krectincircle(int x1, int y1, int x2, int y2, int ox, int oy, double radius_pb2);
int kcheckslope(int x, int y, int x1, int y1, int x2, int y2);
int kinrotrect2(int x, int y, double Ax, double Ay, double Bx, double By, double Cx, double Cy);
int krectinrot(int x1, int y1, int x2, int y2, double Ox, double Oy, int ang, int Alpha, double Radius);
int krotinrot(int x1, int y1, int ang1, int x2, int y2, int ang2, int Alpha1, double Radius1, int Alpha2,
              double Radius2);
// KGRAPH.H
void kplotpixel(unsigned x, unsigned y, uchar color);
void krotsprite(int x, int y, int cx, int cy, int ang, int w, int h, uchar *sprt);
void kdrawsprite(int x, int y, int w, int h, uchar *sprt);
void drawsink(int x, int y, int cx, int cy, int ang, int w, int h, uchar *sprt, int sink_w1, int sink_w2,
              int sink_h1, int sink_h2);
void khorline(int y, int x, int x2);
void kverline(int x, int y, int y2);
// SLOTS.H
void drawslotrect(int x, int y, int x2, int y2);
void drawslots(int color);
// SCROLL.H
int loadsprites();
void setimagesfile(const char *str);
int loadimg(int n, uchar **img);
void loadimages();
void closeimages();
void qmemcpy(uchar *dest, uchar *src, int size);
void kmemcpy(uchar *dest, uchar *src, int size);
void ldrawcell(int y, int n, int left);
void rdrawcell(int y, int n, int left);
void udrawcell(int x, int n, int left);
void ddrawcell(int x, int n, int left);
void fdrawcell(int x, int y, int n);
void drawcell(int x, int y, int n);
void drawmap();
void set_map(int x, int y, int val);
void checkwater(int offset);
void movewater();
int water(uchar value);
uchar maptile(int x, int y);
int waterxy(int x, int y);
int blocking(uchar value);
int blockingxy(int x, int y);
int minblocking(uchar value);
int minblockingxy(int x, int y);
uchar getmapcol(uchar tile);
void setminmap();
void clearminmap(int x2, int y2);
void drawonminmap(int x2, int y2, int col, int ccol);
void initminmap();
void kpixelstable();
void closeminmap();
void drawminmap();
void drawscroll();
void check4scroll();
// BULLETS.H
void shoot();
// AI.H
void findtheway(int sx, int sy, int tx, int ty, pointtypei *path, int *steps_no);
void init_ai();
void close_ai();
// ENEMIES.H
double dist(double x1, double y1, double x2, double y2);
int get_ang(int x1, int y1, int x2, int y2);
void init_enemy_sprts();
void close_enemy_sprts();
// PTANK.H
int checksink();
// BONUS.H
void setbonussprts();
void usebonus();
// LOADMAP.H
int loadfmap(const char *lfname, uchar **MAP);
void loadmap();
void initmap();
void closemap();
// END.H
void gameoverpage();
// MISC.H
void gotonextlvl();
void handlecounter();
// GAME.H
void load_side_bar();
void drawNhandle();
void start_game();
// MENU.H
void drawmenu();
void clearscreen();
void menu();
// KONDOR.CPP
void writemsg(const char *str, const char *str2);
[[noreturn]] void filerr();
[[noreturn]] void memerr(const char *str);
void initgame();
void closeall();
