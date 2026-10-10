# Testing the port

How the port was tested on the Rotwang VM, with every command needed to repeat it by hand.
Each test lists what a pass looks like. The procedures were last run in full on 2026-10-10,
and the results are recorded at the end.

## 0. Environment

Rotwang: Ubuntu 24.04, GCC 13.3, CMake 3.28, SDL 2.30, no physical display or sound card.
Log in from the laptop with `ssh rotwang` (it jumps through Aristo on wg0).

Packages used by the tests:

```sh
sudo apt install build-essential cmake libsdl2-dev python3 \
                 xvfb xdotool imagemagick dosbox unzip
```

- `xvfb` is a virtual X display, used for the windowed test and for DOSBox.
- `xdotool` sends key presses to DOSBox.
- `imagemagick` converts the PPM screenshots and combines them into contact sheets.
- `dosbox` runs the original game as the reference.

Set up a scratch area with a clean copy of the game. All later commands assume these
variables are set, so run the next block in every new shell:

```sh
export P=~/kondor-port          # port source
export T=~/ktest                # scratch area for the tests
export ZIP=~/re/kondor/kond-box.zip
```

```sh
mkdir -p $T && cd $T
sha256sum $ZIP        # 86117edc794d6c9adb435d4c809331dde3d39d60b15cdbbf9d3f3bfd10e4a418
rm -rf game && unzip -q $ZIP -d game
sha256sum game/KONDOR/KONDOR.EXE   # 231079fed9609f29b067c2b24fcce84d9063c7b9bd80240883f6ed8e84fa935b
```

The game writes `KONDOR.CFG` and `KONDOR.HSC` into its directory, so each test runs in a
fresh copy of `game/KONDOR`. The original files stay untouched.

### Viewing screenshots

Screenshots are PPM files. To look at them, convert them on Rotwang and copy them to the
laptop:

```sh
# on Rotwang
convert $T/shots/frame_000420.ppm -scale 200% /tmp/x.png
# on the laptop
scp rotwang:/tmp/x.png . && explorer.exe x.png      # or any image viewer
```

### Key codes for scripts

A key script (`KONDOR_SCRIPT`) has one event per line: `<frame> <down|up> <code in hex>`.
Codes are XT scan codes; add `0x80` for extended keys such as the arrows. Frames are
counted from program start, at 70 per second.

| Key | Code | | Key | Code |
|---|---|---|---|---|
| Esc | `01` | | ↑ | `c8` |
| Enter | `1c` | | ↓ | `d0` |
| Space | `39` | | ← | `cb` |
| Left Alt (fire) | `38` | | → | `cd` |
| `[` / `]` | `1a` / `1b` | | Z / X | `2c` / `2d` |

Startup sequence: the boot screen waits for a key, then the QSoft logo waits for a key, then
the menu appears with START highlighted. The menu items are START, OPTIONS, SCORES, HELP
and EXIT. RESUME also appears once a game has started.

**Use absolute paths** for `KONDOR_SCRIPT` and `KONDOR_SHOTS`, because the commands below
`cd` into the game directory first. A script that can't be opened stops the program with
`cannot open KONDOR_SCRIPT`.

## 1. Build

```sh
cd $P
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8 2>&1 | grep -cE "warning|error"        # expect 0

cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan -j8 2>&1 | grep -cE "warning|error"   # expect 0
```

`build/kondor` is the normal build. `build-asan/kondor` adds AddressSanitizer and
UndefinedBehaviorSanitizer, which detect out-of-bounds accesses, use-after-free,
signed overflow and similar errors at run time.

**Pass:** both builds succeed with zero warnings. The Release build matters for this:
Ubuntu's compiler enables `_FORTIFY_SOURCE` only at `-O2`, and that adds checks such as
unused `fread` results.

## 2. Smoke test: boot, menu, start, drive

