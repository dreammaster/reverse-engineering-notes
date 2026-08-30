"""
IDA Pro script: master list of symbol renames for out.idb (OUT.EXE) --
the overworld / towns / dungeons engine (chains to MUS.EXE, SAVER.EXE,
TWNDR.EXE, CASDR.EXE, DUN.EXE).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> apply_renames_out
(coerce_code does the structural work + fall-through crefs; this only
sets names + repeatable comments and must not trigger a reanalysis.)

Add (ea, name, note) entries as functions become clear. seg000 is
~99.7% coerced (2026-08-30); 190 functions, most still sub_, a handful
over-merged by the call-far fragmentation merge -- flagged for a later
tuning pass. OUT's DGROUP screen text is position-coded and not readily
readable, so naming will lean more on the rtm_* call pattern and the
data-file / chained-EXE references than on printed strings.

    .\run_ida_script.ps1 -Idb out -ScriptName apply_renames_out.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x10030, "out_entry",
     "OUT.EXE program entry (compiled-BASIC module init). Falls through "
     "into the overworld main loop."),
]


def main():
    seg = ida_segment.get_segm_by_name("seg000")
    S0, S0E = seg.start_ea, seg.end_ea

    done = skip = 0
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  {ea:#x}: {cur!r} -> {name!r}")
            continue
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_func_cmt(ea, note, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    total = sum(1 for _ in idautils.Functions(S0, S0E))
    named = sum(1 for f in idautils.Functions(S0, S0E)
                if not idc.get_func_name(f).startswith(("sub_", "j_", "nullsub")))
    print(f"\napplied {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))
    print(f"seg000: {named}/{total} functions named")


main()
