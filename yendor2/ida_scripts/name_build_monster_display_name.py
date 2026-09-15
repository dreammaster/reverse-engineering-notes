"""
Names sub_14B85, called only from BuildClueEntryText and
ShowClueBookMonsterDetail (both in the F8 clue-book monster-stats
cluster; LoadClueBookMonsterEntry's pre-existing comment confirms that
whole cluster reads WORLD.DAT block 0x32, "MONSTER STATISTICS").

Uses the same WorldDat_setBlock5 + FileEntry_Read(errorCode=9,
bx=0x9043) pattern as LoadClueBookMonsterEntry to populate two scratch
buffers (0xAFDA, 0xAFE7), trims trailing spaces off both, then builds
"<0xAFDA> <0xAFE7>" (space-separated, via StpCpy + two StrCat calls,
the literal separator being a single-space string at 0x7960) into
0xAFA8 and returns a pointer to it in bx. Two WORLD.DAT-sourced text
fields joined with a space -- plausibly a monster's name and its
type/category label, matching the "MONSTER STATISTICS" clue-book
context, though the exact field semantics aren't independently
confirmed. -> BuildMonsterDisplayName

Run via:
    .\run_ida_script.ps1 name_build_monster_display_name.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14B85
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "BuildMonsterDisplayName", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'BuildMonsterDisplayName': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Builds '<field1> <field2>' (space-separated) from two WORLD.DAT- "
    "sourced text buffers (0xAFDA, 0xAFE7) into 0xAFA8, returned in bx. "
    "Same WorldDat_setBlock5/FileEntry_Read(errorCode=9) pattern as "
    "LoadClueBookMonsterEntry (WORLD.DAT block 0x32, MONSTER STATISTICS). "
    "Called only from BuildClueEntryText and ShowClueBookMonsterDetail -- "
    "plausibly a monster name + type/category label; exact field "
    "semantics not independently confirmed.",
    False,
)
