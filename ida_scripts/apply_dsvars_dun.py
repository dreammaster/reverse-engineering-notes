"""
IDA Pro script: name the DUN.EXE engine state variables in the DGROUP
segment (DUN's DGROUP is **seg004**, not seg003 -- seg002 is the thunk
table, seg003 the RTM bootstrap).

Found with `ida_scripts/dsvars.py` (run as
`DSV_DATA_SEG=seg004 ... dsvars.py`). Of the 145 DGROUP words the code
touches, the ~15 below are genuine cross-function engine state; the rest
are per-call compiled-BASIC scratch temps.

DUN reuses LEGLIB's engine skeleton, so a few offsets line up with OUT
(`turnActionFlag` @ 212E, `chainDestType` @ 1F16, `hitPoints` @ 1ADA);
the dungeon-specific state sits in the 0x20xx block.

Names data addresses + repeatable comments only. Run after
apply_renames_dun.py.

    .\run_ida_script.ps1 -Idb dun -ScriptName apply_dsvars_dun.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

# DGROUP offset -> (name, comment)
VARS = [
    (0x1ACA, "dungeonLevel",
     "current dungeon depth. dunMain / climbUp / climbDownOrExit / "
     "findJewel / loadDungeonLevel all key off it; used as the RNG "
     "scale (imul) and the DUNM<n>.BSV selector."),
    (0x1ADA, "hitPoints",
     "party hit points. monsterAttack subtracts damage; openChest / "
     "healing traps add (then cap); spellResult checks against 0xFA "
     "(250). Staged into ds:20EA for the \"H.P.\" display."),

    (0x20CE, "playerX",
     "player column in the dungeon. doMovement inc/dec's it; sub_1320C "
     "uses (20CE,20D0) for range/line-of-sight distance."),
    (0x20D0, "playerY", "player row in the dungeon (paired with playerX)."),
    (0x20C4, "tileAhead",
     "code for the tile / feature directly in front of the player -- "
     "doMovement and moveHazards branch on it (0..0x10, bit 0x80 = a "
     "flag); sub_10CDB rewrites it while resolving a step."),
    (0x20C2, "featureUnderfoot",
     "code for the feature on the player's own tile -- checked for "
     "stairs (0x0A / 0x0D) before climbUp / climbDownOrExit, and by "
     "lookOrSearch / openNothing. TENTATIVE."),
    (0x20BE, "scanTile",
     "tile code the look / describe commands report on "
     "(describeSurroundings, lookOrSearch, sub_10CDB). TENTATIVE."),
    (0x20C0, "moveDelta",
     "signed step result set by doMovement (0 / 1 / -1). TENTATIVE."),

    (0x1E24, "selectedSpell",
     "spell-menu selection index (0..0x19). Set by useMagicMenu, read "
     "by castSpell to pick the effect."),
    (0x20EC, "actionPhase",
     "small phase enum (1..4) set by the setActionPhase_* stubs and "
     "castSpell; monsterAttack and sub_12536 advance it. TENTATIVE."),
    (0x1AE2, "levelProgressFlags",
     "per-level bit flags (openChest tests 0x100 / 0x200 / 0x400 / "
     "0x700; sub_11A1D tests 0x400) -- what has been opened / triggered "
     "on this level. TENTATIVE."),

    (0x212E, "turnActionFlag",
     "1 after a turn-consuming action (set by sub_10CDB, cleared by "
     "sub_10CD2). Combat routines also use it as a 0/1 index to pick "
     "attacker vs. defender stats. Same DGROUP slot as OUT.EXE."),
    (0x1F16, "chainDestType",
     "destination kind for the next executable: 3 = overworld "
     "(chainToOverworld), 6 = museum (chainToMuseum). Same slot as "
     "OUT.EXE's chainDestType."),

    (0x2274, "dungeonArrayPtr",
     "far pointer (offset @ 2274, segment @ 2276) to the main DUN "
     "game-data array -- pushed (seg then off) as an argument to nearly "
     "every rtm_ call (rtm_B8). Never written from seg000. Same role as "
     "OUT.EXE's overworldArrayPtr."),
    (0x2276, "dungeonArrayPtr_seg", "segment word of dungeonArrayPtr (2274)."),

    (0x20EA, "hpDisplayScratch",
     "NOT engine state -- a DGROUP scratch word the code stages a value "
     "in before a runtime call / the \"H.P.\" display (showHitPoints "
     "pushes it). Named so it is not mistaken for hitPoints (1ADA)."),
]

# tiny runtime-dispatched stubs: actionPhase := N
PHASE_STUBS = [(0x1072C, 1), (0x10735, 2), (0x1073E, 3)]


def name_data(ea, name, cmt):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 2)
    ida_bytes.create_data(ea, ida_bytes.FF_WORD, 2, idc.BADADDR)
    ok = idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK)
    if ok and cmt:
        idc.set_cmt(ea, cmt, 1)
    return ok


def main():
    s4 = ida_segment.get_segm_by_name("seg004")
    base = s4.start_ea
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

    for ea, n in PHASE_STUBS:
        nm = f"setActionPhase_{n}"
        if not DRY_RUN and idc.get_func_attr(ea, idc.FUNCATTR_START) == ea:
            if idc.get_name(ea) != nm and idc.set_name(ea, nm, idc.SN_NOWARN):
                idc.set_func_cmt(ea, f"actionPhase := {n} (runtime-dispatched stub).", 1)
                done += 1

    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
