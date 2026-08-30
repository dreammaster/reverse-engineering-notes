"""
IDA Pro script: master list of symbol renames for out.idb (OUT.EXE) --
the overworld / towns / dungeons engine (chains to MUS.EXE, SAVER.EXE,
TWNDR.EXE, CASDR.EXE, DUN.EXE).

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_out
(coerce_code does the structural work + crefs; dump_strings decodes the
screen text and comments each `mov reg,<dgrp>` with it; this only sets
names + repeatable comments and must not trigger a reanalysis.)

Most names below come from the screen text a function prints (see
docs/file-formats.md for the string format). Structural names come from
the call graph + the `ds:` state vars each function pokes. The engine
state variables themselves are named in **apply_dsvars_out.py** (run
after this): partyGold (1AD2), hitPoints (1ADA), playerX/playerY
(1B02/1B06), contextMode (1F2A), subMode (2146), combatPhase (2192),
encounterActive (21FE), questFlags (2234), chainDestType (1F16),
turnActionFlag (212E), overworldArrayPtr (24E6), … -- see
`ida_scripts/dsvars.py` for the profiler that found them.

    .\run_ida_script.ps1 -Idb out -ScriptName apply_renames_out.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x10030, "out_entry",
     "OUT.EXE entry / module init: declares the module-scope variables "
     "(14x rt_FF4B/rt_FF50) and sets up the screen (rt_AF x3, rt_98). "
     "Falls through into outInit."),
    (0x10199, "outInit",
     "overworld first-time setup: 9x basScreenInit (screen regions), "
     "loads the overworld data via the engine, calls doMovement. ~2 KB, "
     "called once from out_entry."),
    (0x13C60, "mainDispatch",
     "the central overworld command/event loop (~3.8 KB). Branches on "
     "ds:1F2Ah; prints the combat lines (NOTHING TO FIGHT / NOT IN "
     "RANGE. / YOUR ATTACK MISSES. / ENEMY HIT BY BLOW OF ) and fans out "
     "to the per-command helpers."),

    (0x11760, "applyGameFlag",
     "shared tail of the ds:2234h flag-setter family: mov si,1B96h then "
     "folds the pushed mask into the flag word."),
    (0x11638, "setFlag_03", "set game flag mask 0x03 (-> applyGameFlag)."),
    (0x11681, "setFlag_38", "set game flag mask 0x38 (-> applyGameFlag)."),
    (0x1168A, "setFlag_C0", "set game flag mask 0xC0 (-> applyGameFlag)."),
    (0x116D8, "setFlag_0300", "set game flag mask 0x0300 (-> applyGameFlag)."),
    (0x11705, "setFlag_0800", "set game flag mask 0x0800 (-> applyGameFlag)."),
    (0x1171F, "setFlag_1000", "set game flag mask 0x1000 (-> applyGameFlag)."),
    (0x127D2, "setMode_1", "ds:2146h := 1, then jmp j_rt_FE4E."),
    (0x127DB, "setMode_2", "ds:2146h := 2, then jmp j_rt_FE4E."),
    (0x127E4, "setMode_3", "ds:2146h := 3."),

    # --- named from the screen text (dump_strings.py) ---
    (0x10B06, "doMovement",
     'walk / travel: "MOVE ", terrain gates ("THE RAFT MUST STAY IN THE '
     'WATER.", "YOU ARE NOT EQUIPPED TO / CROSS THE MOUNTAINS.", "THERE '
     'IS TOO MUCH WATER FOR TRAVEL."), the food/health tick ("YOU GROW '
     'SICK FROM / SOMETHING YOU ATE!", "HIT POINTS:"), "RETURN TO '
     'MUSEUM?". ~1.5 KB, called from outInit.'),
    (0x110BF, "tryDisengage",
     '"ATTEMPT TO DISENGAGE ... IS BLOCKED. / IS SUCCESSFUL."'),
    (0x111FC, "enterLocation",
     'board/enter a map location: "ENTER ", "RETURN TO ", "ONLY RUBBLE '
     'IS LEFT."'),
    (0x117B0, "creatureApproach",
     'encounter start: "UNKNOWN CREATURE", " APPROACHING FROM THE ", '
     '" IS / ARE APPROACHING." ~1.3 KB.'),
    (0x11CB3, "creatureAttack",
     'creature turn / damage: "ATTACKED BY ", "HITS: ", "DAMAGE: ", '
     '"YOU FALL UNCONSCIOUS.", "THE SLEEP DOES YOU GOOD. / YOU AWAKE '
     'FEELING BETTER.", "CHECK YOUR SUPPLIES!!". IDA mis-named it '
     'j_rt_FE5B_1; ~1.2 KB.'),
    (0x12143, "describeCreature", '" STANDS / STAND ... BEFORE YOU."'),
    (0x12252, "avoidCreature", '"YOU AVOID THE CREATURE"'),
    (0x125D1, "promptDirection", '" RAFT", "NOTHING TO ", "WHICH DIRECTION?"'),
    (0x12641, "cantDoThat", 'the generic "YOU CAN\'T " refusal.'),
    (0x128C5, "changeGameSpeed",
     '"** CHANGE GAME SPEED ** / (1 IS FASTEST) / GAMESPEED IS: "'),
    (0x12969, "quitOrTalk",
     '"Can\'t quit now", "NO ONE IS THERE / ... FAR AWAY TO HEAR YOU.", '
     'chains SAVER.EXE on quit. Dispatches on ds:1F2Ah.'),
    (0x12B3B, "buyFood",
     'food merchant: "DO YOU WANT TO BUY / DAYS OF FOOD FOR / GOLD?" ~1 KB.'),
    (0x12F8F, "setItemAdj1", 'item adjective := "WELL CRAFTED".'),
    (0x12F9D, "setItemAdj2", 'item adjective := "SPARKLING NEW".'),
    (0x12FAB, "setItemAdj3", 'item adjective := "WONDERFUL".'),
    (0x12FB9, "setItemAdj4", 'item adjective := "MAGNIFICENT".'),
    (0x12FC7, "shopBuy",
     'museum-shop purchase flow: "DO YOU WANT TO BUY A / WOULD YOU LIKE '
     'TO BUY A / MUSEUM COIN FOR / USE THIS", "YOU PASSED UP A GOOD '
     'DEAL! / MAYBE LATER... / PURCHASE COMPLETED". ~0.9 KB.'),
    (0x13896, "describeAdjacent", '"YOU ARE NEXT TO "'),
    (0x138CA, "chainToTown", "chains to TWNDR.EXE (town driver)."),
    (0x13922, "chainToCastle", '"A CASTLE" -> chains to CASDR.EXE.'),
    (0x13961, "chainToMuseum", '"THE MUSEUM" -> chains to MUS.EXE.'),
    (0x13982, "chainToDungeon", '"A DUNGEON" -> chains to DUN.EXE.'),
    (0x13A13, "doAttackOrCast",
     '"ATTACK WITH ", "ATTACK FIZZLES", "CAST SEEK SPELL."'),
    (0x13BDE, "checkSpellRange", '"YOU ARE TOO FAR AWAY.", "NO EFFECT."'),
    (0x157DE, "chainExec",
     "loads + execs another program via the RTM (holds the \"Error in "
     "loading RTM\" path) -- the mechanism behind chainToTown/Castle/etc."),
    (0x158EE, "pegasusOrAmbush",
     'special travel event: "PEGASUS SETS YOU DOWN", "YOU ARE AMBUSHED '
     'BY BANDITS!". ~0.75 KB.'),
    (0x15BE1, "compendiumStolenEvent",
     'scripted story beat: "YOU AWAKE. THE COMPENDIUM IS GONE.", "YOU '
     'HEAR A VOICE... DO NOT BE DISCOURAGED. IT WAS INEVITABLE. KEEP TO '
     'YOUR QUEST."'),
    (0x161AD, "museumAccessPrompt",
     'the museum access-code entry ("World- / Stone- / Ring- ", ordinal '
     'suffixes st/nd/rd/th, "*** TRY AGAIN ***"). ~1.6 KB.'),

    # --- 2nd pass: named from the ds: engine state vars + call graph
    #     (apply_dsvars_out.py) + the screen text ---
    (0x1486B, "enterOverworld",
     'set up overworld play (called once from outInit): clears '
     'contextMode, sets playerX/playerY and turnActionFlag, and calls '
     'loadOverworldData + the sub-setup helpers (sub_12823 / sub_122CB / '
     'setupLocationDisplay). ~0.5 KB.'),
    (0x14619, "loadOverworldData",
     'BLOADs the overworld map/monster banks -- "OUTM<n>.BSV", '
     '"OUTDATA.BSV" (+ lower-case fallbacks). Keyed by combatPhase '
     '("0"+combatPhase in the filename). ~0.6 KB.'),
    (0x1608D, "drawOverworldViewport",
     'redraw the overworld map view centred on playerX/playerY. '
     'Called by the map-load path (sub_145DB / loadOverworldData). '
     'TENTATIVE.'),
    (0x145DB, "initOverworldViewport",
     'viewport / scroll-window setup preceding drawOverworldViewport. '
     'TENTATIVE.'),

    (0x151B7, "resolveMoveTarget",
     'per-move tile examination (~1 KB, BASIC SUB): given the trial '
     'coords from doMovement, work out what is on the destination tile '
     '-- sets enteredLocationId / targetSlot and dispatches to '
     'identifyLocationObject / refreshMapView / readTileObject.'),
    (0x14CDF, "identifyLocationObject",
     'classify the map object on the target tile; writes '
     'enteredLocationId. TENTATIVE.'),
    (0x155A3, "readTileObject",
     'read the object/creature record for a tile (writes '
     'enteredLocationId). TENTATIVE.'),
    (0x15FA2, "classifyLocationTile",
     'map the raw tile/object type under the player (copied to '
     'ds:2182h by doMovement) to a location code in enteredLocationId '
     '-- SELECT CASE 0..7 (0 -> 0x0A, ...).'),
    (0x14AA7, "enterLocationOrChain",
     'act on enteredLocationId: either handle the location inline or '
     'call chainExec to hand off to TWNDR / CASDR / MUS / DUN.'),
    (0x16030, "resolveTownEntry",
     'chainToTown helper: writes the town id into enteredLocationId '
     'before the hand-off. TENTATIVE.'),

    (0x1232F, "beginEncounterView",
     'transition into the encounter / parley view: back up contextMode '
     '(to ds:1F10h) when < 5, set it to 0x0B then 0x0C, roll the '
     'creature position (rtm_FC), set combatPhase. Called by '
     'creatureApproach and quitOrTalk. ~0.7 KB.'),
    (0x13D98, "resolvePlayerAttack",
     'the player attack round: hit/miss math off the value stack, '
     '"YOUR ATTACK MISSES." / "ENEMY HIT BY BLOW OF ", steps '
     'combatPhase. ~0.7 KB.'),
    (0x14054, "creatureDefeated",
     'creature-death resolution (~1.1 KB): " DIES.", clears '
     'contextMode / encounterActive, awards loot -- partyGold, "YOU '
     'FIND " (-> awardFoundItem), and the "DO YOU WANT TO USE THE / '
     'FLESH FOR FOOD?  YOU GAIN <n> DAYS OF FOOD." option.'),
    (0x14523, "awardFoundItem", '"YOU FIND A " -- add a dropped item to inventory.'),
    (0x1449C, "describeFoundItem", '"YOU FIND A " item-description helper. TENTATIVE.'),
    (0x122CB, "rollCreatureStats",
     'roll / fetch the encountered creature\'s stats from '
     'overworldArrayPtr (used by creatureAttack and enterOverworld). '
     'TENTATIVE.'),

    (0x13334, "addFoodDays",
     'add food to the party ration count + redraw the gauge -- called '
     'by buyFood and creatureDefeated ("DAYS OF FOOD"). TENTATIVE.'),
    (0x133FD, "spendFoodDays", 'decrement the ration count. TENTATIVE.'),
    (0x134A1, "drawFoodGauge", 'render the food/rations indicator. TENTATIVE.'),

    (0x157B0, "redrawAfterAction",
     'redraw the status line + map view after any turn-consuming action '
     '(called from mainDispatch, creatureAttack, avoidCreature, shopBuy, '
     'tryDisengage, outInit, creatureDefeated, banditAmbushEvent).'),
    (0x14BD0, "drawStatusBar",
     'draw the HUD / status line (reads turnActionFlag). TENTATIVE.'),
    (0x14EA4, "refreshMapView", 'repaint the map viewport region. TENTATIVE.'),
    (0x11454, "setupLocationDisplay",
     'draw the 10..12 location-type indicators for the current tile '
     '(loops rtm_FE38); reads/writes subMode. TENTATIVE.'),

    (0x15BB7, "banditAmbushEvent",
     'the "YOU ARE AMBUSHED BY BANDITS!" travel event -> knockout ("YOU '
     'FALL UNCONSCIOUS.") -> the scripted Compendium theft ("YOU AWAKE.  '
     'THE COMPENDIUM IS GONE.", "DO NOT BE DISCOURAGED ... KEEP TO YOUR '
     'QUEST."). Hits hitPoints, sets a questFlag. ~0.5 KB.'),
    (0x15B97, "showPegasusLanding", '"PEGASUS SETS YOU DOWN" message.'),
    (0x15A21, "pegasusFlightAnim",
     'the Pegasus fly-across animation loop (steps pegasusFlyStep east '
     'tile by tile).'),
    (0x15A78, "pegasusFlyStep",
     'advance the Pegasus one tile east: playerX++, re-run '
     'resolveMoveTarget, redraw (rtm_FE55).'),
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
