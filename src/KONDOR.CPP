// KONDOR.CPP - Kondor v1.0 (C) 1997-1999 QSoft (Gil Dabah & Oren Becker).
//
// Native SDL2 reconstruction of the DOS game, recovered from KONDOR.EXE (Borland C++,
// shipped with its Turbo Debugger information). The original was one translation unit:
// KONDOR.CPP including the *.H files below, which contain the code; that structure, the
// function names and the algorithms are kept. Each header lists its deviations.
// (reconstructed, KONDOR.CPP lines 31-151)

#include "KONDOR.H"
#include "compat/font8x8_basic.h"

// --- globals (DGROUP order; initial values are those of the original data segment) ------
int planesperlevel;
int LEVEL = 1;
uchar current_level = 1;
char MSG[256];
char MENU_MSG[256];   // "QSOFT(C), 99" - loaded from KONDOR.EXE
options optionmenu;
uchar *tank_img, *cannon_img, *bullet_img, *expl1, *expl2, *expl3, *expl4;
uchar *texpl1, *texpl2, *texpl3, *texpl4, *texpl5, *skey_img, *cross_img, *rocket_img;
uchar *shield_img, *fly_img, *turpad_img, *mig_img, *fshoe_img, *turcan_img;
int powericon, poweractivated, speedactivated, speedcounter;
uchar *start_s, *startc_s, *resume_s, *resumec_s, *options_s, *optionsc_s, *help_s, *helpc_s;
uchar *exit_s, *exitc_s, *gameover_s, *missacco_s, *scores_s, *scoresc_s;
int nextisext;
uchar gcol = 15;
char keypress[256];
unsigned qpixels[200];
unsigned Base = 0x220, IRQ;
uchar DMA;
uchar *gbuffer;
uchar *video_buffer;
const uchar *rom_char_set;
double costbl[512], sintbl[512];
uchar *buffer, *bkg;
int scroll_x, scroll_y;
boolean resumegame;
int showpb = 1, gameloop = 1;
uchar gamepal[768];
uchar *font, *regfont;
unsigned CBuffer;
uchar *DMABuffer;
unsigned long bytes_played;
WaveData VoiceData[10];
int soundfx = 1, soundblaster;
int terrainy[900];
int hhelpscroll_y = 6, done;
char helplines[150][40];   // loaded from KONDOR.EXE
int tempsoundfx = 1;
int firekey = 0x38, breakskey = 0x39, arrowup = 0xC8, arrowdown = 0xD0, arrowleft = 0xCB, arrowright = 0xCD;
int itemkey = 0x1C, keylside = 0x1A, keyrside = 0x1B, keyrotcanl = 0x2C, keyrotcanr = 0x2D;
int fadered = 2, faderedp = 1;
char or_, og, ob;   // (original names: or, og, ob - 'or' is a C++ keyword)
hsc_type highscores[9];
hsc_type current_scores;
float cy, cx = 0.4f, dx = 0.02f;
uchar rock_img[256];      // loaded from KONDOR.EXE
uchar enemy_sprt[340];    // loaded from KONDOR.EXE
uchar gap_img[903];       // loaded from KONDOR.EXE
pointtypei cur_coll;
tank_type tank;
enemies_type enemies;
air_enemies_type air_enemies;
bases_type bases;
bonus_type bonussprts[5];
float sheildrad;
slot_type slots;
int current_slot;
sprite_type bullets_sprts[2];
boolean scrollmove = true;
uchar *map, *minmap, *tiles;
int kpixels[128];
char IMAGES_FNAME[14];
boolean canshoot = true;
unsigned bullet_max_age = 35;
bullets_type bullets;
int step;
uchar *dirmap;
pointtypei *PList[2];
expls_type expls;
sprt_layers_type enemy_sprts[9];
objects_type objects;
boolean canuseitem = true;
char masterid = 'S';
loadtank_type loadtank;
loadenemy_type loadenemy;
loadobject_type loadobject;
loadbase_type loadbase;
int counter;
int resumeyplace;

