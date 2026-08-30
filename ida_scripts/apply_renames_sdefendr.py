"""
IDA Pro script: renames for sdefendr.idb (SDEFENDR.EXE) -- the
**combat-training school** minigame (a 360deg "defender"-style shooter).

Found in a town (chains back to TWNDR.EXE). The player picks "ARMOR
TRAINING" or "WEAPONS TRAINING", gets a briefing ("As you stand in the
center of this stadium, magic fireballs will approach from all sides.
Use your <weapon> to <hit> the fireballs before they hit you ... Use the
direction keys to turn about. Use either shift key to fire arrows ...
training will be over if you're hit five times"), then plays through
waves. Doing well raises the trained stat (ARMOR / WEAPON / ENDURANCE);
each session costs 50 gold; there are seven levels, and clearing all
seven gives "CONGRATULATIONS! ... seven levels of training".

Two code segments:
  * seg000 "bmSDEFENDR" -- the compiled-BASIC framing layer (mode
    select, briefing text, wave/score screens, rating + stat change,
    the gold economy, the TWNDR hand-off).
  * seg001 -- hand-written assembly: the real-time arena engine (player
    turn/fire, 8-direction fireball movement, collision, sprite draw),
    driven from a tight loop over playfield data in seg004.

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_sdefendr

    .\run_ida_script.ps1 -Idb sdefendr -ScriptName apply_renames_sdefendr.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # --- seg000: compiled-BASIC framing layer ---
    (0x10037, "sdefendr_entry", "SDEFENDR.EXE entry stub -> trainingSchoolMain."),
    (0x1003A, "trainingSchoolMain",
     'the school: "ARMOR TRAINING" / "WEAPONS TRAINING" mode select, runs '
     'the levels, then the rating -- "Your training has gone well." / '
     '"You made no real gains." / "You\'ve wasted our time.", " INCREASE: '
     '+" / " DECREASE: -" applied to ARMOR/WEAPON/ENDURANCE, "Train more '
     'for 50 gold?" / "You don\'t have the gold.", "CONGRATULATIONS! ... '
     'seven levels of training", then chains to TWNDR.EXE. ~2.3 KB.'),
    (0x1094C, "showWaveScore",
     '"You\'ve completed / LEVEL <n> - WAVE <n>", "TOTAL HITS: <n>".'),
    (0x10B20, "showBriefing",
     'the "Do you need briefing?" text: "This school is your chance to '
     'perfect your use of <weapon> ... stand in the center of this '
     'stadium, magic fireballs will approach from all sides ... Use '
     'either shift key to fire arrows ... over if you\'re hit five '
     'times." ~1.4 KB.'),
    (0x1114C, "runTrainingLevel",
     'one graded level: loops showWaveIntro / waves, drawScorePanel, then '
     'arenaGameLoop; feeds the hit count back to trainingSchoolMain.'),
    (0x1121D, "runPractice",
     '"Want practice?" / "Want more practice?" -- ungraded arena runs '
     '(drawScorePanel + arenaGameLoop) before the real levels.'),
    (0x11321, "showWaveIntro", '"LEVEL <n>" / " - WAVE <n>" banner.'),
    (0x113D4, "drawScorePanel", '"FIREBALLS        HITS" score panel.'),
    (0x110B4, "drawFramedBox",
     'draws a framed text box (rtm_FE42 x3 / rtm_FE45). Shared by the '
     'briefing / prompt / result screens. TENTATIVE.'),
    (0x10A7D, "clearArenaWindow",
     'clears the full 0..40 x 0..24 play window (rtm_FE42 / rtm_FE45). '
     'TENTATIVE.'),

    # --- seg001: hand-written real-time arena engine ---
    (0x114D0, "arenaGameLoop",
     'the real-time training-arena loop: sets DS from seg004 (playfield) '
     'and repeatedly runs the 8 step routines below until arenaStep_end '
     'signals (carry). Pure asm, no BASIC frame.'),
    (0x114FC, "arenaInitPlayfield",
     'loads the seg004 playfield pointer into ds:0Ch/ds:0Eh and sets up '
     'the arena state. TENTATIVE.'),
    (0x116CA, "pollPlayerTurn",
     'reads the direction keys (ds:11h / ds:16h timer) and rotates the '
     'player\'s facing. TENTATIVE.'),
    (0x11817, "firePlayerArrow",
     'on shift-key (test ds:15h,1) with the ds:22h cooldown elapsed, '
     'spawns a player arrow. TENTATIVE.'),
    (0x1190E, "moveFireballs",
     'advances every incoming fireball; dispatches to one of 8 '
     'direction-specific movers. TENTATIVE.'),
    (0x115E6, "arenaStepEndCheck",
     'per-frame end condition (returns carry to break arenaGameLoop) -- '
     'timer expiry or 5 hits taken. TENTATIVE.'),
    (0x11CA2, "drawArenaSprites",
     'renders the arena frame -- player, arrows, fireballs -- from the '
     'seg004 object list. ~0.6 KB. TENTATIVE.'),
]


def main():
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

    total = named = 0
    for sn in ("seg000", "seg001"):
        seg = ida_segment.get_segm_by_name(sn)
        if not seg:
            continue
        for f in idautils.Functions(seg.start_ea, seg.end_ea):
            total += 1
            if not idc.get_func_name(f).startswith(("sub_", "j_", "nullsub", "start")):
                named += 1
    print(f"\napplied {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))
    print(f"seg000+seg001: {named}/{total} functions named")


main()
