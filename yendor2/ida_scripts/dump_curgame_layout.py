"""
Read-only: gathers what's needed to document the CURGAME/SAVGAMEn layout.

  1. Every named data item inside the two in-memory blocks that
     SaveCurrentGameToSlot snapshots to disk: the 5000-byte header+party
     block at DS:0x93FF and the 12480-byte level-monster block at DS:0x0F26.
  2. The callers (grouped by containing function) of each CURGAME
     record-setup stub, to identify what sections 2-6 actually hold.

    .\run_ida_script.ps1 dump_curgame_layout.py -NoExport
"""
import idautils
import idc
import ida_funcs
import ida_name

DS_BASE = 0x2D860

BLOCKS = [
    ("header+party block", 0x93FF, 0x93FF + 5000),
    ("level-monster block", 0x0F26, 0x0F26 + 12480),
]

STUBS = [
    "PrepareMasterHeaderBlockRead",
    "PrepareGameDialogSizedBlockRead",
    "PrepareGroundItemSlotBlockRead",
    "PrepareRecordAtIndexDC6",
    "PrepareRecordAtIndexDCA",
    "PrepareGameDialogIndexedBlockRead",
    "PrepareGameDialogLargeBlockRead",
]

for label, lo, hi in BLOCKS:
    print(f"\n=== {label}: DS:{lo:#06x}-{hi:#06x} ===")
    ea = DS_BASE + lo
    end = DS_BASE + hi
    while ea < end:
        name = ida_name.get_name(ea)
        if name and not name.startswith(("unk_", "byte_", "word_", "dword_")) or (name and name.startswith(("word_", "dword_", "byte_"))):
            cmt = idc.get_cmt(ea, 0) or idc.get_cmt(ea, 1) or ""
            print(f"  DS:{ea - DS_BASE:#06x} (+{ea - DS_BASE - lo:#06x}) {name}" + (f"  ; {cmt[:160]}" if cmt else ""))
        nxt = idc.next_head(ea, end)
        if nxt <= ea:
            break
        ea = nxt

print("\n=== callers of each CURGAME record-setup stub ===")
for stub in STUBS:
    ea = idc.get_name_ea_simple(stub)
    if ea == idc.BADADDR:
        print(f"{stub}: NOT FOUND")
        continue
    callers = {}
    for ref in idautils.CodeRefsTo(ea, 0):
        fn = ida_funcs.get_func(ref)
        fname = idc.get_func_name(fn.start_ea) if fn else f"<no func> {ref:#x}"
        callers.setdefault(fname, []).append(ref)
    print(f"\n{stub} @ {ea:#x}: {sum(len(v) for v in callers.values())} call sites")
    for fname, refs in sorted(callers.items()):
        print(f"    {fname}  x{len(refs)}  " + ", ".join(f"{r:#x}" for r in refs[:4]))
