"""
Names a matched Set/Clear/Test trio operating on a single-bit flag
in a WORLD.DAT-backed packed bitmap (FileEntry record type 0xA,
prepared via sub_27E04 with a fixed "this" pointer 0x8FFB): each
takes a bit index in ax, splits it into byte offset (ax/8) and bit
position (ax%8, as an 0x80>>bit mask), and operates on that bit in
byte_38808 within the loaded record.

sub_22C3E: ORs the mask in (sets the bit), then FileEntry_Write's the
change back. Called from SpawnMonsterInFacingDirection with ax=[si]
(a monster record's own map-cell index) right after spawning a
monster -- marking that cell as having an active monster.
-> SetCellMonsterSpawnedFlag

sub_22BF5: ANDs the inverted mask in (clears the bit), then
FileEntry_Write's the change back. Called from sub_209D2 with the
same ax=[si] cell-index pattern, in the branch taken when a monster
falls outside the tracked visible range -- clearing the flag on
despawn/cleanup. -> ClearCellMonsterSpawnedFlag

sub_22C85: TESTs the bit (read-only, no write-back), called from
TryInteractAtPosition. -> TestCellMonsterSpawnedFlag

Run via:
    .\run_ida_script.ps1 name_cell_monster_spawned_flag_trio.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x22C3E, "SetCellMonsterSpawnedFlag",
     "Sets (ORs in) bit ax of the WORLD.DAT-backed cell-monster-"
     "spawned bitmap (record type 0xA) and writes it back. Called "
     "from SpawnMonsterInFacingDirection with ax=the spawned "
     "monster's own cell index ([si]). Sibling of "
     "ClearCellMonsterSpawnedFlag/TestCellMonsterSpawnedFlag."),
    (0x22BF5, "ClearCellMonsterSpawnedFlag",
     "Clears (ANDs out) bit ax of the same cell-monster-spawned "
     "bitmap and writes it back. Called from sub_209D2 when a "
     "monster falls outside the tracked visible range (despawn/"
     "cleanup)."),
    (0x22C85, "TestCellMonsterSpawnedFlag",
     "Tests (read-only) bit ax of the same cell-monster-spawned "
     "bitmap, no write-back. Called from TryInteractAtPosition."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
