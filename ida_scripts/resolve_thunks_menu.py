"""
menu.idb structural script: name every int-3Fh thunk in seg001 after the
LEGLIB run-time routine it reaches, using the map produced by
resolve_rtm_leglib.py (ida_scripts/rtm_map.py).

menu.exe's seg001 is a flat table of 3-/4-byte trampolines
(`CD 3F nn` / `CD 3F FF nn` / `CD 3F FE nn`); every `call far` in seg000
targets one of them. See resolve_rtm_leglib.py for the dispatch mechanism.
The (prefix,ordinal) namespace is shared across all client modules, so the
names here match the ones in leglib.idb.

This does NOT touch seg000 (still undisassembled) -- run a seg000
code-coercion pass separately; once that's done these thunk names make
every call site read as `call far rt_<key>`.

    .\run_ida_script.ps1 -Idb menu -ScriptName resolve_thunks_menu.py
"""

import os
import sys
import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False
THUNK_SEG = "seg001"

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from rtm_map import RTM_MAP
except Exception as e:
    RTM_MAP = {}
    print(f"[!] no rtm_map.py ({e}) -- thunks will be named rt_<key> without target info")


def key_str(prefix, ordinal):
    return f"{ordinal:02X}" if prefix is None else f"{prefix:02X}{ordinal:02X}"


def main():
    seg = ida_segment.get_segm_by_name(THUNK_SEG)
    thunks = []
    ea = seg.start_ea
    while ea < seg.end_ea - 2:
        if ida_bytes.get_byte(ea) == 0xCD and ida_bytes.get_byte(ea + 1) == 0x3F:
            b2 = ida_bytes.get_byte(ea + 2)
            if b2 in (0xFE, 0xFF):
                prefix, ordinal, size = b2, ida_bytes.get_byte(ea + 3), 4
            else:
                prefix, ordinal, size = None, b2, 3
            thunks.append((ea, prefix, ordinal, size))
            ea += size
        else:
            ea += 1

    named = commented = itemized = 0
    for ea, prefix, ordinal, size in thunks:
        k = (prefix, ordinal)
        rt = RTM_MAP.get(k)

        if not DRY_RUN:
            # make the thunk a clean 3-/4-byte item: int 3Fh + db operand(s)
            ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
            idc.create_insn(ea)                       # CD 3F -> int 3Fh
            ida_bytes.create_data(ea + 2, ida_bytes.FF_BYTE, size - 2, idc.BADADDR)
            itemized += 1

            want = "rt_" + key_str(prefix, ordinal)
            cur = idc.get_name(ea)
            if not cur or cur.startswith(("loc_", "sub_", "unk_", "byte_", "off_")):
                if idc.set_name(ea, want, idc.SN_NOWARN):
                    named += 1

            if rt:
                cmt = f"-> {rt['name']}  (leglib {rt['seg']}:{rt['ea']:#x})"
                if rt.get("state") == "mid-func":
                    cmt += "  [mid-func]"
            else:
                cmt = f"-> run-time entry {key_str(prefix, ordinal)} (unresolved)"
            idc.set_cmt(ea, cmt, 1)
            commented += 1

    # coverage
    used = set()
    for ea, prefix, ordinal, size in thunks:
        refs = set(idautils.CodeRefsTo(ea, 0)) | set(idautils.DataRefsTo(ea))
        if refs:
            used.add((prefix, ordinal))

    print(f"seg001: {len(thunks)} thunks  "
          f"(bare {sum(1 for _,p,_,_ in thunks if p is None)}, "
          f"FF {sum(1 for _,p,_,_ in thunks if p==0xFF)}, "
          f"FE {sum(1 for _,p,_,_ in thunks if p==0xFE)})")
    print(f"named {named}, commented {commented}, itemized {itemized}"
          + ("   [DRY_RUN]" if DRY_RUN else ""))
    print(f"resolved via rtm_map: {sum(1 for _,p,o,_ in thunks if (p,o) in RTM_MAP)}/{len(thunks)}")
    print(f"currently referenced from (partial) seg000 analysis: {len(used)}")

    miss = [key_str(p, o) for (_, p, o, _) in thunks if (p, o) not in RTM_MAP]
    if miss:
        print(f"not in rtm_map ({len(miss)}): {', '.join(miss)}")


main()
