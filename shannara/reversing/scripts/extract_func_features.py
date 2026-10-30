"""
IDAPython: dumps per-function matching features from an open IDB to JSON, for comparing two builds of the same source
(e.g. retail Shannara, 32-bit Watcom, against Companions of Xanth, 16-bit Microsoft C, see match_engines.py):

  ea, name, size, nblocks, callees (function start eas), n_callers, strings (literals referenced), consts (immediates >= 0x20),
  calls (ordered list of callee eas incl. duplicates), data (ordered non-string data addresses referenced) with data_names

    idat.exe -A -S"extract_func_features.py out.json" copy.idb       (relative out path; works for 16/32-bit IDBs)
"""
import json
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_gdl
import ida_ua
import ida_kernwin

out = idc.ARGV[1] if len(idc.ARGV) > 1 else "features.json"
funcs = {}
starts = set(idautils.Functions())
callers = {}


def strlit(ea):
    try:
        if ida_bytes.is_strlit(ida_bytes.get_flags(ea)):
            s = idc.get_strlit_contents(ea, -1, 0)
            if s:
                return s.decode("latin-1")
    except Exception:
        pass
    return None


for f in starts:
    fn = ida_funcs.get_func(f)
    size = fn.end_ea - fn.start_ea
    strings, consts, calls, data = [], set(), [], []
    ea = fn.start_ea
    while ea < fn.end_ea:
        insn = ida_ua.insn_t()
        ln = ida_ua.decode_insn(insn, ea)
        if ln == 0:
            ea += 1
            continue
        mnem = insn.get_canon_mnem()
        for op in insn.ops:
            if op.type == ida_ua.o_void:
                break
            if op.type == ida_ua.o_imm and op.value >= 0x20 and op.value not in (0xFFFF, 0xFFFFFFFF, 0xFF):
                consts.add(op.value & 0xFFFFFFFF)
        for r in idautils.DataRefsFrom(ea):
            s = strlit(r)
            if s:
                if s not in strings:
                    strings.append(s)
            elif r not in data and not ida_funcs.get_func(r):
                data.append(r)
        if mnem in ("call", "callf", "callfi", "calln"):
            for r in idautils.CodeRefsFrom(ea, 0):
                cf = ida_funcs.get_func(r)
                if cf and cf.start_ea == r:
                    calls.append(r)
                    callers.setdefault(r, set()).add(f)
        ea += ln
    nb = 0
    try:
        nb = sum(1 for _ in ida_gdl.FlowChart(fn))
    except Exception:
        pass
    funcs[f] = dict(ea=f, name=idc.get_func_name(f), size=size, nblocks=nb, strings=strings, consts=sorted(consts)[:40], calls=calls, data=data, data_names=[idc.get_name(d) for d in data])
for f, d in funcs.items():
    d["n_callers"] = len(callers.get(f, ()))
json.dump(list(funcs.values()), open(out, "w"))
if not ida_kernwin.is_idaq():
    idc.qexit(0)
