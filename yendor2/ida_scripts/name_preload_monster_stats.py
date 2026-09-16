"""
Names sub_124EC, called once from InitGame: allocates a large
(0x49A-paragraph = ~18.4KB) memory block, wires it into a FileEntry
via sub_27CFE, and reads into it with errorCode=9 -- the same
errorCode used by LoadClueBookMonsterEntry (confirmed reading
WORLD.DAT block 0x32, "MONSTER STATISTICS") and BuildMonsterDisplayName,
but here with a much larger buffer allocated once at startup rather
than per-entry, plausibly preloading the whole monster stats table
into memory for the rest of the game to reference. -> PreloadMonsterStatsTable

Run via:
    .\run_ida_script.ps1 name_preload_monster_stats.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x124EC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PreloadMonsterStatsTable", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PreloadMonsterStatsTable': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Allocates a large (~18.4KB) block and reads into it with "
    "errorCode=9 -- the same code LoadClueBookMonsterEntry uses for "
    "WORLD.DAT block 0x32 (MONSTER STATISTICS), but with a much larger "
    "one-time buffer, plausibly preloading the whole table. Called "
    "once from InitGame.",
    False,
)
