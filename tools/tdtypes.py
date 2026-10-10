"""Explore Borland TDInfo type/member/scope records of KONDOR.EXE."""
import os, sys, collections
# tdinfo_structs.py from https://github.com/ramikg/tdinfo-parser
sys.path.insert(0, os.environ.get("TDINFO_PARSER", os.path.expanduser("~/re/tdinfo-parser")))
from construct import Struct, Int16ul, Padding, Seek, CString, this, Const, Tell
import tdinfo_structs as T

LINE = Struct('line' / Int16ul, 'offset' / Int16ul)
SRC = Struct('name' / Int16ul, 'date' / Int16ul[2])
CORR = Struct('src' / Int16ul, 'seg' / Int16ul, 'line_index' / Int16ul, 'line_count' / Int16ul)
EXE = Struct(
    'signature' / Const(b'MZ'),
    'used_bytes_in_last_page' / Int16ul, 'file_size_in_pages' / Int16ul,
    'o' / Tell,
    Padding(lambda c: T.calculate_extra_information_offset(c) - c.o),
    'h' / T.TDINFO_HEADER_STRUCT,
    'syms' / T.SYMBOL_RECORD_STRUCT[this.h.symbols_count],
    'mods' / T.MODULE_RECORD_STRUCT[this.h.modules_count],
    'srcs' / SRC[this.h.source_files_count],
    'lines' / LINE[this.h.line_numbers_count],
    'scopes' / T.SCOPE_RECORD_STRUCT[this.h.scopes_count],
    'segs' / T.SEGMENT_RECORD_STRUCT[this.h.segments_count],
    'corrs' / CORR[this.h.correlations_count],
    'types' / T.TYPE_RECORD_STRUCT[this.h.types_count],
    'members' / T.MEMBER_RECORD_STRUCT[this.h.members_count],
    Seek(-this.h.names_pool_size_in_bytes, whence=2),
    'pool' / CString('ascii')[this.h.names_count],
)
import os
e = EXE.parse_file(os.environ.get("KEXE", "KONDOR.EXE"))
P = lambda i: (str(e.pool[i - 1]) if i <= len(e.pool) else f"#{i}") if i else ""
if __name__ == "__main__":
    print(e.h)
    print("segs", [(s.code_segment, s.code_offset, s.code_length, s.scope_index, s.scope_count) for s in e.segs])
    print("srcs", [P(s.name) for s in e.srcs])
    print("corrs", list(e.corrs)[:5])
    c = collections.Counter(str(t.id) for t in e.types); print(c)
    for i, t in enumerate(e.types[:200], 1):
        print(i, t.id, P(t.name), t.size, t.class_type, t.member_type)
    for i, m in enumerate(e.members[:289], 1):
        print("m", i, hex(m.info), P(m.name), m.type)
