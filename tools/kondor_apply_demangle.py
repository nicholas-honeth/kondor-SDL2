import re
# --- minimal Borland C++ 3.x demangler (enough for this binary) -------------
BASIC = {"v": "void", "i": "int", "c": "char", "l": "long", "s": "short",
         "d": "double", "f": "float", "e": "...", "b": "bool"}

def parse_type(s, i, seen):
    mods = []
    while i < len(s) and s[i] in "unzx":            # unsigned / near ptr / signed / const
        mods.append(s[i]); i += 1
    if i < len(s) and s[i] == "t" and s[i + 1:i + 2].isdigit():   # back-reference t1..t9
        t = seen[int(s[i + 1]) - 1]; i += 2
    elif i < len(s) and s[i].isdigit():               # length-prefixed class name
        m = re.match(r"\d+", s[i:]); n = int(m.group()); j = i + len(m.group())
        t = s[j:j + n]; i = j + n
    else:
        t = BASIC.get(s[i], s[i]); i += 1
    for m in reversed(mods):
        t = {"u": "unsigned " + t, "z": "signed " + t, "x": "const " + t}.get(m, t + " *")
    return t, i

def demangle(sym):
    m = re.match(r"@(?:(\w+?)@0)?(\$?\w+?)\$q(.*)$", sym)
    if not m:
        return sym.lstrip("_"), sym.lstrip("_")
    cls, name, args = m.groups()
    if name == "$bctr":
        name = cls
    params, i, seen = [], 0, []
    while i < len(args):
        t, i = parse_type(args, i, seen)
        seen.append(t); params.append(t)
    if params[:1] == ["void"] and len(params) > 1:   # "$qve": interrupt handler (...)
        params = params[1:]
    plist = "" if params == ["void"] else ", ".join(params)
    full = f"{cls + '::' if cls else ''}{name}({plist})"
    short = f"{cls + '_' if cls else ''}{name}".replace("$", "")
    return short, full

