"""
IDA Pro script: renames for dun.idb (DUN.EXE) -- the dungeon engine
(chains back to OUT.EXE).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_dun
(coerce_code does the structural work + crefs; dump_strings decodes the
screen text and comments each `mov reg,<dgrp>` with it; this only sets
names + repeatable comments and must not trigger a reanalysis.)

Add (ea, name, note) entries as functions become clear -- from the
screen text a function prints (dump_strings.py) and the call graph.

    .\run_ida_script.ps1 -Idb dun -ScriptName apply_renames_dun.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# DUN.EXE links two compiled-BASIC code segments: seg000 ("bmDUN", the
# main engine) and seg001 ("bmDUNG", a graphics helper module -- 9 funcs,
# no screen text, left sub_ for now). Thunk table is seg002.
#
# (ea, new_name, note)
RENAMES = [
    (0x10030, "dun_entry", "DUN.EXE entry -> dunMain."),
    (0x10039, "dunMain",
     'main dungeon loop (called from entry): status effects ("YOU ARE '
     'BEFUDDLED."), death ("YOU DIED!"), dispatches to the movement / '
     'combat / magic / search handlers.'),

    (0x103C2, "doMovement", '"TURN ", "MOVE " -- walk/turn in the dungeon.'),
    (0x105E3, "moveHazards",
     '"YOU AVOID THE ", "YOU FALL THROUGH A HIDDEN HOLE.", "YOU\'RE '
     'AMBUSHED BY A " -- per-step trap / ambush check.'),
    (0x10744, "selectAbove", '"- SELECT ABOVE" menu helper.'),
    (0x1077C, "changeGameSpeed",
     '"ENTER GAME SPEED (1 IS FASTEST)". (IDA named it j_rt_FE5B.)'),
    (0x10AE5, "lookOrSearch",
     '"NOTHING UNUSUAL IS IN SIGHT.", "HIDDEN OBJECTS DETECTED!!!"; '
     'chains SAVER.EXE on quit. (IDA: j_rt_FE5B_3.)'),
    (0x12450, "psychoStrengthSpell",
     '"ALREADY IN EFFECT", "YOU FEEL VERY STRONG!". (IDA: j_rt_FE5B_17.)'),
    (0x107DF, "climbUp", '"NOTHING TO CLIMB", "UP".'),
    (0x10838, "climbDownOrExit",
     '"DOWN", "YOU ARE NOW AT LEVEL ", "YOU CLIMB OUT OF THE DUNGEON.", '
     '"STRENGTH:  +" -- level change / dungeon exit.'),
    (0x10ABD, "chainToMuseum", '-> MUS.EXE.'),
    (0x10AD1, "chainToOverworld", '-> OUT.EXE.'),
    (0x10BD7, "describeSurroundings",
     '" IS STALKING YOU!", " IS IN SIGHT.", "YOU ARE STANDING NEXT TO A "'),
    (0x10E33, "openNothing", '"NOTHING TO OPEN" stub.'),
    (0x10E75, "openChest",
     '"THE CHEST IS EMPTY.", "THE CHEST IS BOOBY TRAPPED!!!", "YOU FIND '
     'A ", "YOU FIND A COMPASS!". ~1.4 KB.'),
    (0x1151C, "monsterAttack",
     '"ATTACK MISSED.", "HIT BY BLOW OF ", "KNUCKLES BROKE YOUR ", '
     '" ATE YOUR ", "ENDURANCE DRAIN: ". ~1.2 KB.'),
    (0x11B2E, "checkAttackTarget",
     '"NOTHING TO FIGHT", " IS OUT-OF-RANGE OF YOUR ", "HIT ".'),
    (0x11C30, "doAttack",
     '" WITH ", "YOUR ATTACK MISSES.", "ENEMY HIT BY BLOW OF ", '
     '"(PSYCHO STRENGTH SPELL IN EFFECT", " DIES!!".'),
    (0x11E7B, "useMagicMenu",
     '"USE WHICH MAGIC?", "YOU HAVE NO ", "SHOOT ", "THERE IS NO '
     'EFFECT.", "ATTACK FIZZLES", "SELECT NO MAGIC". ~0.9 KB.'),
    (0x121DA, "castSpell", '"OTHER MAGIC ", "CAST ".'),
    (0x12306, "spellResult",
     '"THE SPELL BACKFIRES!", " LOOKS CONFUSED.".'),
    (0x12814, "findJewel", '"YOU FIND A LARGE PULSATING JEWEL".'),
    (0x13406, "showHitPoints", '"H.P. -".'),
    (0x13571, "loadDungeonLevel",
     'loads a dungeon level ("level", "monst" -- DUNM*.BSV / '
     'DUNMON*.BSV). ~0.7 KB, called from an init helper.'),

    # --- 2nd pass: from the ds: engine state vars (apply_dsvars_dun.py)
    #     + call graph ---
    (0x12536, "processTileFeature",
     'the per-turn tile / feature handler (~0.7 KB, called first from '
     'dunMain). Walks a feature-name table (holds "POISON GAS VENT" / '
     '"FLOOR HOLE" / "SLIME SPLOT" + a char-class table), updates '
     'playerX / playerY / dungeonLevel / levelProgressFlags / '
     'actionPhase, and branches on tileAhead.'),
    (0x1320C, "moveMonsters",
     'per-turn monster update loop (BASIC SUB) -- reads playerX / '
     'playerY to step each monster and check reach. ~0.5 KB, called '
     'from dunMain.'),
    (0x12CE7, "stepMonsterToward",
     'move one monster a step toward (playerX, playerY). TENTATIVE.'),
    (0x1068D, "drawDungeonHud",
     'draw the dungeon status line -- calls showHitPoints, formats the '
     'level / feature text (reads dungeonLevel, featureUnderfoot).'),
    (0x10CDB, "doLookSearch",
     'the LOOK / SEARCH action: sets turnActionFlag, clears scanTile, '
     'scans the surrounding tiles and reports (rtm_FE41). Called from '
     'lookOrSearch.'),
    (0x10CD2, "clearTurnFlag", "turnActionFlag := 0 (before a non-turn action)."),
    (0x1305C, "rebuildLevelView",
     'rebuild the level view after a climb / level change (descriptor '
     'at ds:1E2Ah). Called only from climbDownOrExit. TENTATIVE.'),
    (0x1393C, "updateLevelState",
     'refresh per-level state (reads dungeonLevel + levelProgressFlags) '
     '-- called after climbs, kills, and from processTileFeature. '
     'TENTATIVE.'),
    (0x12C34, "rollChestContents",
     'decide what a chest holds (reads dungeonLevel + levelProgressFlags). '
     'Called from openChest. TENTATIVE.'),
    (0x1345D, "monsterSpecialAttack",
     'monster special-attack resolution (called from monsterAttack). '
     'TENTATIVE.'),
    (0x1031C, "redrawDungeonView",
     'repaint the dungeon viewport (called from climb / describe / '
     'doAttack / the FE5B path). TENTATIVE.'),
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
