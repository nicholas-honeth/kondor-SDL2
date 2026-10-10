"""Annotated Intel-syntax disassembly of every KONDOR.EXE game function.

Uses the FPU-de-emulated image plus spec.json (from tdc.py):
  [bp+N]/[bp-N]      -> original parameter / local names
  DGROUP absolutes   -> global symbol (+ struct member path)
  float constants    -> their float32/float64 value
  far / push-cs near calls -> callee names
Output: one <SRC>.asm per original source file.
Usage: python3 annasm.py fpu/KONDOR.EXE port-gen/spec.json outdir
"""
import json, os, re, struct, subprocess, sys, bisect, tempfile

exe, specp, outdir = sys.argv[1:4]
B = open(exe, "rb").read()
HDR = struct.unpack_from("<H", B, 8)[0] * 16
DG = 0x25B20  # DGROUP linear
spec = json.load(open(specp))
os.makedirs(outdir, exist_ok=True)

TYPES = {t["name"]: t for t in spec["types"]}
BASIC = {"char": 1, "uchar": 1, "int": 2, "uint": 2, "long": 4, "ulong": 4, "float": 4, "double": 8, "ldouble": 10}

def tsize(t):
    if t in BASIC: return BASIC[t]
    if t.startswith("ptr:"): return 4
    if t.startswith("arr:"):
        _, n, e = t.split(":", 2); return int(n) * tsize(e)
    if t.startswith("struct:"): return TYPES[t[7:]]["size"]
    if t.startswith("enum:"): return 2
    return 1

def member_path(t, off):
    """Describe byte offset `off` inside type t as .field[idx].field..."""
    if off == 0 and (t in BASIC or t.startswith(("ptr:", "enum:"))): return ""
    if t.startswith("arr:"):
        _, n, e = t.split(":", 2); es = tsize(e)
        i, r = divmod(off, es)
        return f"[{i}]" + member_path(e, r)
    if t.startswith("struct:"):
        for f in TYPES[t[7:]]["fields"]:
            fs = tsize(f["type"])
            if f["off"] <= off < f["off"] + fs:
                return "." + f["name"] + member_path(f["type"], off - f["off"])
    return f"+{off}" if off else ""

GL = sorted((g["off"], g["name"], g["type"]) for g in spec["globals"] if g["off"] >= 0)
GOFF = [g[0] for g in GL]
def dg_name(off):
    k = bisect.bisect_right(GOFF, off) - 1
    if k >= 0:
        o, n, t = GL[k]
        if off < o + max(tsize(t), 1):
            return n + member_path(t, off - o)
    return None

def dg_const(off, width):
    a = HDR + (DG - 0x10000) + off
    if width == 4: return struct.unpack_from("<f", B, a)[0]
    if width == 8: return struct.unpack_from("<d", B, a)[0]
    if width == 10:
        m, e = struct.unpack_from("<QH", B, a)
        s = -1 if e & 0x8000 else 1; e &= 0x7FFF
        return s * m / (1 << 63) * 2.0 ** (e - 16383) if e else 0.0
    return struct.unpack_from("<h", B, a)[0]

def dg_string(off):
    a = HDR + (DG - 0x10000) + off
    s = B[a:a + 80].split(b"\0")[0]
    if not s: return None
    if all(32 <= c < 127 for c in s): return s.decode("latin1")
    if all(32 <= (c & 0x7F) < 127 for c in s) and sum(c >= 0x80 for c in s) * 2 >= len(s):
        return "~" + bytes(c & 0x7F for c in s).decode("latin1")   # ~ = stored with bit 7 set
    return None

FN = sorted(spec["functions"], key=lambda f: f["addr"])
ADDR2NAME = {f["addr"]: f["full"] for f in FN}
GAME = [f for f in FN if 0x14880 <= f["addr"] < 0x22DD0]
ends = {f["addr"]: (FN[i + 1]["addr"] if i + 1 < len(FN) else f["addr"] + 0x200) for i, f in enumerate(FN)}

def disasm(lin_start, lin_end):
    data = B[HDR + lin_start - 0x10000: HDR + lin_end - 0x10000]
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as t:
        t.write(data); path = t.name
    out = subprocess.run(["objdump", "-D", "-b", "binary", "-m", "i8086", "-M", "intel",
                          f"--adjust-vma={lin_start - 0x14880:#x}", path],
                         capture_output=True, text=True).stdout
    os.unlink(path)
    res = []
    for ln in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*\t(.*)$", ln)
        if m: res.append((int(m.group(1), 16), m.group(2).replace(" ", ""), m.group(3)))
        else:
            m = re.match(r"\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*$", ln)
            if m and res:   # continuation bytes
                o, b, t = res[-1]; res[-1] = (o, b + m.group(2).replace(" ", ""), t)
    return res

WIDTH = {"BYTE": 1, "WORD": 2, "DWORD": 4, "QWORD": 8, "TBYTE": 10}

