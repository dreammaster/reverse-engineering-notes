"""
IDA Pro script: name the CELDRV.EXE (endgame victory cinematic) DGROUP
variables (CELDRV's DGROUP is **seg003**).

Found with `ida_scripts/dsvars.py` (DSV_DATA_SEG=seg003). CELDRV is tiny
(2 KB of code, 16 funcs) -- 47 DGROUP words touched, and nearly all are
one-function `mov ds:20xx, <const>` writes staging drawString / rtm_FE
region params. The handful of real state below drive the cinematic.

Names data addresses + repeatable comments only. Run after
apply_renames_celdrv.py.

    .\run_ida_script.ps1 -Idb celdrv -ScriptName apply_dsvars_celdrv.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

VARS = [
    (0x20B2, "storyLine",
     "the victory-narration line counter (0..999). celdrv_entry / "
     "runCreditsCrawl / celAnimStep compare it against 0x3E5 (997) and "
     "0x3E7 (999); passing 997 hands off to runCreditsCrawl."),
    (0x208A, "celBank",
     "which of the five image banks is being loaded -- celdrv_entry's "
     "SELECT CASE 0..4 = CEL0 / CEL1 / CEL2 / DIS9 / CEL3.BSV."),
    (0x208C, "celRelocBase",
     "the paragraph offset celdrv_entry adds to a freshly-BLOADed "
     "bank's internal pointer table to relocate it (`es:[bx] += "
     "ds:208Ch`)."),
    (0x20BE, "celFrame",
     "the cel-animation frame counter -- celAnimStep inc's it and wraps "
     "1..5."),
    (0x20F8, "displayDuration",
     "how long to hold the current frame / credit page (~3000..6000 "
     "ticks). Each showCredit* / celAnimStep sets its own; blitCelFrame "
     "reads it."),

    # --- tentative ---
    (0x1AEE, "creditPage",
     "runCreditsCrawl page counter (const 1). TENTATIVE."),
    (0x211A, "musicTickState",
     "serviceMusic per-call state word. TENTATIVE."),
]


def name_data(ea, name, cmt):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 2)
    ida_bytes.create_data(ea, ida_bytes.FF_WORD, 2, idc.BADADDR)
    if idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK):
        if cmt:
            idc.set_cmt(ea, cmt, 1)
        return True
    return False


def main():
    base = ida_segment.get_segm_by_name("seg003").start_ea
    done = skip = 0
    for off, name, cmt in VARS:
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  ds:{off:04X}: {cur!r} -> {name!r}")
            continue
        if cur == name:
            skip += 1
            continue
        if name_data(ea, name, cmt):
            done += 1
        else:
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
