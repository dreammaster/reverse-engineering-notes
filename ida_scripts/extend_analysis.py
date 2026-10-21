"""
Turns unanalysed code reachable through xrefs (jump table entries, calls from data areas) into functions.
Run against a copy of the database (ext/WL_ext.idb): it saves its changes.
"""
import idc
import ida_bytes
import ida_ua
import ida_funcs
import ida_auto
import idautils

LO, HI = 0x10000, 0x1CB67


def covered():
    spans = []
    for ea in idautils.Functions(LO, HI):
        f = ida_funcs.get_func(ea)
        spans.append((f.start_ea, f.end_ea))
        # chunks
        it = ida_funcs.func_tail_iterator_t(f)
        ok = it.main()
        while ok:
            c = it.chunk()
            spans.append((c.start_ea, c.end_ea))
            ok = it.next()
    spans.sort()
    return spans


def gaps():
    out = []
    end = LO
    for a, b in covered():
        if a > end:
            out.append((end, a))
        end = max(end, b)
    if end < HI:
        out.append((end, HI))
    return out


added = 0
for rnd in range(6):
    g = gaps()
    seeds = []
    for a, b in g:
        for ea in range(a, b):
            refs = [x for x in idautils.XrefsTo(ea, 0) if x.type in ((16, 17, 18, 19, 20, 21)) or x.type in (1, 2, 3, 4, 5)]
            if refs:
                seeds.append(ea)
    new = 0
    for ea in seeds:
        if ida_funcs.get_func(ea):
            continue
        if not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            ida_bytes.del_items(ea, 0, 1)
            if not ida_ua.create_insn(ea):
                continue
        if ida_funcs.add_func(ea):
            new += 1
    ida_auto.auto_wait()
    added += new
    print("round", rnd, "seeds", len(seeds), "new functions", new)
    if new == 0:
        break
tot = sum(b - a for a, b in gaps())
print("remaining gap bytes", tot, "functions added", added)
