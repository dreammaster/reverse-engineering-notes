"""
IDA Pro script: renames for mus.idb (MUS.EXE) -- **the MUSEUM driver**
(not music -- "MUS" = Museum). The Tarmalon Museum is the game's central
hub; its display cases are portals into the world. Chains out to TWNDR
(town exhibits), DUN, STDRV (story), CELDRV (cel animations). Loads
MUSDATA.BSV / MUSOBJ.BSV, reads MUSMSG.TXT.

Names-only, run last:
  resolve_thunks -> coerce_code -> resolve_thunks -> dump_strings
  -> apply_renames_mus

    .\run_ida_script.ps1 -Idb mus -ScriptName apply_renames_mus.py
"""

import idc
import idautils
import ida_segment

DRY_RUN = False

# MUS.EXE links seg000 "bmMUS" (109 funcs) + seg001 "bmMUSDUNG" (8 funcs,
# the museum-interior version of the bmDUNG helper). Thunk table seg002.
#
# (ea, new_name, note)  -- names from the screen text (dump_strings.py)
RENAMES = [
    (0x10030, "mus_entry", "MUS.EXE entry / museum init."),

    (0x10199, "doWalk", '"TURN ", "WALK ", "BUMP INTO WALL".'),
    (0x10454, "selectAbove", '"- SELECT ABOVE".'),
    (0x104B1, "changeGameSpeed", '"SELECT GAME SPEED (1 IS FASTEST)".'),
    (0x1054A, "describeMuseumRoom",
     '"YOU ARE IN AN ANCIENT MUSEUM", "YOU SEE A DOOR WITHOUT A KNOB", '
     '"A DISPLAY CASE PULSES NEARBY", "A TORCH CRACKLES NEARBY.".'),
    (0x106A4, "readPlaque",
     '"FACE THE DISPLAY CASE.", "YOU SEE A PLAQUE.  IT READS".'),
    (0x106FE, "enterExhibit",
     'the display-case portal: "(INSERT ", " - EXHIBIT CLOSED - ", '
     '"WOULD YOU LIKE TO GO / TO", "YOU HAVEN\'T USED THIS EXHIBIT", '
     '"YOU\'LL NEED A", "INSERT YOUR ". ~1 KB.'),
    (0x111AE, "welcomeMessage",
     '"WELCOME / TO THE FAMED / TARMALON MUSEUM!".'),
    (0x11374, "eatFruitCommand", '"EAT THE FRUIT".'),
    (0x113A1, "fruitEffect", '"YOU FEEL A TINGLING SENSATION".'),
    (0x11486, "caretakerOffer",
     '"DO YOU ACCEPT THE / CARETAKER\'S OFFER?". ~0.75 KB.'),
    (0x117A5, "searchCommand", '"HELP SEARCH", "CONTINUE SEARCHING".'),
    (0x118E4, "showGold", '"GOLD:  +". (IDA: j_rt_FE5B.)'),
    (0x11A85, "climbCommand", '"CLIMB ON".'),
    (0x11CA0, "rereadDescription",
     '"DO YOU WANT TO REREAD THE / DESCRIPTION OF THIS EXHIBIT?".'),
    (0x1212D, "useCommand",
     '"WORKING...", "NOTHING HAPPENS.", " HUMS SOFTLY.", "THE DOOR DOES '
     'NOT BUDGE.", "FORCE FIELD STOPS YOU.", "THERE IS NO REPLY.".'),
    (0x12437, "chainToTown", '-> TWNDR.EXE (town exhibit).'),
    (0x12445, "chainToDungeon", '-> DUN.EXE (dungeon exhibit).'),
    (0x12490, "chainToStory", '-> STDRV.EXE (story driver).'),
    (0x1249E, "chainToCel", '-> CELDRV.EXE (cel-animation player).'),
    (0x1271A, "caretakerDialog", '"THE CARETAKER WANTS TO ".'),
    (0x12B64, "caretakerPraise", '"YOU HAVE DONE WELL SINCE I ".'),
    (0x12967, "loadExhibitData", 'loads an exhibit .BSV.'),
    (0x12BBF, "loadMuseumData", 'loads MUSDATA.BSV.'),

    # exhibit name / description setters (SELECT CASE on exhibit id)
    (0x10F9B, "exhibitName_artifact", '"ANCIENT ARTIFACT".'),
    (0x10FCD, "exhibitName_weaponry", '"THE ANCIENT / ART OF WEAPONRY".'),
    (0x10FF2, "exhibitName_thornberry",
     '"THORNBERRY", "A TYPICAL TOWN OF TARMALON".'),
    (0x11032, "exhibitName_herbOfLife", '"THE / HERB OF LIFE".'),
    (0x11057, "exhibitName_pirateTreasure", '"PIRATE TREASURE".'),
    (0x11072, "exhibitName_nativeCurrency", '"NATIVE CURRENCY".'),
    (0x1108D, "exhibitName_stonesOfWisdom", '"STONES OF WISDOM".'),
    (0x110F5, "exhibitName_testForKnights", '"TEST FOR KNIGHTS".'),
    (0x1113F, "exhibitName_guardian", '"THE / GUARDIAN".'),
    (0x11164, "exhibitName_fourJewels", '"THE / FOUR JEWELS".'),
    (0x11189, "exhibitName_flightOfFancy", '"FLIGHT OF FANCY".'),
    (0x11A45, "exhibitName_fourJewelDungeon", '"THE FOUR JEWEL DUNGEON".'),
    (0x11310, "exhibitName_piratesLair", '"THE PIRATE\'S LAIR".'),

    # --- 2nd pass: from the ds: engine state vars (apply_dsvars_mus.py) ---
    (0x10D16, "testExhibitFlag",
     'test whether the bits in flagTestMask are set in the museum flag '
     'word ([ds:1B96 desc + 0x16]); leaves the result in '
     'flagTestResult.'),
    (0x10B7B, "checkFlag_03", "flagTestMask := 0x03, then testExhibitFlag."),
    (0x10C57, "checkFlag_2B", "flagTestMask := 0x2B, then testExhibitFlag."),
    (0x10C9C, "checkFlag_D0", "flagTestMask := 0xD0, then testExhibitFlag."),
    (0x10CA5, "checkFlag_0300", "flagTestMask := 0x0300, then testExhibitFlag."),
    (0x10CEA, "checkFlag_0800", "flagTestMask := 0x0800, then testExhibitFlag."),
    (0x1143F, "checkFlag_2000", "flagTestMask := 0x2000, then testExhibitFlag."),
    (0x1204B, "showExhibitResult",
     'display the message string indexed by ds:1ADC (from the array at '
     'the ds:1D38 descriptor) and add to hitPoints. TENTATIVE.'),
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
