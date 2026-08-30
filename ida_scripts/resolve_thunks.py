"""
Generic structural script: find every int-3Fh run-time thunk in a Legacy
of the Ancients client module and name it after the LEGLIB routine it
reaches, using ida_scripts/rtm_map.py (built by resolve_rtm_leglib.py).

Each client .EXE has a flat table of 3-/4-byte trampolines --
`CD 3F nn` / `CD 3F FF nn` / `CD 3F FE nn` -- that every `call far`
targets; the (prefix,ordinal) namespace is identical across modules (see
docs/overview.md "int 3Fh run-time dispatch"). The table's location
varies per module: menu.exe puts it in its own segment (seg001); out.exe
embeds it mid-code in seg000. This script auto-locates it by scanning
every segment for the dense `CD 3F ..` run.

Non-destructive: itemises a thunk only if it's still raw (never disturbs
one coerce_code.py already promoted to a returning function -- that would
trigger a reanalysis that drops the code segment's flow-crefs). Names and
the propagating `-> rtm_*` comments are re-applied every run, so this is
safe as both the first and the last step of a module's pipeline.

    .\run_ida_script.ps1 -Idb out -ScriptName resolve_thunks.py
"""

import os
import sys
import idc
import idautils
import ida_bytes
import ida_segment
import ida_funcs
import ida_auto

DRY_RUN = False

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from rtm_map import RTM_MAP
except Exception as e:
    RTM_MAP = {}
    print(f"[!] no rtm_map.py ({e}) -- thunks named rt_<key> without target info")


def key_str(prefix, ordinal):
    return f"{ordinal:02X}" if prefix is None else f"{prefix:02X}{ordinal:02X}"


def find_thunks():
    """Return [(ea, prefix, ordinal, size)] for the module's thunk table,
    located as the longest run of back-to-back `CD 3F` trampolines in any
    segment."""
    best = []
    for i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(i)
        run = []
        ea = seg.start_ea
        gap = 0
        while ea < seg.end_ea - 3:
            if ida_bytes.get_byte(ea) == 0xCD and ida_bytes.get_byte(ea + 1) == 0x3F:
                b2 = ida_bytes.get_byte(ea + 2)
                if b2 in (0xFE, 0xFF):
                    prefix, ordinal, size = b2, ida_bytes.get_byte(ea + 3), 4
                else:
                    prefix, ordinal, size = None, b2, 3
                run.append((ea, prefix, ordinal, size))
                ea += size
                gap = 0
            else:
                ea += 1
                gap += 1
                if gap > 16 and run:
                    if len(run) > len(best):
                        best = run
                    run = []
        if len(run) > len(best):
            best = run
    return best


def ensure_thunk_selector(lo, hi):
    """out.exe embeds the thunk table mid-code in seg000; its `call far`
    operands use a frame selector (e.g. 0x67E) that IDA has no mapping
    for, so they render as `call far ptr 67Eh:472h`. Register the
    selector -> frame-paragraph mapping so a `call far` to a named thunk
    resolves to `call far ptr rt_<key>` -- without carving a new segment
    (that would renumber every segment after it, and Paul correlates
    segment names/numbers with the DOSBox debugger). menu.exe keeps the
    table in its own segment already -- nothing to do."""
    seg = ida_segment.getseg(lo)
    if seg.start_ea >= lo and seg.end_ea <= hi:
        return
    from collections import Counter
    frames = Counter()
    for a in range(seg.start_ea, seg.end_ea - 4):
        if ida_bytes.get_byte(a) != 0x9A:
            continue
        off, sel = ida_bytes.get_word(a + 1), ida_bytes.get_word(a + 3)
        for base in (0x1000, 0):
            t = ((sel + base) << 4) + off
            if lo <= t < hi:
                frames[(sel, (t - off) >> 4)] += 1
    if not frames:
        return
    (sel, fpara), _ = frames.most_common(1)[0]
    idc.set_selector(sel, fpara)
    ida_auto.auto_wait()
    print(f"thunk frame selector {sel:#x} -> para {fpara:#x}")


def main():
    thunks = find_thunks()
    if not thunks:
        print("no thunk table found")
        return
    lo, hi = thunks[0][0], thunks[-1][0] + thunks[-1][3]
    ensure_thunk_selector(lo, hi)
    seg = ida_segment.getseg(lo)
    print(f"thunk table: {ida_segment.get_segm_name(seg)}:{lo - seg.start_ea:#x}"
          f"-{hi - seg.start_ea:#x}  ({lo:#x}-{hi:#x})")

    named = commented = itemized = 0
    for ea, prefix, ordinal, size in thunks:
        rt = RTM_MAP.get((prefix, ordinal))
        if DRY_RUN:
            continue

        f = ida_bytes.get_full_flags(ea)
        if not ida_bytes.is_code(f) and not ida_bytes.is_data(f):
            idc.create_insn(ea)
            ida_bytes.create_data(ea + 2, ida_bytes.FF_BYTE, size - 2, idc.BADADDR)
            itemized += 1

        want = "rt_" + key_str(prefix, ordinal)
        cur = idc.get_name(ea)
        if (not cur or cur.startswith(("loc_", "sub_", "unk_", "byte_", "off_"))) \
           and idc.set_name(ea, want, idc.SN_NOWARN):
            named += 1

        if rt:
            cmt = f"-> {rt['name']}  (leglib {rt['seg']}:{rt['ea']:#x})"
            if rt.get("state") == "mid-func":
                cmt += "  [mid-func]"
        else:
            cmt = f"-> run-time entry {key_str(prefix, ordinal)} (unresolved)"
        fn = ida_funcs.get_func(ea)
        if fn and fn.start_ea == ea:
            idc.set_func_cmt(ea, cmt, 1)     # propagates from a func head
        else:
            idc.set_cmt(ea, cmt, 1)          # propagates from a plain label
        commented += 1

    bare = sum(1 for _, p, _, _ in thunks if p is None)
    ff = sum(1 for _, p, _, _ in thunks if p == 0xFF)
    fe = sum(1 for _, p, _, _ in thunks if p == 0xFE)
    used = sum(1 for ea, _, _, _ in thunks
               if set(idautils.CodeRefsTo(ea, 0)) | set(idautils.DataRefsTo(ea)))
    print(f"{len(thunks)} thunks (bare {bare}, FF {ff}, FE {fe})")
    print(f"named {named}, commented {commented}, itemized {itemized}"
          + ("   [DRY_RUN]" if DRY_RUN else ""))
    print(f"resolved via rtm_map: {sum(1 for _, p, o, _ in thunks if (p, o) in RTM_MAP)}"
          f"/{len(thunks)}   referenced: {used}")
    miss = [key_str(p, o) for (_, p, o, _) in thunks if (p, o) not in RTM_MAP]
    if miss:
        print(f"not in rtm_map ({len(miss)}): {', '.join(miss)}")


main()
