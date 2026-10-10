# How Kondor was decompiled

This is a record of how the C++ source in `src/` was recovered from the shipped DOS
executable. It covers the starting point, the tools, each step with its commands, the
problems found along the way and how they were solved, and how the recovered code was
turned into source by hand.

All work was done on the Rotwang sandbox VM (Ubuntu 24.04, Ghidra 12.1.4, OpenJDK 21,
Python 3.12). The commands below can be re-run from an empty directory. They were re-run
that way on 2026-10-10 and gave the same counts as the original work.

## 1. Starting point

| | |
|---|---|
| Archive | `kond-box.zip` from the DOS Games Archive, sha256 `86117edc794d6c9adb435d4c809331dde3d39d60b15cdbbf9d3f3bfd10e4a418` |
| Binary | `KONDOR/KONDOR.EXE`, 221,323 bytes, sha256 `231079fed9609f29b067c2b24fcce84d9063c7b9bd80240883f6ed8e84fa935b` |
| Compiler | Borland C++ 3.x (runtime © 1991), 16-bit real mode, large memory model (far code and data, 1,103 relocations) |
| Debug info | Turbo Debugger information (TDInfo v3.16, magic `0xFB52`) appended after the 159,312-byte load image |

The TDInfo block was the key to the whole project. It contains:

- 1,902 symbols, including mangled C++ names with parameter types (for example
  `@findtheway$qiiiin10pointtypeini` and `@tank_type@0handle$qv`);
- 528 type records and the member lists of all 31 structs and classes, with exact sizes;
- the names, types and stack-frame offsets of every parameter and local variable;
- 3,332 line-number records that tie code addresses to lines in named source files.

The source file table showed that the program was one translation unit: `KONDOR.CPP`,
which `#include`s 28 headers containing the actual code (QUNIT.H, PAL.H, FONT.H, SOUND.H,
ALLKEYS.H, HELP.H, OPTIONS.H, HIGHSCR.H, COLL.H, KGRAPH.H, LAYERS.H, SLOTS.H, SCROLL.H,
BULLETS.H, AI.H, EXPL.H, ENEMIES.H, OBJECTS.H, PTANK.H, FRAMES.H, BONUS.H, LOADMAP.H,
AIRFORCE.H, BASES.H, END.H, MISC.H, GAME.H, MENU.H). The port keeps that layout.

Memory layout of the image (needed by several tools):

- the load image starts at file offset `0x1600` (header size from the MZ header);
- game code runs from linear `0x14880` (segment `0x1488`), and the Borland runtime and
  other code segments follow up to `0x25B20`;
- DGROUP, the data segment, starts at linear `0x25B20` (segment `0x25B2`). In the file
  that is `0x1600 + 0x15B20`, which the port uses to read initialised data from the EXE.

Linear address = `0x10000 + segment*16 + offset`, because Ghidra loads the image at
`1000:0000`.

## 2. Tools and setup

```sh
# On Ubuntu 24.04
sudo apt install openjdk-21-jdk python3-venv binutils unzip
# Ghidra 12.1.4 in /opt/ghidra, with GHIDRA_INSTALL_DIR and JAVA_HOME set
# (on Rotwang: source /etc/profile.d/rea.sh)

# TDInfo record definitions: the standalone tdinfo_structs.py from
# ramikg/tdinfo-parser (public domain), plus the construct library
git clone https://github.com/ramikg/tdinfo-parser ~/re/tdinfo-parser
python3 -m venv ~/re/venv && ~/re/venv/bin/pip install construct==2.10.70
```

The scripts are in `tools/`:

| Script | Purpose |
|---|---|
| `unemu87.py` | undoes the 8087-emulator fix-ups so that floating-point code disassembles correctly |
| `tdtypes.py` | parses the whole TDInfo block (symbols, modules, sources, line numbers, scopes, types, members, names) |
| `kondor_apply_demangle.py` | a small Borland C++ 3.x name demangler |
| `tdc.py` | turns the TDInfo into `types.h` (a C++ declaration of every type) and `spec.json` (types, globals, functions with source file, line range, parameters and locals) |
| `KondorApply.java` | a headless Ghidra script that applies `spec.json` to the program and decompiles every game function |
| `annasm.py` | writes annotated Intel-syntax assembly for every game function (uses `objdump -m i8086`) |

