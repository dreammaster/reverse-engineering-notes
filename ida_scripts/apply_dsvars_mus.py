"""
IDA Pro script: name the MUS.EXE (the MUSEUM driver) engine state
variables in the DGROUP segment (seg004).

Found with `ida_scripts/dsvars.py` (DSV_DATA_SEG=seg004). MUS is small
(143 DGROUP words touched); the ~10 below are the real state.

Shared LEGLIB slots line up as usual: `partyGold` @ 1AD2, `hitPoints`
@ 1ADA, `playerX`/`playerY` @ 1B02/1B06, `menuChoice` @ 1E22. MUS does
*not* use `chainDestType` -- it hands the next module its name directly
as a string in `chainExeName` (ds:210C).

Names data addresses + repeatable comments only. Run after
apply_renames_mus.py.

    .\run_ida_script.ps1 -Idb mus -ScriptName apply_dsvars_mus.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

# (offset, name, is_str, comment)
VARS = [
    (0x1AD2, "partyGold", False,
     "party gold (32-bit; high word at 1AD4). caretakerOffer and "
     "showGold touch it. Same DGROUP slot as OUT / DUN / TWNDR / CASDR."),
    (0x1AD4, "partyGold_hi", False, "high word of partyGold (1AD2)."),
    (0x1ADA, "hitPoints", False,
     "party hit points -- caretakerOffer sets it to 0xBB8 (3000, a "
     "full-heal / cap); sub_1204B adds and caps. Same slot as the other "
     "modules."),

    (0x1B02, "playerX", False,
     "player column in the museum. Teleported to fixed exhibit "
     "positions by useCommand / sub_1127E / sub_1134E."),
    (0x1B06, "playerY", False, "player row in the museum (paired with playerX)."),

    (0x20FE, "exhibitId", False,
     "id of the display case the player is at. readPlaque sets it; "
     "enterExhibit is a big SELECT CASE on it (3 / 6 / 8 / 0x0A .. 0x0D "
     "-> the different exhibits -> chain to TWNDR / DUN / STDRV / "
     "CELDRV)."),

    (0x210C, "chainExeName", True,
     "string buffer holding the name of the module to chain to -- "
     "chainToTown / chainToDungeon / chainToStory / chainToCel do "
     "basStrAssign(210C, \"TWNDR\" | \"DUN\" | \"STDRV\" | \"CELDRV\"). "
     "MUS uses this instead of a chainDestType code."),

    (0x2136, "flagTestMask", False,
     "the bit mask staged before an exhibit / quest flag test (the "
     "checkFlag_* helpers set it, then call testExhibitFlag)."),
    (0x2138, "flagTestResult", False,
     "scratch: result of `record_flags & flagTestMask` from "
     "testExhibitFlag (record flag words live at [ds:1B96 desc + 0x16] "
     "etc, not in DGROUP)."),

    # --- tentative ---
    (0x20B6, "messageId", False,
     "current plaque / message text index -- set (39..73) before a draw "
     "by doWalk / enterExhibit / the exhibitName_* setters. TENTATIVE."),
    (0x2106, "exhibitSubStep", False,
     "small counter (0..2, wraps) tracking a sub-step inside "
     "enterExhibit. TENTATIVE."),
]


def name_data(ea, name, is_str, cmt):
    n = 16 if is_str else 2
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, n)
    if not is_str:
        ida_bytes.create_data(ea, ida_bytes.FF_WORD, 2, idc.BADADDR)
    if idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK):
        if cmt:
            idc.set_cmt(ea, cmt, 1)
        return True
    return False


def main():
    s4 = ida_segment.get_segm_by_name("seg004")
    base = s4.start_ea
    done = skip = 0
    for off, name, is_str, cmt in VARS:
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  ds:{off:04X}: {cur!r} -> {name!r}")
            continue
        if cur == name:
            skip += 1
            continue
        if name_data(ea, name, is_str, cmt):
            done += 1
        else:
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
