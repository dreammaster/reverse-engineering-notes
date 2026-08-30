"""
IDA Pro script: names for the shared **bmTNCALB** module -- `seg001` of
both twndr.idb and casdr.idb (byte-for-byte the same 26-function
compiled-BASIC module, only relocated differently, so it's keyed by
seg001-relative offset and this one script applies to either IDB).

bmTNCALB is the common town/castle *interior* engine: the map-view
renderer, actor movement, coordinate / facing math, and tile lookups
that TWNDR and CASDR share. Its routines don't touch DOS or print text;
they drive the `rtm_FE1x` / `rtm_FE06` graphics primitives in
`leglib.idb` seg004 and read the interior map array (empty tile = 0xFF),
using the `ds:1F26h/1F28h` viewport, `ds:262Ah/262Ch` scan state, and
`ds:2636h` show/hide flag.

Run after the module's coerce pass:
  ... -> coerce_code ($env:COERCE_SEG='seg001') -> apply_renames_tncalb

    .\run_ida_script.ps1 -Idb twndr -ScriptName apply_renames_tncalb.py
    .\run_ida_script.ps1 -Idb casdr -ScriptName apply_renames_tncalb.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# (seg001 offset, name, note)  -- offsets verified identical twndr/casdr
RENAMES = [
    (0x0DF, "drawInteriorTiles",
     "renders the interior room view -- walks the map array and blits "
     "each tile via rtm_FE1B / rtm_FE14 (leglib seg004)."),
    (0x2C3, "drawActor",
     "draw or erase the actor/NPC sprite at its tile (ds:2636h = 1 draw "
     "/ 0 erase). Called from moveActor both ways."),
    (0x37F, "stepByDirection",
     "given a direction (0..3) and a coord pair, produce the coord one "
     "step that way (and an in-bounds flag). Called from doWalk."),
    (0x33,  "viewFaceDirection",
     "direction (0..3) -> the facing code 0x81/0x82/0x84 + camera set-up "
     "(rtm_C8 / rtm_FE06)."),
    (0x884, "findObjectTile",
     "scan the interior map array for the first non-empty tile "
     "(!= 0xFF). Used by the NPC / dialog code."),
    (0x8EF, "tileAt",
     "look up the map tile / object id at a coord (rtm_FE17). Returns "
     "the id (and via es:[bx] its attributes)."),
    (0xA5E, "scanLineOfSight",
     "step outward from a position calling tileAt, accumulating in "
     "ds:262Ch -- line-of-sight / nearest-blocker scan. Called from "
     "doWalk, jailScene, the NPC code."),
    (0xCC1, "moveActor",
     "move the actor one tile and redraw: erase (drawActor 0), update "
     "position (via dirBetween / updateTile), draw (drawActor 1). "
     "Called from stepByDirection."),
    (0xF14, "refreshView",
     "rebuild + blit the whole interior view: drawInteriorTiles then "
     "rtm_FE69. Called after any state change."),
    (0xF31, "setViewport",
     "direction (0..3) -> the interior viewport window "
     "(ds:1F26h/1F28h/ds:2634h/ds:2082h)."),
    (0x1055, "traceCombatLine",
     "combat: trace from the attacker along a direction (tileAt per "
     "step, rtm_FE18 to draw), set ds:1F02h/1F04h. Called from "
     "fightGuard."),
    (0x12E9, "dirBetween",
     "compute the step direction (0..3, or 3 = none) from one coord to "
     "another via the map."),

    # --- 2nd pass: the remaining helpers (seg001 profile + call graph) ---
    (0x81D, "refreshTileGraphic",
     "redraw a single interior tile (rtm_FE19). The shared low-level "
     "blit -- called from moveActor, walkBlocked, traceCombatLine, "
     "robCommand, speakCommand and the ray helpers."),
    (0x5E7, "stepLineOfSight",
     "one outward step of the scanLineOfSight ray -- advances the coord "
     "(via dirBetween) and tests the tile."),
    (0x72C, "sightBlockedBy",
     "the second scanLineOfSight helper -- classify what stopped the "
     "ray (wall / actor / edge). TENTATIVE."),
    (0x4BD, "traceCombatRay",
     "step the traceCombatLine ray one tile (-> refreshTileGraphic). "
     "TENTATIVE."),
    (0xC6C, "combatRayResult",
     "resolve what the combat ray hit (rtm_DF). Called from "
     "traceCombatLine. TENTATIVE."),
    (0xB38, "placeNpcSprite",
     "position an NPC sprite in the view (rtm_C8 camera + rtm_11). "
     "Called from npcRecurringDialog. TENTATIVE."),
    (0xC3E, "drawViewFrame",
     "draw the interior-view border / frame (rtm_FE1E). Called from "
     "townServiceDispatch and robberyEvent. TENTATIVE."),
    (0x12AF, "tileAtOffset",
     "tileAt wrapper that first applies a coord offset. TENTATIVE."),
]


def main():
    s1 = ida_segment.get_segm_by_name("seg001")
    if s1 is None:
        print("no seg001")
        return
    base = s1.start_ea

    done = skip = 0
    for off, name, note in RENAMES:
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  {ea:#x} (seg001+{off:#x}): {cur!r} -> {name!r}")
            continue
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_func_cmt(ea, note, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    total = sum(1 for _ in idautils.Functions(base, s1.end_ea))
    named = sum(1 for f in idautils.Functions(base, s1.end_ea)
                if not idc.get_func_name(f).startswith(("sub_", "j_", "nullsub")))
    print(f"\napplied {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))
    print(f"seg001 (bmTNCALB): {named}/{total} functions named")


main()