```sh
cd $T && rm -rf play shots && cp -r game/KONDOR play && mkdir shots
cat > smoke.txt <<'EOF'
40 down 39
43 up 39
80 down 39
83 up 39
140 down 1c
143 up 1c
260 down c8
420 up c8
EOF
cd play && KONDOR_HEADLESS=1 KONDOR_NOSOUND=1 KONDOR_SEED=1 \
  KONDOR_SCRIPT=$T/smoke.txt KONDOR_SHOTS=$T/shots \
  KONDOR_SHOT_FRAMES=20,60,120,200,300,420 KONDOR_MAXFRAMES=450 \
  timeout -s KILL 60 $P/build/kondor; echo "exit code $?"
cd $T && montage shots/*.ppm -tile 3x2 -geometry +2+2 /tmp/smoke.png
```

The script presses Space twice (boot screen, then logo), Enter (START), then holds ↑ for
160 frames.

**Pass:**

- the program prints `exit after 450 frames` and exit code 0;
- the six screenshots show, in order:
  1. the boot text ending with "DETECTING SB- FAILED!" and "CONFIGURATION LOADED";
  2. the QSoft logo;
  3. the menu;
  4. level 1 with the "KILL ALL ENEMIES..." message;
  5. the tank driving north;
  6. the tank sinking into the water north of the start.

`timeout -s KILL` is deliberate: SDL catches SIGTERM, so a hung build would ignore plain
`timeout`.

## 3. Menu screens: options, scores, help

```sh
cd $T && rm -rf menu && mkdir menu
shot() {  # shot <script> <frames> <maxframes>
  rm -rf menu-game && cp -r game/KONDOR menu-game
  (cd menu-game && KONDOR_HEADLESS=1 KONDOR_NOSOUND=1 KONDOR_SEED=1 KONDOR_FAST=1 \
     KONDOR_SCRIPT=$T/$1 KONDOR_SHOTS=$T/menu KONDOR_SHOT_FRAMES=$2 KONDOR_MAXFRAMES=$3 \
     timeout -s KILL 120 $P/build/kondor 2>&1 | tail -1)
}
printf "10 down 39\n12 up 39\n30 down 39\n32 up 39\n60 down d0\n62 up d0\n70 down 1c\n72 up 1c\n" > s_options.txt
printf "10 down 39\n12 up 39\n30 down 39\n32 up 39\n60 down d0\n62 up d0\n64 down d0\n66 up d0\n70 down 1c\n72 up 1c\n" > s_scores.txt
printf "10 down 39\n12 up 39\n30 down 39\n32 up 39\n60 down d0\n62 up d0\n64 down d0\n66 up d0\n68 down d0\n70 up d0\n74 down 1c\n76 up 1c\n200 down d0\n400 up d0\n" > s_help.txt
shot s_options.txt 200 210;    mv menu/frame_000200.ppm menu/1_options.ppm
shot s_scores.txt 200 210;     mv menu/frame_000200.ppm menu/2_scores.ppm
shot s_help.txt 180,400 410;   mv menu/frame_000180.ppm menu/3_help.ppm; mv menu/frame_000400.ppm menu/4_help_scrolled.ppm
montage menu/*.ppm -tile 4x1 -geometry +2+2 /tmp/menus.png
```

**Pass:**

- the OPTIONS screen lists the default keys (FIRE: LEFT ALT, BREAK: SPACEBAR, … ROTATE
  CANNON RIGHT: X), plus POWERBAR: ON and SOUND: OFF. Sound is OFF because no sound device
  was found;
- the SCORES table is empty, because the shipped `KONDOR.HSC` is empty;
- HELP shows the credits text between blue scrolling borders, and holding ↓ scrolls it to
  the webmaster address.

`KONDOR_FAST=1` only removes the waiting between frames. Because the script counts
frames, the screenshots are the same with or without it.

## 4. All twelve levels

