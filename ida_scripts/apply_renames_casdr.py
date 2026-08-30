"""
IDA Pro script: renames for casdr.idb (CASDR.EXE) -- the castle driver
(entered from OUT.EXE's board at a town; chains back to OUT).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_casdr

Add (ea, name, note) entries as functions become clear -- from the
screen text a function prints (dump_strings.py) and the call graph.

    .\run_ida_script.ps1 -Idb casdr -ScriptName apply_renames_casdr.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# CASDR.EXE = the castle / fortress driver (endgame content: the Warlord,
# the Compendium, the king's quest). Links seg000 "bmCASDR" (102 funcs)
# + seg001 "bmTNCALB" (the same town/castle animation helper module
# TWNDR uses -- 26 funcs, no screen text). Thunk table seg002 (431).
#
# (ea, new_name, note)  -- names from the screen text (dump_strings.py)
RENAMES = [
    (0x10030, "casdr_entry", "CASDR.EXE entry / init."),

    (0x10284, "spottedByGuard", '"YOU ARE SPOTTED NOW!".'),
    (0x103F0, "doWalk", '"WALK " -- castle movement.'),
    (0x10773, "chooseAbove", '"-CHOOSE ABOVE" menu helper.'),
    (0x107FA, "changeGameSpeed", '"** CHANGE GAMESPEED **".'),
    (0x108A9, "moveBlocked", '"MOVE NOWHERE".'),
    (0x10A12, "privateLevelWarn", '"PRIVATE LEVEL!".'),
    (0x10B72, "gasTrap", '"GAS TRAP!".'),
    (0x10C7F, "exitCastle",
     '"GO OUTSIDE.", "* YOU FORGOT THE ", "THE FORTRESS EXPLODES BEHIND" '
     '-- chains to OUT.EXE.'),
    (0x10FE9, "gasRoomTrap",
     '"DOORS SLAM SHUT... / GAS FILLS THE ROOM... / YOU FALL ASLEEP.".'),
    (0x112F0, "guardsWary", '"THE GUARDS EYE YOU WARILY.".'),
    (0x1140F, "doFight",
     '"FIGHT WITH ", "ENTER DIRECTION: ", "YOUR ARROW DROPS.", "HIT '
     'DOOR.  IT HOLDS.", "ATTACK ON ", " STRUCK ". ~2 KB.'),
    (0x11D60, "attackMiss", '" -- MISSED".'),
    (0x11E15, "attackHit", '" -- BLOW ", " H.P.".'),
    (0x11F49, "gasDamage", '"GAS DAMAGE ".'),
    (0x11FEE, "speakToGuard",
     '"SPEAK PASSWORD.", "NOBODY TO SPEAK TO.", "GREETINGS SOLDIER.", '
     '"THE GUARD WALKS OVER.".'),
    (0x12286, "jailerThreat",
     '"SHUT YER TRAP OR I\'LL / REACH THROUGH AND BOP YOU!".'),
    (0x122C5, "ambushGuard",
     '"YOU SURPRISE THE GUARD AND ", "YOU FIND A ROD ON THE BODY.", '
     '"IT UNLOCKS THE DOOR.".'),
    (0x123F7, "warlordConfrontation",
     'the climactic villain scene: "THE WARLORD APPEARS AT THE ", "YOU '
     'FOOL!   / YOU CAN\'T STOP ME!", "SONIC MAGIC...", "TO CAST THE '
     'SPELL OF DEATH.  ALL LIFE / OUTSIDE THIS FORTRESS WILL ". ~0.8 KB.'),
    (0x1283B, "enemyAttack", '" ATTACK - BLOW  ... H.P.".'),
    (0x129DF, "warlordAttack", '"WARLORD ATTACK - BLOW ".'),
    (0x12ABF, "describeRoom",
     'look / room description: "ICE GLISTENS BENEATH YOU.", "YOU\'RE IN '
     'A HIDDEN CORRIDOR.", "A RIVER RACES BY.", "IN A LARGE CASTLE.", '
     '"ON CASTLE LEVEL 2.", "NO OTHER / INFORMATION AVAILABLE HERE.".'),
    (0x12F0D, "describePlant",
     '"YOU SEE A STRANGE PLANT.", "IT BEARS SEVERAL SEEDS.".'),
    (0x1303D, "describeObjects",
     'room-object descriptions: "A BOX IS OPEN.", "A GRATE BLOCKS THE '
     'WAY.", "YOU SEE A SCROLL STAND.", "THE COMPENDIUM IS THERE!", '
     '"YOU SEE CANNONS.".'),
    (0x132DB, "takeItem",
     '"YOU PUT ON ARMOR.", "NOTHING TO TAKE", "YOU TAKE  SEEDS.", "YOU '
     'GRAB THE ", "YOU CAN\'T HOLD IT.".'),
    (0x13651, "openDoor", '"DOORS LOCKED." -- door/lock handling. ~1 KB.'),
    (0x13AD5, "useKey", '"UNLOCK DOOR.", "THIS KEY DOES NOTHING HERE.".'),
    (0x13C7A, "freezeWater",
     '"NO WATER CLOSE ENOUGH.", "WATER FROZEN".'),
    (0x13ED5, "invisibilitySpell", '"YOU\'RE INVISIBLE".'),
    (0x14001, "weakenSpell", '" WEAKENS.", "TOO FAR.", "THE ATTACK STOPS.".'),
    (0x1412A, "loadCastleLevel",
     'loads a castle/fort level ("castle.bs" / "fort.bs" -- CASTLE.BS1/2, '
     'FORT.BS1/2; also "COMPENDIUM").'),
    (0x142F4, "loadCastleObjects", 'loads TCASOBJ.BSV.'),
    (0x144D8, "potionWizard",
     '"MEET THE WIZARD OF POTIONS", "MY POTION CAN HELP YOU / IT WILL '
     'COST 2,500 GOLD.", "CHECK YOUR ATTRIBUTES.".'),
    (0x146B7, "kingConfides",
     'the main-quest NPC: "I WOULD LIKE TO CONFIDE IN YOU", "YOU ARE NOT '
     'STRONG ENOUGH TO / SEE ME", "FIND THE GUARDIANS OF [THE] SCROLL.  '
     'THEY ARE IN MANY TOWNS", "TALK ONLY TO THOSE WITH A SPECIAL / '
     'SECRET MARK.", "I\'VE NOW PUT THIS MAGIC MARK [ON YOUR] / '
     'FOREARM.  ONLY GUARDIANS CAN". (IDA: j_rt_FE5B_11.) ~1.2 KB.'),

    # --- 2nd pass: from the ds: engine state vars (apply_dsvars_casdr.py)
    #     + call graph + screen text ---
    (0x11BC8, "fortressSelfDestruct",
     'the post-Warlord sequence: "OUR LEADER HAS BEEN KILLED.  BLOCK / '
     'ALL DOORS.  EXPLOSIVE CHARGES SET. / SELF-DESTRUCTION IN 5 '
     'MINUTES!" -- the timed escape.'),
    (0x12EAA, "describeChest", '"YOU SEE A TREASURE CHEST." (describeRoom case).'),
    (0x12ECC, "describeLockedDoor",
     '"A MASSIVE DOOR LOOMS / BEFORE YOU.  IT IS LOCKED." (describeRoom '
     'case).'),
    (0x12FA2, "describeGasRoom",
     '"YOU\'RE IN A BARREN ROOM. / SOME OF THE AIR LOOKS CLOUDY." '
     '(describeRoom case -- the gas-trap rooms).'),
    (0x12FFC, "describePotionShop",
     '"A LOVELY YOUNG WOMAN STANDS BEHIND / A COUNTER.  MAGIC FILLS THE '
     'AIR." (describeRoom case -- the potionWizard\'s room).'),

    (0x1021B, "facePlayerDirection",
     'turn the player to face a direction (-> bmTNCALB viewFaceDirection). '
     'Same shape as TWNDR.'),
    (0x102D3, "checkLineOfSight",
     'line-of-sight test to a target (-> bmTNCALB scanLineOfSight).'),

    (0x1068D, "castleTurnUpdate",
     'per-turn castle update: reads castleOrFort / turnFlag, moves the '
     'guards and calls warlordAttack (-> refreshView / scanLineOfSight). '
     'TENTATIVE.'),
    (0x143B1, "redrawCastleView",
     'repaint the castle interior view -- called from the traps, '
     'moveBlocked, and the tile helpers. TENTATIVE.'),
    (0x14CAE, "resolveOpenDoor",
     'the OPEN-a-door logic (~0.6 KB, called from openDoor) -- reads '
     'mapStride + tileAhead, updates the tile. TENTATIVE.'),
    (0x14F74, "resolveUseKey",
     'the USE-a-key logic (called from useKey; reads tileAhead). '
     'TENTATIVE.'),
    (0x12195, "jailPlayer",
     'move the player into the cell and run jailerThreat (writes '
     'playerX/playerY). TENTATIVE.'),
    (0x13F37, "applyHealing",
     'add to hitPoints (`add ax,ds:1ADAh / mov ds:1ADAh,ax`) -- '
     'triggered via useKey. TENTATIVE.'),
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