def annotate(f):
    vars_bp = {}
    regs = []
    for v in f["vars"]:
        if v["class"] == "AUTO": vars_bp.setdefault(v["bp"], (v["name"], v["type"]))
        elif v["class"] == "REGISTER": regs.append(f"{v['name']}={'SI' if v['bp']==6 else 'DI' if v['bp']==7 else v['bp']}")
    lines = [f"; ==== {f['full']}   @1488:{f['addr']-0x14880:04x}  {f['src']} lines {f['lines']}"]
    if vars_bp:
        lines.append("; frame: " + ", ".join(f"[bp{bp:+#x}]={n}:{t}" for bp, (n, t) in sorted(vars_bp.items())))
    if regs: lines.append("; register vars: " + ", ".join(regs))
    esbx = None   # (varname, struct type) when ES:BX was loaded from a typed far pointer
    for off, raw, text in disasm(f["addr"], ends[f["addr"]]):
        notes = []
        ml = re.match(r"les\s+bx,DWORD PTR \[bp([+-])(0x[0-9a-f]+)\]", text)
        if ml:
            o = int(ml.group(2), 16) * (1 if ml.group(1) == "+" else -1)
            v = vars_bp.get(o)
            esbx = (v[0], v[1][4:]) if v and v[1].startswith("ptr:struct:") else None
        elif re.match(r"(les\s+bx|mov\s+bx|pop\s+bx|add\s+bx|xchg\s+bx|lea\s+bx|mov\s+es|pop\s+es)", text) or text.startswith(("call", "lcall")):
            esbx = None
        if esbx:
            for m in re.finditer(r"es:\[bx(?:\+(0x[0-9a-f]+))?\]", text):
                o = int(m.group(1), 16) if m.group(1) else 0
                notes.append(f"{esbx[0]}->{member_path(esbx[1], o).lstrip('.')}")
        # stack vars
        def bpsub(m):
            sign, num = m.group(1), int(m.group(2), 16)
            o = num if sign == "+" else -num
            for bp, (n, t) in vars_bp.items():
                if bp <= o < bp + max(tsize(t), 2):
                    return f"[{n}{member_path(t, o - bp) if o != bp else ''}]"
            return m.group(0)
        text = re.sub(r"\[bp([+-])(0x[0-9a-f]+)\]", bpsub, text)
        # DGROUP absolute operands  ds:0x1234 or [0x1234]  (code assumes DS=DGROUP)
        mm = re.search(r"(BYTE|WORD|DWORD|QWORD|TBYTE) PTR (?:ds:|es:|ss:|cs:)?(?:\[)?(0x[0-9a-f]+)\]?", text)
        seg_es = " es:" in text or "PTR es:" in text
        if mm and not seg_es and "bp" not in text:
            o = int(mm.group(2), 16); w = WIDTH[mm.group(1)]
            n = dg_name(o)
            if n: notes.append(n)
            elif text.split()[0].startswith("f"):
                notes.append(f"const {dg_const(o, w)!r}")
            else:
                notes.append(f"DG:{o:#x}")
        # array/indexed DGROUP refs like [bx+0x547]
        for m in re.finditer(r"\[(?:bx|si|di)(?:\+(?:si|di))?\+(0x[0-9a-f]+)\]", text):
            o = int(m.group(1), 16)
            if o >= 0x100 and not seg_es:
                n = dg_name(o)
                if n: notes.append(f"{m.group(1)}={n}")
        # immediates that look like DGROUP strings (push 0x1234 before push ds)
        mi = re.match(r"(push|mov)\s+(?:\w+,)?(0x[0-9a-f]+)$", text)
        if mi:
            o = int(mi.group(2), 16)
            s = dg_string(o) if 0x100 <= o < 0xF470 else None
            if s and len(s) >= 2: notes.append(f'"{s}"')
            elif 0x100 <= o < 0xF470 and dg_name(o) and o in GOFF: notes.append(f"&{dg_name(o)}?")
        # calls
        rb = bytes.fromhex(raw)
        if rb[:1] == b"\x9a":
            o, s = struct.unpack_from("<HH", rb, 1)
            tgt = 0x10000 + s * 16 + o
            notes.append("call " + ADDR2NAME.get(tgt, f"?{tgt:#x}"))
        elif rb[:1] == b"\xe8":
            rel = struct.unpack_from("<h", rb, 1)[0]
            tgt = 0x14880 + ((off + 3 + rel) & 0xFFFF)
            notes.append("call " + ADDR2NAME.get(tgt, f"?{tgt:#x}"))
        lines.append(f"  {off:04x}: {text:<52}" + ("; " + " | ".join(notes) if notes else ""))
    return "\n".join(lines) + "\n"

by_src = {}
for f in GAME:
    by_src.setdefault(f["src"].replace("F:\\TC\\INCLUDE\\", ""), []).append(annotate(f))
for src, parts in by_src.items():
    open(os.path.join(outdir, src + ".asm"), "w").write("\n".join(parts))
print({k: len(v) for k, v in by_src.items()})