```sh
cd $T && rm -rf levels levels-game && mkdir levels && cp -r game/KONDOR levels-game
printf "10 down 39\n12 up 39\n40 down 39\n42 up 39\n" > start.txt
cd levels-game && ASAN_OPTIONS=detect_leaks=0 KONDOR_TEST_LEVELS=$T/levels \
  KONDOR_HEADLESS=1 KONDOR_NOSOUND=1 KONDOR_FAST=1 KONDOR_SEED=7 \
  KONDOR_SCRIPT=$T/start.txt timeout -s KILL 600 $P/build-asan/kondor 2>&1 \
  | grep -E "^level|runtime error|AddressSanitizer"
cd $T && montage levels/*.ppm -tile 6x2 -geometry +2+2 /tmp/levels.png
```

`KONDOR_TEST_LEVELS` loads levels 1–12 in turn and plays each for 400 frames without
input. Enemies move, path-find and shoot at the stationary tank. It saves frame 200 of each
level.

**Pass:**

- twelve `level N:` lines, each with `400 frames`;
- no sanitizer output;
- twelve level images in `$T/levels`.

Expected counts (seed 7):

```
level  1: 6 enemies, 0 aircraft, 1 bases, 4 objects, power 8400, lives 5, 400 frames
level  2: 12 enemies, 1 aircraft, 1 bases, 7 objects, power 10000, lives 5, 400 frames
level  3: 26 enemies, 1 aircraft, 1 bases, 14 objects, power 6200, lives 5, 400 frames
level  4: 28 enemies, 2 aircraft, 1 bases, 36 objects, power 9000, lives 5, 400 frames
level  5: 45 enemies, 1 aircraft, 1 bases, 10 objects, power 10000, lives 5, 400 frames
level  6: 2 enemies, 5 aircraft, 1 bases, 40 objects, power 10000, lives 5, 400 frames
level  7: 19 enemies, 2 aircraft, 1 bases, 40 objects, power 9200, lives 5, 400 frames
level  8: 12 enemies, 1 aircraft, 4 bases, 40 objects, power 10000, lives 5, 400 frames
level  9: 15 enemies, 2 aircraft, 3 bases, 25 objects, power 10000, lives 5, 400 frames
level 10: 22 enemies, 4 aircraft, 1 bases, 40 objects, power 9000, lives 5, 400 frames
level 11: 20 enemies, 3 aircraft, 2 bases, 39 objects, power 10000, lives 5, 400 frames
level 12: 29 enemies, 5 aircraft, 3 bases, 36 objects, power 8800, lives 5, 400 frames
```

"power" is the tank's health out of 10,000, so lower values mean enemies hit it. Identical
numbers on a re-run show that the game is deterministic for a given seed.

## 5. Random-input sanitizer runs

`tools/fuzzkeys.py <seed> <frames>` generates a random key script. About 75% of the presses
are game keys (fire, drive, turn, items, cannon) and 25% are menu keys (Enter, Escape,
Space and others), held for 1–90 frames. `tools/fuzz.sh` runs one sanitizer session per
seed in a fresh game copy, several in parallel:

```sh
cd $T
# without sound: seeds 1-12, up to 20,000 frames each, 6 in parallel
FUZZ_OUT=$T/fuzz $P/tools/fuzz.sh $P/build-asan/kondor game/KONDOR 1 12 20000 6

# with sound: the audio path runs into SDL's "dummy" driver
KONDOR_NOSOUND=0 FUZZ_OUT=$T/fuzz-sound $P/tools/fuzz.sh $P/build-asan/kondor game/KONDOR 41 60 20000 6
```

Each line of output looks like
`seed 3 rc=0 sanitizer_reports=0 exit after 19381 frames`.

**Pass:** every line has `rc=0` and `sanitizer_reports=0`. If a seed fails, its log is in
`$T/fuzz/log<seed>.txt` with a stack trace, and `script<seed>.txt` replays it exactly:

