"""Generate a C++ skeleton (types, globals, functions w/ locals, source-file map) from KONDOR.EXE TDInfo."""
import json, os, sys, bisect
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tdtypes import e, P
from kondor_apply_demangle import demangle

T = e.types; M = e.members
BASIC = {"SCHAR": "char", "SINT": "int", "SLONG": "long", "UCHAR": "unsigned char", "UINT": "unsigned",
         "ULONG": "unsigned long", "FLOAT": "float", "DOUBLE": "double", "LDOUBLE": "long double",
         "VOID": "void", "PCHAR": "char", "BOOL": "bool"}
ENUMS = {}
# enums: member groups in order; enum type records in order
enum_tis = [i for i, t in enumerate(T, 1) if str(t.id) == "ENUM"]
groups, cur = [], []
for i, m in enumerate(M, 1):
    if m.info in (0x00, 0x80) and (cur or True):
        pass
# find enum member groups: runs of members whose names are UPPER/true/false etc and types are small ints, ended by 0x80
i = 0
starts = {}
for ti in enum_tis:
    pass

def size(ti):
    t = T[ti - 1]; k = str(t.id)
    if k in ("STRUCT", "OBJECT", "ARRAY", "FAR", "ENUM") or k in BASIC: return t.size
    return t.size

# member groups: struct/class groups end with a 0xC0 record; enum groups end with info&0x80
GROUPS, cur = [], []
for idx, m in enumerate(M, 1):
    if m.info == 0xC0:
        GROUPS.append(("rec", cur)); cur = []; continue
    cur.append(idx)
    if m.info & 0x80:
        GROUPS.append(("enum", cur)); cur = []
CLASS_FIRST = {}
for kind, g in GROUPS:
    for idx in g:
        nm = P(M[idx - 1].name)
        if M[idx - 1].info & 0x08 and nm.startswith("@") and "@0" in nm:
            CLASS_FIRST[nm[1:nm.index("@0")]] = g[0]; break
ENUM_FIRST = {}
for ti_, (kind, g) in zip([i for i, t in enumerate(T, 1) if str(t.id) == "ENUM"], [x for x in GROUPS if x[0] == "enum"]):
    ENUM_FIRST[ti_] = g

def members(ti):
    t = T[ti - 1]; out = []
    j = (CLASS_FIRST[P(t.name)] if str(t.id) == "OBJECT" else t.member_type) - 1
    while j < len(M):
        m = M[j]
        if m.info == 0xC0: break
        out.append(m)
        if m.info & 0x80: break
        j += 1
    return out

def tname(ti, inner=""):
    if ti == 0: return ("void " + inner).strip()
    t = T[ti - 1]; k = str(t.id)
    if k in BASIC and not P(t.name): return (BASIC[k] + " " + inner).strip()
    if k == "FAR":
        sub = T[t.member_type - 1] if t.member_type else None
        if sub is not None and str(sub.id) == "FUNCTION":
            return tname(sub.member_type, f"(*{inner})()")
        return tname(t.member_type, "*" + inner)
    if k == "ARRAY":
        es = size(t.member_type) or 1
        n = t.size // es
        if inner.startswith("*"): inner = f"({inner})"
        return tname(t.member_type, f"{inner}[{n}]")
    if k in ("STRUCT", "OBJECT", "ENUM"):
        return (P(t.name) + " " + inner).strip()
    if k == "FUNCTION": return tname(t.member_type, f"{inner}()")
    return f"/*{k}#{ti}*/ {inner}"

out = []
# enums: match by following the member list for names after the enum record's companion
enum_members = []
grp = []
for m in M:
    if m.info == 0xC0: grp = []; continue
for ti in enum_tis:
    vals = ", ".join(f"{P(M[i-1].name)} = {M[i-1].type}" for i in ENUM_FIRST[ti])
    out.append(f"enum {P(T[ti-1].name)} {{ {vals} }};")
seen = set()
for ti, t in enumerate(T, 1):
    k = str(t.id)
    if k not in ("STRUCT", "OBJECT") or ti in seen: continue
    seen.add(ti)
    kw = "class" if k == "OBJECT" else "struct"
    out.append(f"{kw} {P(t.name)} {{   // size {t.size} (0x{t.size:x})")
    if k == "OBJECT": out.append("public:")
    off = 0
    for m in members(ti):
        if m.info & 0x08:   # method
            short, full = demangle(P(m.name))
            out.append(f"    /* method */ {full};")
            continue
        out.append(f"    {tname(m.type, P(m.name))};".ljust(48) + f"// +0x{off:03x}")
        off += size(m.type)
    if off != t.size: out.append(f"    // !! computed size {off} != {t.size}")
    out.append("};\n")
open(sys.argv[1] + "/types.h", "w").write("\n".join(out))

# globals and functions
CODESEG = e.segs[0].code_segment
gl, fn = [], []
syms = list(e.syms)
for idx, y in enumerate(syms[:e.h.globals_count + 2000]):
    pass
for y in syms:
    if str(y.bitfield.symbol_class) != "STATIC" or not y.index: continue
    nm = P(y.index)
    t = T[y.type - 1] if y.type else None
    addr = 0x10000 + y.segment * 16 + y.offset
    if t is not None and str(t.id) in ("FUNCTION",) or nm.startswith("@") and "$q" in nm:
        fn.append({"name": nm, "addr": addr, "ret": tname(t.member_type) if t is not None and str(t.id)=="FUNCTION" else "?"})
    elif y.segment == 0x15B2 - 0x1000 or y.segment >= 0x15B2 - 0x1000:
        gl.append({"name": nm, "off": addr - 0x25B20, "type": tname(y.type, nm), "size": size(y.type) if y.type else 0})