`tdtypes.py` finds `tdinfo_structs.py` through `$TDINFO_PARSER` (default
`~/re/tdinfo-parser`) and reads the EXE named in `$KEXE` (default `KONDOR.EXE` in the
current directory).

## 3. Step by step

### 3.1 Unpack, and undo the 8087 emulation

```sh
mkdir -p ~/kdoc && cd ~/kdoc
unzip -q /path/to/kond-box.zip -d game
mkdir work && cd work
python3 ~/kondor-port/tools/unemu87.py ../game/KONDOR/KONDOR.EXE KONDOR.EXE 0x15b2
#  ...
#   413  INT 3Ch -> FWAIT ES: ESC
#   397  INT 3Dh -> NOP FWAIT
#  2385  total patched (code bytes 0x1600..0x17120)
```

**Why this step exists.** A first Ghidra pass on the original binary was useless for every
function that used `double`. The output was full of `swi(0x34)` … `swi(0x3d)` calls and
`halt_baddata()`, and two functions hung Ghidra's analysis for up to 38 minutes.

The cause is Borland's floating-point emulator. Programs could run on PCs without an 8087
coprocessor, so the linker replaced each x87 instruction with a software interrupt,
`INT 34h` to `INT 3Dh`. It did this by *adding* fix-up constants to the opcode bytes. At run
time the emulator either emulated the instruction or, if a coprocessor was present,
patched the bytes back into the real FPU opcode. Ghidra only ever sees the interrupts.

The fix-up constants were in the TDInfo itself (`FIDRQQ=0x5C32`, `FIWRQQ=0xA23D`,
`FIERQQ`, `FICRQQ`, `FISRQQ`, `FIARQQ`, `FJARQQ`, `FJSRQQ`, `FJCRQQ`). Subtracting them
gives the rewrite rules:

| Emulated | Real |
|---|---|
| `CD 34`…`CD 3B` | `9B D8`…`9B DF` (FWAIT + ESC 0–7) |
| `CD 3D` | `90 9B` (NOP, FWAIT) |
| `CD 3C xx` | `9B <seg> D8+(xx&7)`; the top two bits of `xx` select the segment override (11 = ES, 00 = DS, 01 = SS, 10 = CS) |

The replacement has the same length, so no address moves. The patch is limited to the code
segments (image paragraphs below `0x15B2`, where DGROUP begins), so data bytes that happen
to look like `CD 3x` are left alone. All 413 `INT 3Ch` sites were ES overrides, as expected
in the large memory model.

After the patch, the second Ghidra pass named 248 of 248 functions in under four minutes.
`tank_type::handle()` went from 123 `swi()` calls and 742 lines to 0 calls and 130 lines.

### 3.2 Parse the debug information

```sh
KEXE=KONDOR.EXE ~/re/venv/bin/python ~/kondor-port/tools/tdc.py .
# 301 functions; 206 globals
# Counter({'QUNIT.H': 42, '?': 41, 'SCROLL.H': 33, 'SOUND.H': 20, ...})
# spec: 32 types 301 functions 206 globals
```

Outputs: `types.h`, `skeleton.json` and `spec.json`.

What `tdc.py` does:

