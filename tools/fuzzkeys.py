"""Random key script for KONDOR_SCRIPT: mostly game keys, sometimes menu/escape keys."""
import random, sys
seed, frames = int(sys.argv[1]), int(sys.argv[2])
rng = random.Random(seed)
game = [0x38, 0x39, 0xC8, 0xD0, 0xCB, 0xCD, 0x1C, 0x1A, 0x1B, 0x2C, 0x2D]
menu = [0xC8, 0x1C, 0x1C, 0x01, 0x39, 0x0E, 0x1E, 0x30]   # no "down": rarely reaches EXIT
held = {}
f = 30
out = []
while f < frames:
    pool = menu if rng.random() < 0.25 else game
    k = rng.choice(pool)
    dur = rng.randint(1, 90)
    out.append((f, "down", k))
    out.append((f + dur, "up", k))
    f += rng.randint(1, 25)
for fr, w, k in sorted(out):
    print(fr, w, f"{k:02x}")
