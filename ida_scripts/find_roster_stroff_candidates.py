"""
Read-only candidate finder for wiring the RosterEntry struct into real
instruction operands (op_stroff), per the roadmap's "mechanical
follow-up" item. Rather than guessing which functions touch
RosterEntry pointers, this scans every instruction in the IDB for a
`[reg+N]`-style displacement operand where N exactly matches one of
RosterEntry's own member offsets, and groups the hits by function.

Confidence signal: a function that touches MULTIPLE distinct
RosterEntry offsets is very likely genuinely indexing a RosterEntry
pointer (coincidental single-offset matches with unrelated structures
are far more likely for functions that only touch one offset, since
several small offsets like 0x00/0x01/0x11/0x12 are common in
completely unrelated array/table code throughout this codebase).
Printed sorted by distinct-offset count descending so the strongest
candidates come first.

Read-only: does not modify anything. Run against both ultima_bootup.idb
and ultima_exodus.idb (structs are per-IDB).
"""

import idc
import idautils
import ida_bytes
import ida_ua
import ida_funcs
import ida_struct

STRUCT_NAME = "RosterEntry"


def get_member_offsets(sid):
    offsets = {}
    off = ida_struct.get_struc(sid).memqty
    m = ida_struct.get_struc(sid)
    idx = 0
    off_val = 0
    # walk members via get_member (by index is fragile across API
    # versions) -- use offset-based enumeration instead
    size = ida_struct.get_struc_size(sid)
    seen = set()
    for o in range(size):
        mem = ida_struct.get_member(m, o)
        if mem and mem.soff not in seen:
            seen.add(mem.soff)
            name = ida_struct.get_member_name(mem.id)
            offsets[mem.soff] = name
    return offsets


def main():
    sid = idc.get_struc_id(STRUCT_NAME)
    if sid == idc.BADADDR:
        print(f"[!] struct {STRUCT_NAME!r} not found in this IDB")
        return
    offsets = get_member_offsets(sid)
    print(f"RosterEntry members: {len(offsets)} distinct offsets: "
          f"{sorted(offsets.items())}\n")

    # per-function: set of distinct offsets touched, and list of
    # (ea, opnum, offset, disasm) hits
    per_func = {}

    for ea in idautils.Heads():
        if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            continue
        for n in range(2):
            optype = idc.get_operand_type(ea, n)
            if optype != idc.o_displ:
                continue
            disp = idc.get_operand_value(ea, n)
            disp &= 0xFFFF
            if disp in offsets:
                f = ida_funcs.get_func(ea)
                fname = idc.get_func_name(f.start_ea) if f else "???"
                per_func.setdefault(fname, {"offsets": set(), "hits": []})
                per_func[fname]["offsets"].add(disp)
                per_func[fname]["hits"].append((ea, n, disp))

    ranked = sorted(per_func.items(), key=lambda kv: -len(kv[1]["offsets"]))
    for fname, data in ranked:
        offs = sorted(data["offsets"])
        names = [offsets[o] for o in offs]
        print(f"{fname}: {len(offs)} distinct offsets -> {list(zip([hex(o) for o in offs], names))}")
        for ea, n, disp in data["hits"][:40]:
            print(f"    {ea:#06x} op{n} +{disp:#x} ({offsets[disp]})  {idc.GetDisasm(ea)}")


if __name__ == "__main__":
    main()