#include "QUNIT.H"
#include "PAL.H"
#include "FONT.H"
#include "SOUND.H"
#include "ALLKEYS.H"
#include "HELP.H"
#include "OPTIONS.H"
#include "HIGHSCR.H"
#include "COLL.H"
#include "KGRAPH.H"
#include "LAYERS.H"
#include "SLOTS.H"
#include "SCROLL.H"
#include "BULLETS.H"
#include "AI.H"
#include "EXPL.H"
#include "ENEMIES.H"
#include "OBJECTS.H"
#include "PTANK.H"
#include "FRAMES.H"
#include "BONUS.H"
#include "LOADMAP.H"
#include "AIRFORCE.H"
#include "BASES.H"
#include "END.H"
#include "MISC.H"
#include "GAME.H"
#include "MENU.H"

// --- port: data that lived in the original's initialised data segment -----------------------
// Rather than copying the game's text and bitmaps into this source, they are read from the
// player's own KONDOR.EXE (v1.0, 221323 bytes) at their DGROUP offsets.
static const long EXE_SIZE = 221323;
static const long DGROUP_FILE_OFFSET = 0x1600 + 0x15B20;   // MZ header + DGROUP paragraph

static void load_exe_data()
{
    FILE *f = fopen_ci("kondor.exe", "rb");
    if (f == NULL) {
        fprintf(stderr, "kondor: KONDOR.EXE (v1.0) from the original game is needed in the data directory\n");
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    struct { long off; void *dst; size_t len; } items[] = {
        {0x105, MENU_MSG, 256}, {0x3023, helplines, sizeof helplines},
        {0x4971, rock_img, sizeof rock_img}, {0x4A71, enemy_sprt, sizeof enemy_sprt},
        {0x4BC5, gap_img, sizeof gap_img}};
    for (auto &it : items) {
        fseek(f, DGROUP_FILE_OFFSET + it.off, SEEK_SET);
        fread_z(it.dst, it.len, 1, f);
    }
    fclose(f);
    if (size != EXE_SIZE || memcmp(MENU_MSG, "\xD1\xD3\xCF\xC6\xD4", 5) != 0) {   // "QSOFT", bit 7 set
        fprintf(stderr, "kondor: KONDOR.EXE is not the expected v1.0 (12/20/1999) build\n");
        exit(1);
    }
}

// The BIOS 8x8 ROM font (F000:FA6E) is replaced by the public-domain font8x8_basic,
// converted to the BIOS bit order (bit 7 = leftmost pixel).
static uchar bios_font[128 * 8];
static void init_rom_font()
{
    for (int c = 0; c < 128; c++)
        for (int r = 0; r < 8; r++) {
            uchar b = (uchar)font8x8_basic[c][r], o = 0;
            for (int i = 0; i < 8; i++)
                if (b & (1 << i)) o |= (uchar)(0x80 >> i);
            bios_font[c * 8 + r] = o;
        }
    rom_char_set = bios_font;
}

// --- KONDOR.CPP proper ------------------------------------------------------------------------

// Error screen in the BIOS font. (The second argument is ignored, as in the original.)
void writemsg(const char *str, const char *str2)
{
    (void)str2;
    fprintf(stderr, "kondor: %s\n", str);   // port: also report on the terminal
    screenis(video_buffer);
    cleardevice();
    writestrxy(0, 0, "KONDOR!");
    writestrxy(0, 10, str);
    delay(3000);
}

void filerr()
{
    fprintf(stderr, "kondor: cannot open %s\n", last_file_name());
    writemsg("FILE ERROR!", "");
    closeall();
    exit(0);
}

void memerr(const char *str)
{
    fprintf(stderr, "kondor: out of memory (%s)\n", str);
    writemsg("Error not enough memeory!", str);
    closeall();
    exit(0);
}

void initgame()
{
    init320x200();
    qpixelstable();
    if (!getvirscr(&buffer)) memerr("VIRTUAL SCREEN[1]");
    if (!getvirscr(&bkg)) memerr("VIRTUAL SCREEN[2]");
    screenis(video_buffer);
    loadsprites();
    load_game_pal();
    init_font();
    set_game_pal();
    drawstring(0, 0, "KONDOR   -   VER 1.0", font);
    drawstring(0, 7, "COPYRIGHT(C) 1997-99, QSOFT", font);
    drawstring(0, 20, "PALETTE LOADED", font);
    loadimages();
    drawstring(0, 27, "GRAPHICS LOADED", font);
    randomize();
    init_ai();
    initmap();
    maketbls();
    initminmap();
    inithelpscroll();
    init_enemy_sprts();
    kpixelstable();
    drawstring(0, 34, "MEMORY ALLOCATED", font);
    drawstring(0, 41, "KEYBOARD INITIALIZATION", font);
    getskeys();
    drawstring(0, 48, "DETECTING SB-", font);
    if (!initsb()) {
        soundblaster = 0;
        drawstring(0, 55, "FAILED!", font);
    } else {
        getsb();
        if (DMA == 0xFF || IRQ == 0) {
            soundblaster = 0;
            drawstring(0, 55, "FAILED!", font);
        } else {
            soundblaster = 1;
            drawstring(0, 55, "FOUND AT IRQ: ", font);
            drawstring(84, 55, (unsigned long)IRQ, font);
            drawstring(98, 55, "DMA: ", font);
            drawstring(125, 55, (unsigned long)DMA, font);
        }
    }
    loadoptions();
    if (soundblaster) initsound();
    loadscores();
    drawstring(0, 62, "CONFIGURATION LOADED", font);
    while (!skeypressed()) {
    }
    clearskeys();
    clearscreen();
    if (!loadpal("kondor18.dat")) filerr();
    screenis(video_buffer);
    if (!load32("kondor17.dat")) filerr();   // QSoft logo
    while (!skeypressed()) {
    }
    clearskeys();
    screenis(buffer);
    cleardevice();
    set_game_pal();
}

void closeall()
{
    close_enemy_sprts();
    closeimages();
    closeminmap();
    closemap();
    tank.layers.close();
    free(tiles);
    close_ai();
    closevirscr(buffer);
    closevirscr(bkg);
    close_font();
    closeskeys();
    closegraph();
    if (soundblaster) {
        closesb();
        closesound();
    } else {
        soundfx = tempsoundfx;
        saveoptions();
    }
}

// Port: test hook. KONDOR_TEST_LEVELS=<dir> plays every level for a few seconds without
// input (enemies move, shoot and path-find) and saves a frame of each into <dir>.
static void test_levels(const char *dir)
{
    for (int lvl = 1; lvl <= 12; lvl++) {
        current_level = lvl;
        tank.lives = 5;
        resumegame = true;
        loadmap();
        clearskeys();
        set_game_pal();
        load_side_bar();
        drawscroll();
        gameloop = 0;
        int f;
        for (f = 0; f < 400 && !gameloop && current_level == lvl; f++) {
            drawNhandle();
            if (f == 200) {
                char path[512];
                snprintf(path, sizeof path, "%s/level%02d.ppm", dir, lvl);
                plat::save_frame(path);
            }
        }
        fprintf(stderr, "level %2d: %d enemies, %d aircraft, %d bases, %u objects, power %u, lives %u, %d frames\n",
                lvl, enemies.enemies_no, air_enemies.enemies_no, bases.bases_no, objects.objects_no, tank.power,
                tank.lives, f);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (!plat::init()) return 1;
    atexit(plat::shutdown);
    init_rom_font();
    load_exe_data();
    initgame();
    if (const char *dir = getenv("KONDOR_TEST_LEVELS"))
        test_levels(dir);
    else
        menu();
    closeall();
    return 0;
}
