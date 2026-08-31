"""
IDA Pro script: name the OUT.EXE engine state variables in the DGROUP
segment (seg003).

OUT.EXE is compiled BASIC, so its module-scope variables live at fixed
offsets in DGROUP (`ds:<off>` in the listing). `ida_scripts/dsvars.py`
(read-only) profiles every DGROUP word the code touches -- read/write
counts, the constants stored, and which functions use it. The entries
below are the ones whose meaning is clear from those use sites; the rest
are mostly per-call BASIC scratch temps (the compiler stages an argument
in DGROUP, pushes its address, and calls a runtime routine) and are not
worth naming.

Names data addresses + repeatable comments only -- safe to re-run, never
triggers a reanalysis. Run after apply_renames_out.py.

    .\run_ida_script.ps1 -Idb out -ScriptName apply_dsvars_out.py
"""

import idc
import idautils
import ida_segment
import ida_bytes

DRY_RUN = False

# DGROUP offset -> (name, is_dword, comment)
VARS = [
    (0x1AD2, "partyGold", True,
     "party gold (32-bit; high word at 1AD4). shopBuy / buyFood subtract "
     "the price via rtm_EE; rewards and sales add. See apply_renames_out."),
    (0x1AD4, "partyGold_hi", False, "high word of partyGold (1AD2)."),

    (0x1ADA, "hitPoints", False,
     "party hit points. doMovement subtracts a starvation/fatigue amount "
     "each step (then prints \"HIT POINTS: \"); creatureAttack subtracts "
     "combat damage; <=1 -> \"YOU FALL UNCONSCIOUS.\"; rest / eating "
     "restore it. buyFood compares against 0x64 (100 = full)."),

    (0x1B02, "playerX", False,
     "player column on the current map. Init 40 (out_entry). doMovement "
     "stages a trial X in ds:208C, runs the collision test (sub_151B7), "
     "and copies it back here on success. Scripted events (mainDispatch, "
     "pegasusOrAmbush, ...) also teleport it."),
    (0x1B06, "playerY", False,
     "player row on the current map. Init 30. Paired with playerX "
     "(trial Y staged in ds:208A)."),

    (0x1F02, "enteredLocationId", False,
     "map-object id of the location the player is entering / standing on "
     "(set by enterLocation, read by doMovement and the chainTo* "
     "functions to tell the next module which place to load)."),
    (0x1F16, "chainDestType", False,
     "destination kind handed to the next executable: 2 = castle "
     "(chainToCastle), 3 = town (chainToTown), 4 = dungeon "
     "(chainToDungeon), 6 = museum (chainToMuseum / outInit). Consumed "
     "by chainExec."),

    (0x1F2A, "contextMode", False,
     "top-level context / mode selector. mainDispatch branches on it and "
     "clears it to 0; values 0x0A / 0x0B / 0x0C seen for the "
     "overworld / encounter / location sub-contexts. (Previously noted "
     "only as \"ds:1F2Ah = OUT's top-level mode var\".)"),
    (0x2146, "subMode", False,
     "secondary mode within contextMode -- setMode_1 / setMode_2 / "
     "setMode_3 store 1 / 2 / 3 (also 4); read by mainDispatch, "
     "creatureAttack, doAttackOrCast."),

    (0x2192, "combatPhase", False,
     "encounter / attack state machine. creatureApproach and "
     "creatureAttack step it through 1..7 (approach, avoid check, "
     "trade blows, resolve); 0 = no encounter in progress."),
    (0x21FE, "encounterActive", False,
     "1 while a creature encounter is running -- gated on by "
     "creatureApproach / creatureAttack / describeCreature / "
     "avoidCreature / quitOrTalk (\"cmp ds:21FEh, 1\")."),

    (0x2234, "questFlags", False,
     "quest / story progress bit-field. The setFlag_* family OR in a "
     "mask (0x03 / 0x38 / 0xC0 / 0x0300 / 0x0800 / 0x1000) via "
     "applyGameFlag."),
    (0x212E, "turnActionFlag", False,
     "set to 1 when the player has taken a turn-consuming action "
     "(doMovement). mainDispatch, on seeing it, decrements the in-game "
     "day/turn counter in the game record and clears it back to 0."),

    (0x24E6, "overworldArrayPtr", True,
     "far pointer (offset @ 24E6, segment @ 24E8) to the main OUT "
     "game-data array -- loaded by outInit and pushed (seg then off) as "
     "an argument to nearly every rtm_ call (rtm_B8 in particular)."),
    (0x24E8, "overworldArrayPtr_seg", False,
     "segment word of overworldArrayPtr (24E6)."),

    # --- tentative ---
    (0x2180, "tileAhead", False,
     "map tile value read from the world array during doMovement and "
     "handed to the collision test. TENTATIVE."),
    (0x2254, "activeCreaturePtr", False,
     "record base (used as BX) for the creature currently being handled "
     "in the encounter / talk code. TENTATIVE."),
    (0x1F24, "targetSlot", False,
     "reset to 0xFF (= none) at encounter start and by several handlers; "
     "doMovement sets it to a small index. Likely the selected "
     "creature / target slot. TENTATIVE."),

    # --- 2nd pass: the mid-traffic vars ---
    (0x1F04, "basRetVal", False,
     "the shared function-return / expression-result word (touched by "
     "27 functions). A callee stages its result here (enterLocation "
     "0x61A8, the combatBeat_* subcodes, doMovement's blocked flag) and "
     "the caller reads it."),
    (0x1F06, "basRetVal2", False,
     "the aux / string-descriptor half of basRetVal -- holds a second "
     "value or a DGROUP string pointer alongside 1F04."),
    (0x0101, "dgroupSeg", False,
     "the DGROUP self-segment -- `mov es, ds:101h` in resolveMoveTarget "
     "/ resolveLocationFromMap / refreshMapView / museumAccessPrompt."),
    (0x1E22, "menuChoice", False,
     "the current Y/N / menu answer (read-only in seg000 -- set by the "
     "prompt routines). Same DGROUP slot as every other module."),
    (0x1E24, "selectedSpell", False,
     "spell-menu selection index -- doAttackOrCast / lookupSpellSlot "
     "read it; cleared by outInit / mainDispatch. Same slot as "
     "DUN.EXE's selectedSpell."),
    (0x208C, "trialX", False,
     "candidate move destination X -- doMovement stages it here, hands "
     "its address to resolveMoveTarget, and copies it into playerX on "
     "success."),
    (0x208A, "trialY", False, "candidate move destination Y (paired with trialX)."),
    (0x1ADC, "remarkIndex", False,
     "index into the flavour-string array (showIndexedRemark x4's it; "
     "useCompass keys on `== 0x0B`)."),

    # --- tentative ---
    (0x214A, "attackRange", False,
     "attack / spell range value -- checkSpellRange and j_rt_FE5B_5 "
     "test `== 7`. TENTATIVE."),
    (0x1E20, "screenLayout", False,
     "active screen-layout / view-mode code (set to 0x0B by "
     "enterLocation / enterOverworld / the FE4E handlers). Recurs across "
     "modules. TENTATIVE."),
    (0x1F1A, "travelEventFlag", False,
     "1 when a Pegasus / ambush travel event is pending (set by "
     "pegasusOrAmbush / doMovement, cleared on resolution). TENTATIVE."),
    (0x2182, "pendingLocationType", False,
     "copy of enteredLocationId staged before classifyLocationTile. "
     "TENTATIVE."),
    (0x222E, "flagWordSel", False,
     "second quest-flag word / group selector used by applyGameFlag "
     "next to questFlags (2234). TENTATIVE."),
    (0x2444, "tileObjectRec", False,
     "base of the ~8-word tile-object record readTileObject fills and "
     "resolveMoveTarget consumes (type / coords / id fields). TENTATIVE."),
    (0x231C, "transactionType", False,
     "shop / food transaction kind (buyFood / addFoodDays / shopBuy / "
     "creatureDefeated set 4 or 7). TENTATIVE."),
]


def main():
    s3 = ida_segment.get_segm_by_name("seg003")
    base = s3.start_ea

    done = skip = 0
    for off, name, is_dword, cmt in VARS:
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  ds:{off:04X} ({ea:#x}): {cur!r} -> {name!r}")
            continue
        # these offsets sit inside a big undefined DGROUP blob -- carve a
        # word out before naming it, or set_name rejects the tail byte.
        # 32-bit values are named as two words (lo + _hi/_seg companion).
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 2)
        ida_bytes.create_data(ea, ida_bytes.FF_WORD, 2, idc.BADADDR)
        if cur != name and not idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK):
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
            continue
        idc.set_cmt(ea, cmt, 1)
        done += 1 if cur != name else 0
        skip += 1 if cur == name else 0

    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