- **Functions and source files.** It joins the symbol table, the scope records (which list
  each function's parameters and locals with their `bp` offsets or registers) and the
  line-number correlation records. That gives every function a source file and a line
  range, for example `findtheway` in AI.H. The 41 functions marked `?` are runtime-library
  or assembly routines without line numbers.
- **Types.** Struct and class member lists are linked through "member groups". For
  classes, the group is found through the mangled names of the class's methods; for enums,
  through a terminating flag. All 31 structs and classes come out at exactly their recorded
  sizes with byte packing (for example `loadenemy_type` = 147 bytes, `enemy_type` = 414),
  which confirms that the member offsets are right.
- **Names.** `kondor_apply_demangle.py` demangles Borland names:
  `@cls@0meth$q<args>` becomes `cls::meth(args)`, where `n` = pointer, `u` = unsigned,
  `z` = signed, `t1`…`t9` = back-reference, `$bctr` = constructor and `$qve` = interrupt
  handler.

Two parser quirks were found on the way. The field order of the correlation record in
`tdinfo_structs.py` has `src` and `seg` swapped. And the programmer's point type really is
called `pointtypei`, so that is not a demangler bug.

### 3.3 Apply everything in Ghidra and decompile

```sh
source /etc/profile.d/rea.sh     # GHIDRA_INSTALL_DIR, JAVA_HOME
mkdir -p ghidra decomp
/opt/ghidra/support/analyzeHeadless ~/kdoc/work/ghidra kondor \
    -import KONDOR.EXE -overwrite \
    -scriptPath ~/kondor-port/tools -postScript KondorApply.java \
    ~/kdoc/work/spec.json ~/kdoc/work/decomp > ghidra.log 2>&1
grep "KondorApply.java>" ghidra.log
#   types applied: 32
#   globals applied: 201
#   near calls overridden: 290
#   functions: 301
#   decompiled ok=261 failed=0
```

This takes about 4.5 minutes, most of it Ghidra's auto-analysis. Once the project exists,
the script can be re-run without re-analysing:

```sh
/opt/ghidra/support/analyzeHeadless ~/kdoc/work/ghidra kondor -process KONDOR.EXE \
    -noanalysis -readOnly -scriptPath ~/kondor-port/tools -postScript KondorApply.java \
    ~/kdoc/work/spec.json ~/kdoc/work/decomp
```

The script runs five stages:

1. **Types.** It creates all 32 types first and fills them in afterwards, so forward
   references resolve. Borland `int` is mapped to a 16-bit `short` and `long` to 32 bits,
   whatever Ghidra's data organisation says.
2. **Globals.** It types and names 201 globals in DGROUP. It first runs `clearListing`
   over DGROUP, because auto-analysis had disassembled parts of the data segment as code,
   which made `createData` fail with "Not enough space".
3. **Near-call fix.** Borland emits `push cs; call near` for calls between functions in
   the same code segment, which act as far calls. Ghidra computes the 16-bit relative
   target wrongly across the 64 KB offset boundary, so these calls pointed at wrong or
   nonexistent functions. The script recomputes all 290 targets and attaches
   `CALL_OVERRIDE_UNCONDITIONAL` references, which the decompiler honours.
4. **Functions.** It creates any missing functions; the interrupt handlers such as
   `key_interrupt` and `ServiceIRQ*` are only reached through the interrupt vector table,
   so auto-analysis never finds them. It names every function inside its class namespace,
   sets the `__cdecl16far` signature and names the stack locals.
5. **Decompile.** It decompiles all 261 game functions and writes them grouped by original
   source file: `decomp/QUNIT.H.c`, `decomp/ENEMIES.H.c` and so on.

The segment mapping in the script (`CODESEG 0x1488`, `DATASEG 0x25B2`, the extra code
segments at `0x22DD` and `0x2555`) came from listing Ghidra's memory blocks.

**Why not REA?** The REA MCP server was used for the first exploration and for naming.
But it can't set types, its edits only last for one session, and the CLI re-runs analysis
(about 2.5 minutes) for every call. For bulk work, a headless Ghidra script was far
faster and repeatable.

### 3.4 Annotated assembly

```sh
python3 ~/kondor-port/tools/annasm.py KONDOR.EXE spec.json asm
ls asm   # one <SOURCE>.asm per original file
```

**Why this was needed.** Ghidra's decompiler loses track of the x87 register stack
across calls, so floating-point expressions in the drafts were often wrong or meaningless.
The clearest case was `dist()`. The decompiled version was gibberish; the assembly shows it
returns the *squared* distance, or 100000.0 when the player's tank is under water
(`tank.height < 0`). The game's thresholds (9025 = 95², 18225, 2500, 4900) only make sense
once you know that.

`annasm.py` disassembles each function with `objdump -m i8086 -M intel` and adds comments:

- `[bp+N]` and `[bp-N]` become the original parameter and local names;
- DGROUP addresses become `global.member[i].path`, using the recovered struct layouts;
- `les bx,[this]` followed by `es:[bx+N]` becomes `this->member`;
- float and double constants in DGROUP are shown as values;
- far calls and `push cs; call near` calls show the callee's name;
- strings are shown inline. Some strings, such as `"QSOFT(C), 99"`, are stored with bit 7
  set as light obfuscation; those are shown with a `~` prefix.

For reading, a filtered copy without prologue and epilogue noise (`nop`, `fwait`,
`push bp` and so on) was used.

### 3.5 Switch statements

Ghidra didn't recover several `switch` jump tables:

- the key-name switches in `getkeyname` (ALLKEYS.H);
- the eight-direction switches of the path search in `findtheway` (AI.H);
- the menu's draw, up, down and enter switches (MENU.H);
- one sparse switch that compiles to a value table plus a target table.

Five small throwaway Python scripts decoded these tables. Each read the 16-bit offsets at
the address used by the indirect `jmp cs:[bx+…]` and printed the case targets, which were
then matched to the annotated assembly. They are not in `tools/` because each one was
specific to a single table.

## 4. From drafts to source

Each original header was rewritten by hand into `src/<NAME>.H`, in the original order and
with the original function and variable names. For each function, the inputs were:

1. the Ghidra C draft, for structure: loops, conditions and calls;
2. the annotated assembly, for anything involving floating point, for exact integer
   widths, signedness and shifts, and to check the draft wherever it looked doubtful;
3. the TDInfo line range, to keep functions in their original order within each file.

Typical corrections to the drafts:

- floating-point data flow across calls;
- 16-bit arithmetic: the original's `int` was 16 bits, so expressions that could
  overflow were checked one by one (only `kincircle` actually depended on it);
- Borland evaluated function arguments right to left. Where two arguments both call
  `random()`, the order of the calls decides the result, so those calls were split into
  separate statements in the original order;
- `FTOL@`, the runtime's float-to-integer conversion, truncates toward zero;
- 8-bit and 16-bit wrap-around, which was made explicit where the game relies on it (for
  example when mixing sound samples).

Small items from the initialised data segment, such as the default key bindings,
`gcol`, `bullet_max_age` and the Sound Blaster base port, became initialisers in
`KONDOR.CPP`. A throwaway script found them by dumping DGROUP with the recovered global
names.
(The cos/sin tables are computed at start-up by `maketbls()`, as in the original.) Large data
blocks that belong to the game, such as the help text and a few bitmaps, are **not** copied
into the source. They are read from the player's `KONDOR.EXE` at run time; see `PORTING.md`.

Facts about file formats, the sound engine and the random number generator were confirmed
by checking the recovered code against the shipped data (map files, `KONDOR.CFG`,
`KONDOR.HSC`) and against the original running in DOSBox. See `TESTING.md`.

## 5. What was learned

- **Leftover debug information changes everything.** Names, types and source layout made
  this a translation job rather than a guessing game. Without TDInfo, the same work would
  have taken many times longer.
- **Check the floating-point path first in any Borland or Microsoft DOS binary.** If
  the decompiler shows `INT 34h`–`3Dh`, de-emulate before doing anything else.
- **Don't trust decompiled x87 code.** Read the assembly for every floating-point
  expression.
- **Struct sizes are a free correctness check.** If every recovered struct comes out at
  its recorded size, the member offsets are almost certainly right.
- **Decompiling forces decisions about bugs.** Several original bugs were real memory
  errors, others were harmless quirks of the game. `PORTING.md` lists which were fixed and
  which were kept.