```sh
cd $T && rm -rf replay && cp -r game/KONDOR replay && cd replay
ASAN_OPTIONS=detect_leaks=0 KONDOR_HEADLESS=1 KONDOR_NOSOUND=1 KONDOR_FAST=1 KONDOR_SEED=3 \
  KONDOR_SCRIPT=$T/fuzz/script3.txt KONDOR_MAXFRAMES=20000 $P/build-asan/kondor
```

How to read the frame counts:

- a run that ends before the limit isn't a failure: the random keys chose EXIT in the menu
  (`rc=0`);
- the same seed always reaches the same frame count, which is a quick determinism check;
- for longer coverage, raise the frame limit or the number of seeds.

## 6. Reference: the original in DOSBox

The original `KONDOR.EXE` runs in DOSBox on a virtual X display. Screenshots are taken
with DOSBox's Ctrl+F5.

```sh
# virtual display :7 (once per boot)
pgrep -f "Xvfb :7" || (Xvfb :7 -screen 0 800x600x24 >/dev/null 2>&1 &)
export DISPLAY=:7

mkdir -p $T/ref/cap && rm -f $T/ref/cap/* && rm -rf $T/ref/game && cp -r $T/game/KONDOR $T/ref/game
cat > $T/ref/dosbox.conf <<EOF
[sdl]
output=surface
[dosbox]
captures=$T/ref/cap
machine=svga_s3
[cpu]
core=auto
cycles=max
[render]
scaler=none
aspect=false
[sblaster]
sbtype=sb16
[autoexec]
mount c $T/ref/game
c:
KONDOR.EXE
EOF
(SDL_AUDIODRIVER=dummy setsid dosbox -conf $T/ref/dosbox.conf > $T/ref/dosbox.log 2>&1 &)
sleep 6
W=$(xdotool search --name DOSBox | head -1)
k() { xdotool key --window $W $1; sleep ${2:-1}; }
k space 3; k space 3; k ctrl+F5 1          # boot screen, logo, capture the menu
k Return 4; k ctrl+F5 1                     # START, capture level 1
xdotool keydown --window $W Up              # drive north, one capture per second
for i in 1 2 3 4 5; do sleep 1; xdotool key --window $W ctrl+F5; done
xdotool keyup --window $W Up; sleep 2; k ctrl+F5 1
ls $T/ref/cap                               # kondor_000.png ... kondor_007.png
montage $T/ref/cap/*.png -tile 4x2 -geometry +2+2 /tmp/refcap.png
pkill -x dosbox
```

Compare `/tmp/refcap.png` with the port's screenshots from tests 2 and 3. The SB16 is
detected, so the boot screen in DOSBox reports the card.

**Pass:**

- the menu and the level-1 start look the same: pixel art, palette and layout, including
  the rocks, the base, enemy tank and turret positions, side bar, minimap and message line;
- driving north leads into the water in both games, the tank sinks, and a dark patch is
  left where it went under.

**Limitations of this comparison:**

- it is visual, not a pixel-by-pixel diff;
- DOSBox plus `xdotool` lag behind real time, so the original reaches a given point later
  than the port. The port's tank is in the water after about 2.3 s of ↑ (test 2); the
  DOSBox one after about 3–4 s. Compare *what* happens, not *when*;
- enemy behaviour depends on the random seed, which DOSBox takes from the clock.

Notes:

- `pkill -x dosbox` matches the process name exactly. When these commands are run through
  `ssh rotwang '…'`, `pkill -f dosbox` would also kill the remote shell, whose command line
  contains "dosbox".
- `setsid` detaches DOSBox from the ssh session.

## 7. Windowed run and frame rate

This checks the real SDL window path, as opposed to headless mode, at the paced 70 Hz:

