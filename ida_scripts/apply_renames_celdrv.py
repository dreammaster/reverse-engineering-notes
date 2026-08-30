"""
IDA Pro script: renames for celdrv.idb (CELDRV.EXE) -- the **endgame
victory cinematic** ("cel" = the cel-animation image banks it plays).

CELDRV shows the game's ending: it BLOADs the CEL0/CEL1/CEL2.BSV +
DIS9.BSV + CEL3.BSV image banks, puts up the "AGAINST ALL ODDS!" title
card, then scrolls the illustrated victory-story narration -- a past-tense
recap of the whole quest with the hero's name (`<N>`) substituted in
("THUS <N> ARRIVED AT THE GREAT MUSEUM ... FLEW THE FABLED PEGASUS ...
VICTORIOUS, AND PLACED THE WIZARD'S COMPENDIUM FOR ETERNAL SAFEKEEPING
... ADVENTURE AWAITS...") over music, and finishes with the end credits
(John & Charles Dougherty; IBM version by Alvin De Young / Rick Tumanis /
Gregg Seelhoff / Bob Luzenski; music by Johnny Klonaris; artwork by Rick
Tumanis / Dan Stechow / Roseann Miller / C&J Dougherty / Ron Vanlent).

Chained to from CASDR.EXE after the Warlord is defeated (MUS.EXE also
has a `chainToCel` hand-off).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_celdrv

    .\run_ida_script.ps1 -Idb celdrv -ScriptName apply_renames_celdrv.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# CELDRV.EXE: tiny -- single code seg seg000 "bmCELDRV" (16 funcs, 2 KB),
# thunk table seg001 (373 entries), DGROUP seg003.
#
# (ea, new_name, note)
RENAMES = [
    (0x10030, "celdrv_entry",
     'the endgame cinematic driver: BLOADs the CEL0-2 / DIS9 / CEL3.BSV '
     'image banks (rt_FE07), relocates their offset tables, shows the '
     '"AGAINST ALL ODDS!" title, then runs the scrolling victory-story '
     'narration (999 lines, <N> = hero name) with music, then falls into '
     'runCreditsCrawl. ~1.1 KB.'),

    (0x106E9, "scrollStoryText",
     'scrolls one screenful of the victory narration: fetches story line '
     'i (rt_46 array access), draws it (drawStringInner), computes the '
     'pixel scroll offset and calls delayWithMusic; loops i=1..count.'),

    (0x104CE, "runCreditsCrawl",
     'the end-credits sequence: cycles the credit pages (showCredit*), '
     'random frame pick (rt_FC), cel animation via rt_FE2C / rt_FE42; '
     'entered when the story line counter passes 997.'),
    (0x105AB, "showCreditIbmVersion",
     '"IBM VERSION BY / ALVIN DE YOUNG / RICK TUMANIS / GREGG SEELHOFF / '
     'BOB LUZENSKI".'),
    (0x105D6, "showCreditMusic",
     '"ORIGINAL MUSIC & SOUNDS BY / JOHNNY KLONARIS".'),
    (0x10601, "showCreditArtwork",
     '"ARTWORK BY / RICK TUMANIS / DAN STECHOW / ROSEANN MILLER / ...".'),
    (0x1062C, "showCreditArtworkCont",
     '" C&J DOUGHERTY / RON VANLENT" (artwork list continuation).'),
    (0x10585, "drawCreditLine",
     'assign + drawStringInner + clear one credit string. TENTATIVE.'),

    (0x10800, "serviceMusic",
     'music-queue pump: rt_CE (basPlayMusic) + rt_4E / rt_F0 / rt_F4. '
     'Called from every delay / wait point.'),
    (0x107B4, "delayWithMusic",
     'wait ~N units (arg) while pumping serviceMusic. TENTATIVE.'),
    (0x107CA, "waitKeyWithMusic",
     'poll for a keypress (rt_FE54) while pumping serviceMusic; self-loop. '
     'TENTATIVE.'),

    (0x10657, "celAnimStep",
     'advance one cel-animation frame (j_rt_FE2C + blitCelFrame). TENTATIVE.'),
    (0x10685, "blitCelFrame",
     'blit the current cel frame (rt_FE05 / rt_FE53 graphics). TENTATIVE.'),
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
