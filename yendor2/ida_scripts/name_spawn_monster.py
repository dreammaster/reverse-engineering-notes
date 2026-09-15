"""
Names sub_22A68, called from sub_212B8 (not traced -- a movement/
trigger handler gated on word_3292C >= 0x11 and a flag bit on some
caller-supplied record). Spawns a new monster into the level's
monster pool:

- Finds an empty slot in g_levelMonsters (base 0xF26, 80 x 0x9C-byte
  records, confirmed via ida_scripts/... earlier this session) --
  scans for [+0] == 0, bails (message via sub_28412) if the pool is
  full.
- Sets the slot's type id ([si] = bx, the monster catalog id passed
  in), loads the monster's catalog record via
  WorldDat_setBlock6/WorldDat_setBlock5 (block math matches
  LoadClueBookMonsterEntry's WORLD.DAT block 0x32).
- Computes a spawn position from a facing-direction offset table
  (0x7090/0x70F6/0x715C/0x71C2, selected by word_36CF5 tier bits --
  the same bits ShowCompassDirection reads), added to the current
  position (word_36CF7/36CF9), plus a pixel-space position ([si+6]).
- Sets a countdown timer ([si+8] = RandomInRange(5) + a template
  value), a display-variant field ([si+0xA], matching
  LoadClueBookMonsterEntry's [+0x92] bit-0 check), and full HP
  ([si+0x10] = [si+0x50], the record's max-HP field).
- Calls sub_233F5 and sub_22C3E (not traced) and looks up a second
  table (0xE4E9) by monster id.

The per-level monster spawn function: creates a new g_levelMonsters
instance ahead of the party in their current facing direction, at
full HP.

-> SpawnMonsterInFacingDirection

Run via:
    .\run_ida_script.ps1 name_spawn_monster.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22A68
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SpawnMonsterInFacingDirection", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SpawnMonsterInFacingDirection': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Spawns a monster (catalog id in ax/bx) into an empty "
    "g_levelMonsters slot: loads its catalog record from WORLD.DAT "
    "(same block math as LoadClueBookMonsterEntry), computes a spawn "
    "position offset from the current facing direction (word_36CF5 "
    "tier bits -- same as ShowCompassDirection) plus current position "
    "(word_36CF7/36CF9), sets a countdown timer (RandomInRange(5) + "
    "template) and full HP ([+0x10]=[+0x50]). Called from sub_212B8 "
    "(a movement/trigger handler, not traced).",
    False,
)