fn.sort(key=lambda f: f["addr"])
# scopes -> locals; scope offset is in code segment
fa = [f["addr"] for f in fn]
for s in e.scopes:
    base = s.offset if s.parent == 0 else e.scopes[s.parent - 1].offset
    a = 0x10000 + CODESEG * 16 + s.offset
    k = bisect.bisect_right(fa, a) - 1
    if k < 0: continue
    f = fn[k]
    for j in range(s.symbol_index - 1, s.symbol_index - 1 + s.symbol_count):
        y = syms[j]; c = str(y.bitfield.symbol_class)
        o = y.offset - 0x10000 if y.offset > 0x7FFF else y.offset
        f.setdefault("vars", []).append({"name": P(y.index), "class": c, "bp": o, "type": tname(y.type, P(y.index)),
                                          "scope": hex(a)})
# source files via line records
lines = list(e.lines)
for c in e.corrs:
    src_i, seg_i = c.seg, c.src   # field order swapped in our struct
    src = P(e.srcs[src_i - 1].name)
    for l in lines[c.line_index - 1: c.line_index - 1 + c.line_count]:
        a = 0x10000 + CODESEG * 16 + l.offset
        k = bisect.bisect_right(fa, a) - 1
        if k < 0: continue
        f = fn[k]
        f["src"] = src
        f["lmin"] = min(f.get("lmin", 1 << 30), l.line); f["lmax"] = max(f.get("lmax", 0), l.line)
for i, f in enumerate(fn):
    f["end"] = fn[i + 1]["addr"] if i + 1 < len(fn) else None
    f["short"], f["full"] = demangle(f["name"])
json.dump({"functions": fn, "globals": gl}, open(sys.argv[1] + "/skeleton.json", "w"), indent=1)
print(len(fn), "functions;", len(gl), "globals")
import collections
print(collections.Counter(f.get("src", "?") for f in fn))

# ---- machine-readable spec for the Ghidra script --------------------------------
SPECB = {"SCHAR": "char", "SINT": "int", "SLONG": "long", "UCHAR": "uchar", "UINT": "uint", "ULONG": "ulong",
         "FLOAT": "float", "DOUBLE": "double", "LDOUBLE": "ldouble", "VOID": "void", "PCHAR": "char", "BOOL": "uchar"}
def tspec(ti):
    if ti == 0: return "void"
    t = T[ti - 1]; k = str(t.id)
    if k in SPECB and not P(t.name): return SPECB[k]
    if k == "FAR":
        sub = T[t.member_type - 1] if t.member_type else None
        if sub is None or str(sub.id) in ("FUNCTION", "SPECIALFUNC"): return "ptr:void"
        return "ptr:" + tspec(t.member_type)
    if k == "ARRAY":
        es = size(t.member_type) or 1
        return f"arr:{t.size // es}:" + tspec(t.member_type)
    if k in ("STRUCT", "OBJECT"): return "struct:" + P(t.name)
    if k == "ENUM": return "enum:" + P(t.name)
    if k == "FUNCTION": return "void"
    return "uchar" if t.size == 1 else "int" if t.size == 2 else f"arr:{t.size}:uchar" if t.size else "void"

spec_types = []
for ti in enum_tis:
    spec_types.append({"kind": "enum", "name": P(T[ti-1].name), "size": 2,
                       "values": {P(M[i-1].name): M[i-1].type for i in ENUM_FIRST[ti]}})
done = set()
for ti, t in enumerate(T, 1):
    if str(t.id) not in ("STRUCT", "OBJECT") or P(t.name) in done: continue
    done.add(P(t.name)); off = 0; fields = []
    for m in members(ti):
        if m.info & 0x08: continue
        fields.append({"name": P(m.name), "off": off, "type": tspec(m.type)}); off += size(m.type)
    spec_types.append({"kind": "struct", "name": P(t.name), "size": t.size, "fields": fields})

ty_of = {}
for y in syms:
    if str(y.bitfield.symbol_class) == "STATIC" and y.index: ty_of[P(y.index)] = y.type
spec_fn = []
for f in fn:
    if f["addr"] >= 0x25B20: continue
    t = T[ty_of[f["name"]] - 1] if ty_of.get(f["name"]) else None
    ret = tspec(t.member_type) if t is not None and str(t.id) in ("FUNCTION", "SPECIALFUNC") else "void"
    params, locs, regs = [], [], []
    for v in f.get("vars", []):
        y_t = v["type"]
    for s in e.scopes:
        pass
    spec_fn.append({"addr": f["addr"], "name": f["short"], "full": f["full"], "mangled": f["name"], "ret": ret,
                    "src": f.get("src", "?"), "lines": [f.get("lmin"), f.get("lmax")], "vars": []})
# vars with spec types
fa2 = [f["addr"] for f in spec_fn]
for s in e.scopes:
    a = 0x10000 + CODESEG * 16 + s.offset
    k = bisect.bisect_right(fa2, a) - 1
    if k < 0: continue
    for j in range(s.symbol_index - 1, s.symbol_index - 1 + s.symbol_count):
        y = syms[j]; c = str(y.bitfield.symbol_class)
        o = y.offset - 0x10000 if y.offset > 0x7FFF else y.offset
        spec_fn[k]["vars"].append({"name": P(y.index), "class": c, "bp": o, "type": tspec(y.type),
                                   "scope": a, "scope_len": s.length})
spec_gl = [{"off": g["off"], "name": g["name"], "type": tspec(ty_of[g["name"]]) if ty_of.get(g["name"]) else "uchar"}
           for g in gl]
json.dump({"types": spec_types, "functions": spec_fn, "globals": spec_gl}, open(sys.argv[1] + "/spec.json", "w"), indent=1)
print("spec:", len(spec_types), "types", len(spec_fn), "functions", len(spec_gl), "globals")
