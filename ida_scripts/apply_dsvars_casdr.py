"""
IDA Pro script: name the CASDR.EXE engine state variables in the DGROUP
segment (CASDR's DGROUP is **seg004**, like DUN / TWNDR).

Found with `ida_scripts/dsvars.py` (DSV_DATA_SEG=seg004). Of the 252
DGROUP words the code touches, the ~13 below are genuine engine state.

CASDR shares LEGLIB's engine skeleton, so the same slots line up with
OUT / DUN / TWNDR: `partyGold` @ 1AD2, `hitPoints` @ 1ADA, `tileAhead`
@ 1F02, `targetSlot` @ 1F24. (CASDR is the endgame -- no economy -- so
`partyGold` is barely touched here, only by the story NPCs.)

Names data addresses + repeatable comments only. Run after
apply_renames_casdr.py.

    .\run_ida_script.ps1 -Idb casdr -ScriptName apply_dsvars_casdr.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

VARS = [
    (0x1AD2, "partyGold",
     "party gold (32-bit; high word at 1AD4). Same DGROUP slot as OUT / "
     "DUN / TWNDR; in CASDR only potionWizard and kingConfides touch it "
     "(the castle has no shops)."),
    (0x1AD4, "partyGold_hi", "high word of partyGold (1AD2)."),
    (0x1ADA, "hitPoints",
     "party hit points -- attackHit subtracts combat damage (`sub "
     "ds:1ADAh,ax`), sub_13F37 heals, warlordConfrontation resets it to "
     "0x1C. Same slot as OUT / DUN / TWNDR."),

    (0x1B00, "playerX",
     "player column in the castle / fort. loadCastleLevel and the "
     "scripted teleports (privateLevelWarn, gasRoomTrap, j_rt_FE4E_3, "
     "sub_12195) set it; movement inc's it; describeRoom / "
     "describeObjects branch on it."),
    (0x1B04, "playerY", "player row in the castle / fort (paired with playerX)."),

    (0x1F02, "tileAhead",
     "code for the tile / object in front of the player -- the #1 read "
     "in the module (doWalk, doFight, describeRoom, describeObjects, the "
     "gas-trap rooms, freezeWater all branch on it). Set by the bmTNCALB "
     "tile engine. Same slot as TWNDR."),
    (0x1F24, "targetSlot",
     "reset to 0xFF (= none) by doWalk / gasRoomTrap / "
     "warlordConfrontation. The selected creature / target slot. Same "
     "slot as OUT."),
    (0x1F2A, "turnFlag",
     "1 after a turn-consuming action (doWalk, doFight, ambushGuard, "
     "gasTrap set it; cleared to 0 elsewhere). Same slot as TWNDR."),

    (0x2222, "enemyHitPoints",
     "the current opponent's hit points during doFight / attackHit / "
     "enemyAttack / warlordAttack / gasDamage (accumulator: `mov "
     "ax,2222 / sub ax,2222 / mov 2222,ax`)."),

    (0x25B0, "castleArrayPtr",
     "far pointer (offset @ 25B0, segment @ 25B2) to the main CASDR "
     "game-data array -- pushed (seg then off) to nearly every rtm_ "
     "call. Never written from seg000. Same role as OUT's "
     "overworldArrayPtr etc."),
    (0x25B2, "castleArrayPtr_seg", "segment word of castleArrayPtr (25B0)."),

    # --- tentative ---
    (0x20C0, "castleOrFort",
     "a 1 / 2 selector read all over (doFight, exitCastle, "
     "privateLevelWarn, describeRoom) -- CASDR drives both the castle "
     "(CASTLE.BS1/2) and the fort (FORT.BS1/2). TENTATIVE."),
    (0x2084, "viewLevel",
     "second 1 / 2 mode flag, often checked next to castleOrFort. "
     "TENTATIVE."),
    (0x1E22, "menuChoice",
     "current menu / Y-N answer (changeGameSpeed, doFight, kingConfides, "
     "potionWizard read it; never written from seg000). TENTATIVE."),
    (0x1F26, "mapStride",
     "current interior map row width -- loadCastleLevel sets it (0x70 / "
     "0x5A), bmTNCALB's setViewport also sets it per layout, and it's "
     "the imul / idiv factor for `y*stride + x` (sub_13E33 / sub_14CAE)."),
    (0x1F28, "mapHeight",
     "current interior map height (0x28 / 0x5B / 0x49), paired with "
     "mapStride, set by bmTNCALB's setViewport."),

    # --- shared with the bmTNCALB interior engine (seg001) ---
    (0x0101, "dgroupSeg",
     "the DGROUP self-segment -- `mov es, ds:101h` in the bmTNCALB tile "
     "routines."),
    (0x2082, "viewBufOffset",
     "screen-buffer start offset for the interior view region "
     "(setViewport sets 0x960 / 0x1060). TENTATIVE."),

    # --- bmTNCALB private state (CASDR link: the 0x24xx block --
    #     the same variables TWNDR links at 0x26xx, +0x176 apart) ---
    (0x24B4, "tileScanIndex",
     "running index into the map array during a tileAt lookup / LOS "
     "scan; moveActor resets it. (TWNDR: ds:262A.)"),
    (0x24B6, "losScanResult",
     "line-of-sight scan result -- scanLineOfSight inits it to 0xFFFF "
     "then stores the first blocking tile. (TWNDR: ds:262C.)"),
    (0x24C0, "actorDrawMode",
     "1 = erase the actor sprite at its old tile, 0 = draw it at the "
     "new one (moveActor). (TWNDR: ds:2636.)"),
    (0x24B0, "mapArrayBase",
     "base offset of the interior map data in its array descriptor. "
     "(TWNDR: ds:2626.) TENTATIVE."),
    (0x24B2, "mapRowBytes",
     "row stride in bytes of the map array. (TWNDR: ds:2628.) "
     "TENTATIVE."),
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
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
