"""
IDA Pro script: master list of symbol renames (functions + globals) for
EXODUS.BIN (DOS) -- Ultima III's overworld/combat/dungeon engine,
chained into from BOOTUP.BIN's handleJourneyOnward via an indirect jump
through a self-describing entry vector (see docs/overview.md). Companion
to apply_renames_ultima.py/apply_renames_bootup.py -- see those files'
docstrings for the full naming/DRY_RUN convention writeup. Run against
ultima_exodus.idb:

    .\\run_ida_script.ps1 -Idb ultima_exodus -ScriptName apply_renames_exodus.py

IMPORTANT: this executable's real entry point (0x124C4, the target of
BOOTUP.BIN's `JMP WORD PTR [1328h]`) was NOT recognized as code or a
function by IDA's own auto-analysis when this IDB was first created --
nothing in EXODUS.BIN's own bytes calls it, so IDA had no reason to
treat that address as anything but data. It had to be manually converted
to code (idc.plan_and_wait) and given a function boundary
(ida_funcs.add_func) before it could even be named -- done once,
directly, before this script existed; the entry here is just for the
changelog/idempotent-skip record.

This is a MUCH larger executable than ULTIMA.COM/BOOTUP.BIN (144
functions vs. 58/73, ~14,600 asm lines) and turns out to be the actual
game-world engine (overworld movement, towns, dungeons, combat, spells,
shops, temples, the ending sequence -- see the string table dump in
docs/overview.md's findings log). This first pass only covers the
shared low-level runtime (confirmed byte-for-byte identical in
structure to the same-named functions in ultima.idb/ultima_bootup.idb,
verified by reading through this whole cluster line-by-line, not
assumed from addresses alone) -- the game-specific logic (a handful of
genuinely massive functions like sub_17B54, the overworld command
dispatcher, spanning thousands of bytes) is a separate, much larger
follow-up pass. See docs/roadmap.md.
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x124C4, "entryFromBootup",
     "the real entry point once BOOTUP.BIN's handleJourneyOnward chains "
     "in -- reached via an indirect jump through a vector at file "
     "offset 0x1228 (absolute 0x1328 once loaded over BOOTUP.BIN's "
     "memory), not by falling through to offset 0 the way ULTIMA.COM "
     "and BOOTUP.BIN's own entries work. Had no proc boundary at all "
     "until manually created (see this file's module docstring). Loads "
     "CHARSET.ULT/SHAPES.ULT/PARTY.ULT and continues into the game "
     "proper -- same boot-sequence shape as the other two executables' "
     "entry routines (same INT 23h/24h vector patching, same stack "
     "setup), but no decorative title-screen animation this time -- "
     "goes straight into gameplay."),

    # -- shared low-level runtime, byte-for-byte structurally identical
    # to the same-named functions in ultima.idb/ultima_bootup.idb --

    (0x14F3B, "writeString", "identical to the other IDBs' writeString."),
    (0x14F51, "writeStringPreserveCx", "identical."),
    (0x14F57, "swapCursorPos", "identical (xchg dx,word_12A90, this IDB's _textCursorPos)."),
    (0x14F5C, "writeCharacter", "identical."),
    (0x14F90, "drawCharGlyph", "identical."),
    (0x1504D, "plotPixel2bpp", "identical."),
    (0x15072, "drawTileGrid", "identical."),
    (0x150D3, "clearFramebuffer",
     "clears both CGA banks and resets the text cursor. In the other "
     "two IDBs this exact logic is an anonymous orphan block with no "
     "proc boundary; here it's a properly bounded function, so it gets "
     "a name for the first time -- generic descriptive name since no "
     "specific caller context narrows it further."),
    (0x150FD, "readLine", "identical."),
    (0x1514B, "stepTimeSeededPrng", "identical."),
    (0x15184, "swapAnimTableRows", "identical."),
    (0x151AA, "updateLogoAnimationA", "identical."),
    (0x151F9, "updateLogoAnimationB", "identical."),
    (0x15271, "updateLogoAnimationC", "identical."),
    (0x152B2, "updateLogoAnimationD", "identical."),
    (0x15311, "runIdleAnimationTick", "identical."),
    (0x15334, "pollKeypressAndAnimate", "identical."),
    (0x15347, "getKeypressAndWaitRaw", "identical."),
    (0x153A2, "printHexWord",
     "identical logic to the printHexWord found (as an orphan) in "
     "ultima.idb and (as a proper function) in ultima_bootup.idb -- "
     "here it's a proper function too."),
    (0x153AD, "printHexByte", "identical."),
    (0x153C6, "printHexNibble", "identical."),
    (0x153D8, "accumulateInputDigit", "identical."),
    (0x15462, "getMenuChoice", "identical to ultima_bootup.idb's getMenuChoice."),
    (0x154D9, "promptForNumberEntry", "identical to ultima_bootup.idb's promptForNumberEntry."),
    (0x1552A, "saveFile", "identical."),
    (0x1557A, "loadFile", "identical."),
    (0x155CD, "openFileWithRetry", "identical."),
    (0x15608, "printStringAt", "identical to ultima_bootup.idb's printStringAt."),
    (0x15611, "adjustAnimSpeed", "identical."),
    (0x15634, "drawLogoTileGrid",
     "identical mechanism to the other IDBs' drawLogoTileGrid, but its "
     "11x11 source tile-index buffer is `entryFromBootup` itself (the "
     "boot routine's own code bytes, reused as scratch data once it's "
     "run exactly once at startup) rather than a dedicated byte array -- "
     "a real memory-reuse trick worth remembering if entryFromBootup's "
     "own bytes ever look like they've been 'corrupted': they're "
     "deliberately being read as tile-index data after boot, not a "
     "disassembly error."),
    (0x1564F, "computeAnimTableByte", "identical."),
    (0x15674, "updateWindDisplay", "identical mechanism, verified via ida_bytes.get_word() against all 5 WIND_DIRECTION_TABLE entries."),
    (0x156D9, "playSoundEffect", "identical dispatcher."),
    (0x156C0, "_soundEnabled", "identical role."),
    (0x156C1, "SOUND_EFFECT_TABLE",
     "identical mechanism, verified via ida_bytes.get_word() against "
     "all 12 entries. Real address 0x156C1, NOT what IDA's own "
     "auto-generated array name would suggest -- same lesson as the "
     "other two IDBs' equivalent tables, don't trust an auto-name's "
     "numeric suffix as its address."),
    (0x156F9, "playToneFF", "identical."),
    (0x1571B, "playErrorBeep", "identical role (effect 0FEh) -- confirmed call sites: readLine, promptForNumberEntry, getMenuChoice, openFileWithRetry's 'Wrong Diskette!' prompt."),
    (0x1573D, "playToneFD", "identical."),
    (0x15796, "playToneFC", "identical."),
    (0x157BF, "playToneFB", "identical."),
    (0x157DA, "playToneFA", "identical."),
    (0x157F8, "playToneF9", "identical."),
    (0x15802, "playToneF8", "identical."),
    (0x15828, "playToneF7", "identical."),
    (0x15832, "playToneF6", "identical."),
    (0x1585E, "playToneHelper", "identical -- shared helper for playToneF7/playToneF6, not itself in SOUND_EFFECT_TABLE."),
    (0x1588E, "playToneF5", "identical."),

    (0x12AAD, "aCalmWind", "'\\x10Calm  Wind\\x11\\x00' -- wind-direction display string."),
    (0x12ABA, "aNorthWind", "'\\x10North Wind\\x11\\x00' -- wind-direction display string."),
    (0x12AC7, "aSouthWind", "'\\x10South Wind\\x11\\x00' -- wind-direction display string."),
    (0x12AD4, "aEastWind", "'\\x10East  Wind\\x11\\x00' -- wind-direction display string."),
    (0x12AE1, "aWestWind", "'\\x10West  Wind\\x11\\x00' -- wind-direction display string."),
    (0x1566A, "WIND_DIRECTION_TABLE",
     "identical mechanism to the other IDBs' WIND_DIRECTION_TABLE, "
     "same Calm/North/East/South/West selection order -- verified via "
     "ida_bytes.get_word() against all 5 entries, not assumed."),
]


def apply_rename(ea, new_name, note):
    cur = idc.get_name(ea)
    if cur == new_name:
        print(f"{ea:X}: already {new_name!r} -- skipping")
        return
    print(f"{ea:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    if not ok:
        print("    [!] rename FAILED")


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    if not RENAMES:
        print("[no-op] RENAMES is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