```sh
export DISPLAY=:7
cd $T && rm -rf win && cp -r game/KONDOR win && cd win
t0=$(date +%s.%N)
KONDOR_NOSOUND=1 KONDOR_SCRIPT=$T/smoke.txt KONDOR_MAXFRAMES=700 \
  timeout -s KILL 60 $P/build/kondor 2>&1 | tail -1 &
sleep 7; import -window root /tmp/xwin.png      # screenshot of the whole virtual screen
wait; t1=$(date +%s.%N)
python3 -c "print('700 frames in %.2f s = %.1f fps' % ($t1-$t0, 700/($t1-$t0)))"
```

**Pass:**

- about 10 s for 700 frames, which is close to 70 fps. The measured time includes start-up,
  so expect 66–70 fps;
- `/tmp/xwin.png` shows the game centred in an 800×600 screen at 4:3, with black borders.

## 8. Manual play on a real desktop

This hasn't been done yet. The steps below are for the laptop, using WSL2 with WSLg, which
provides both a display and sound; any Linux desktop works the same way.

```sh
# on the laptop
scp rotwang:kondor-port-src.tar.gz rotwang:re/kondor/kond-box.zip .
sudo apt install build-essential cmake libsdl2-dev unzip
mkdir -p ~/kondor && tar -xzf kondor-port-src.tar.gz -C ~/kondor
cd ~/kondor/kondor-port && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
unzip -q ~/kond-box.zip -d ~/kondor/data
cd ~/kondor/data/KONDOR && ~/kondor/kondor-port/build/kondor
```

Checklist:

| Check | Expected |
|---|---|
| boot screen | "FOUND AT IRQ: 5 DMA: 1"; "FAILED!" means SDL found no audio device |
| logo and menu | logo after a key press; menu with yellow highlight; ↑/↓ move it |
| sound | shots, explosions and pickups audible, no crackling or drift over a few minutes |
| game speed | about the same as DOSBox: the tank crosses the 256-px view in a few seconds |
| controls | Left Alt fires, arrows drive, Space brakes, Z/X turn the cannon, `[`/`]` pick an item, Enter uses it |
| Esc | back to the menu; RESUME continues the game |
| OPTIONS | redefine a key, quit, restart: the key binding is kept (`KONDOR.CFG`) |
| SCORES | finish or lose a game, enter a name: it shows up after a restart (`KONDOR.HSC`) |
| window | resizable, keeps 4:3; Alt+Enter toggles full screen; closing the window quits |
| terminal | no error messages |

## 9. Results on 2026-10-10

| Test | Result |
|---|---|
| 1. Build | Release and ASan builds, 0 warnings, 0 errors |
| 2. Smoke | pass; frames as described |
| 3. Menus | pass; options, scores and help as described |
| 4. Levels | 12/12 levels, 400 frames each, no sanitizer reports |
| 5. Random input | 0 sanitizer reports. This re-run: 12 seeds without sound (up to 20k frames) and 20 seeds with sound. Earlier development runs: about 35 sessions of up to 30k frames. Seeds 7–12 and 41–48 reached identical frame counts in separate runs, and the seed-3 replay reached the same 19,381 frames |
| 6. DOSBox reference | menu and level 1 match visually; driving into the water and sinking match (DOSBox gets there about 1 s later) |
| 7. Window | 700 frames in 10.3 s (68 fps including start-up), correct 4:3 placement |
| 8. Manual play | not done yet |

Bugs found while writing this guide, and fixed:

- `KONDOR_NOSOUND=0` used to *disable* sound, because only the presence of the variable was
  checked. Flags now count as off when empty or `0`.
- A `KONDOR_SCRIPT` path that couldn't be opened used to be ignored silently, which
  produced a run with no input. It is now a fatal error.
- The Release build had 15 warnings about unchecked `fread` results, which only appear at
  `-O2`. All reads now go through `fread_z()`.

Re-running the reconstruction pipeline itself (see `DECOMPILATION.md`) from an empty
directory reproduced the original counts: 32 types, 201 globals, 290 call fixes and 261
functions decompiled.
