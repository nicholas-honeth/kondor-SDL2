"""Undo Borland/Microsoft 8087 emulation fixups in a DOS MZ image (static de-emulation).

The linker emitted INT 34h..3Dh in place of x87 instructions, adding the FIxRQQ /
FJxRQQ constants to the original bytes. At runtime the emulator either emulated
them or, with a real FPU present, patched them back. This does that patch ahead of
time so disassemblers see real x87 code. Same length, so no addresses move.

  CD 34..3B          -> 9B D8..DF          (FWAIT + ESC 0..7)      FIDRQQ = 0x5C32
  CD 3D              -> 90 9B              (NOP, FWAIT)            FIWRQQ = 0xA23D
  CD 3C xx           -> 9B <seg> ESC       segment override        FI[ECSA]RQQ, FJ[CSA]RQQ
      top 2 bits of xx: 11=ES, 00=DS, 01=SS, 10=CS; low 3 bits = ESC opcode

Usage: python3 unemu87.py in.exe out.exe code_end_paragraph
"""
import struct, sys
from collections import Counter

SEG_BY_TOP_BITS = {0b11: 0x26, 0b00: 0x3E, 0b01: 0x36, 0b10: 0x2E}  # ES, DS, SS, CS

src, dst, code_end_para = sys.argv[1], sys.argv[2], int(sys.argv[3], 0)
b = bytearray(open(src, "rb").read())
hdr = struct.unpack_from("<H", b, 8)[0] * 16
code_end = hdr + code_end_para * 16          # file offset where code segments stop
stats = Counter()
i = hdr
while i < code_end - 1:
    if b[i] == 0xCD and 0x34 <= b[i + 1] <= 0x3D:
        n = b[i + 1]
        if n <= 0x3B:
            b[i], b[i + 1] = 0x9B, 0xD8 + (n - 0x34)
            stats[f"INT {n:02X}h -> FWAIT ESC{n - 0x34}"] += 1
            i += 2
            continue
        if n == 0x3D:
            b[i], b[i + 1] = 0x90, 0x9B
            stats["INT 3Dh -> NOP FWAIT"] += 1
            i += 2
            continue
        x = b[i + 2]                          # n == 0x3C
        b[i], b[i + 1], b[i + 2] = 0x9B, SEG_BY_TOP_BITS[x >> 6], 0xD8 | (x & 0x07)
        stats[f"INT 3Ch -> FWAIT {('ES','DS','SS','CS')[[0b11,0b00,0b01,0b10].index(x >> 6)]}: ESC"] += 1
        i += 3
        continue
    i += 1
open(dst, "wb").write(b)
for k, v in sorted(stats.items()):
    print(f"{v:5}  {k}")
print(f"{sum(stats.values()):5}  total patched (code bytes {hdr:#x}..{code_end:#x})")
