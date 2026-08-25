"""
IDA Pro script: brings the `Savegame` (and `Creature`) struct definitions
up to date and in sync across ULTIMA.EXE / GEN.EXE / OUT.EXE / SPACE.EXE.

Context: each of the 4 executables that reference `_savegame` defined
this struct independently (no shared type library across IDBs), so the
4 copies drifted -- some fields were named `_equippedWeapon` in one IDB
and the more game-accurate `_readyWeapon` in another (SPACE.EXE had
already worked out `_shipFuel`/`_shipShield` for two fields the other
3 IDBs left as `field_B0`/`field_B2`), and a few names were actively
unclear (`_overworldWidgets`, `_quests`, the `_X_array` fields -- see
notes below). This script is the single source of truth for the
*renames*; run it once per IDB (GEN/OUT/SPACE -- see
apply_structs_savegame_ultima.py for ULTIMA.EXE, whose copy was
additionally offset-misaligned and needed real surgery, not just
renames).

Findings that justify the less-obvious renames (full writeup in
docs/overview.md):

- `_overworldWidgets` -> `_overworldEntities`: NOT purely a monster
  list (the user's first guess) -- confirmed by code that both (a)
  compares `_type` against `TILE_FIRST_MONSTER` to decide "is this
  actually a monster" (implying non-monster entries exist), and (b)
  `exitLocation` re-adds the SAME widget (a ship/transport, `_data` =
  the tile that was underneath it) back onto the overworld at the
  player's position when leaving a town/dungeon. "Entities" covers
  both; "monsters" would have been actively wrong.
- `field_AA` -> `_overworldEntityCount`: confirmed via both
  `saveGame` and `writeInUseAndExit`, which both do
  `_savegame.field_AA = _creaturesCount` immediately before writing
  the whole struct to disk, and the load path in OUT.EXE's startup
  restoring `_creaturesCount = _savegame.field_AA` right after loading
  the town map -- a save/restore pair for the live entity-count global.
- `field_B0`/`field_B2` -> `_shipFuel`/`_shipShield`: already
  correctly named in SPACE.EXE's independent copy; just propagating
  those names to the 2 IDBs that still had them as `field_B0`/`B2`.
- `_equippedWeapon`/`_equippedSpell`/`_equippedArmor` ->
  `_readyWeapon`/`_readySpell`/`_readyArmor`: standardizing on
  SPACE.EXE's existing names, which match the game's own command
  terminology (the `R`eady command, named `ready` in every executable
  that has it) rather than the more generic "equipped".
- `_quests` -> `_questStatus`: it's a 9-word array of per-castle quest
  STATE (-1 = not offered, 1 = accepted, 0 = reward claimed), indexed
  by `_castleIndex`, not a list of quest objects.
- `_armor_array`/`_weapons_array`/`_spells_array`/`_transports_array`
  -> `_armorSlot0`/`_weaponSlot0`/`_spellSlot0`/`_transportSlot0`:
  these are genuinely distinct 2-byte memory slots 2 bytes *before*
  the first individually-named item in each category (e.g.
  `_armor_array` at 0x54, `_leatherArmor` at 0x56) -- not, as the old
  "_array" name implied, the base of an array whose first element is
  one of the named fields. What item (if any) this slot 0 actually
  represents is NOT confirmed -- `dropPenceCastle`'s random weapon-
  boost event picks `getRandomNumber(1, 15)`, skipping index 0, which
  is consistent with either "index 0 is unused padding" (plausible
  1-based-BASIC-port artifact, item ID 0 reserved as a "nothing"
  sentinel elsewhere in the game, e.g. MONDAIN.EXE's
  selectedSpellIndex) or "index 0 is a real item this one random event
  just doesn't grant". Renamed to remove the actively-misleading
  "_array" implication without asserting an unconfirmed identity --
  still an open question, see docs/roadmap.md.

Left alone (confirmed genuinely unreferenced by any instruction in
OUT.EXE -- no `Creature.field_A`/`field_C`/`field_E` struct-relative
access anywhere in the disassembly, unlike every other Creature field):
`Creature.field_A`, `field_C`, `field_E` -- 6 bytes of the 16-byte
Creature slot with no confirmed purpose, most likely padding written
once and never read, but not asserted as such.

    .\\run_ida_script.ps1 -Idb ultima1_gen -ScriptName apply_structs_savegame.py -NoExport
    .\\run_ida_script.ps1 -Idb ultima1_out -ScriptName apply_structs_savegame.py -NoExport
    .\\run_ida_script.ps1 -Idb ultima1_space -ScriptName apply_structs_savegame.py -NoExport
"""

import idc

DRY_RUN = False

# (offset, new_name, note)
SAVEGAME_RENAMES = [
    (0x2A, "_readyWeapon", "standardized on SPACE.EXE's name -- matches the `ready` [R] command's own terminology."),
    (0x2C, "_readySpell", "see _readyWeapon."),
    (0x2E, "_readyArmor", "see _readyWeapon."),
    (0x3A, "_questStatus", "9-word per-castle quest state array (-1/1/0), not a list of quest objects."),
    (0x54, "_armorSlot0", "distinct 2-byte slot before _leatherArmor -- item identity unconfirmed, see module docstring."),
    (0x60, "_weaponSlot0", "distinct 2-byte slot before _dagger -- item identity unconfirmed, see module docstring."),
    (0x80, "_spellSlot0", "distinct 2-byte slot before _open -- item identity unconfirmed, see module docstring."),
    (0x96, "_transportSlot0", "distinct 2-byte slot before _horse -- item identity unconfirmed, see module docstring."),
    (0xAA, "_overworldEntityCount", "saved/restored alongside _moveCount in saveGame/writeInUseAndExit/startup load, mirrors the live _creaturesCount global."),
    (0xB0, "_shipFuel", "propagated from SPACE.EXE's independently-correct name."),
    (0xB2, "_shipShield", "propagated from SPACE.EXE's independently-correct name."),
    (0xB4, "_overworldEntities", "NOT purely monsters -- see module docstring. Array of 40 Creature."),
]


def apply_savegame_renames():
    sid = idc.get_struc_id("Savegame")
    if sid == idc.BADADDR:
        print("[!] Savegame struct not found in this IDB -- skipping")
        return
    for off, new_name, note in SAVEGAME_RENAMES:
        cur = idc.get_member_name(sid, off)
        if cur == new_name:
            print(f"Savegame+{off:#04x}: already {new_name!r} -- skipping")
            continue
        if cur is None:
            # no member defined at this offset at all (a real gap in
            # the struct, e.g. GEN.EXE never got field_B0/field_B2
            # defined in the first place) -- create it instead of
            # renaming.
            print(f"Savegame+{off:#04x}: <no member> -> {new_name!r} (creating)")
            print(f"    {note}")
            if DRY_RUN:
                continue
            ok = idc.add_struc_member(sid, new_name, off, idc.FF_DATA | idc.FF_WORD, -1, 2)
            if ok != 0:
                print(f"    [!] add_struc_member FAILED (code {ok})")
            continue
        print(f"Savegame+{off:#04x}: {cur!r} -> {new_name!r}")
        print(f"    {note}")
        if DRY_RUN:
            continue
        ok = idc.set_member_name(sid, off, new_name)
        if not ok:
            print("    [!] rename FAILED")


def fix_movecount_type():
    """_moveCount should be a single 4-byte dword at 0xAC -- some IDBs
    (GEN.EXE) still have it split as a 2-byte _moveCount + a separate
    2-byte field_AE. Only touches it if that split pattern is present;
    leaves it alone if already a proper 4-byte member (OUT.EXE,
    SPACE.EXE) or if the offset holds something else entirely."""
    sid = idc.get_struc_id("Savegame")
    if sid == idc.BADADDR:
        return
    name_ac = idc.get_member_name(sid, 0xAC)
    size_ac = idc.get_member_size(sid, 0xAC)
    name_ae = idc.get_member_name(sid, 0xAE)
    if name_ac == "_moveCount" and size_ac == 2 and name_ae and name_ae.startswith("field_"):
        print(f"Savegame+0xAC: _moveCount is 2 bytes with {name_ae!r} at +0xAE -- merging into a 4-byte dword")
        if not DRY_RUN:
            ok = idc.del_struc_member(sid, 0xAE)
            if not ok:
                print("    [!] del_struc_member(+0xAE) FAILED")
            ok = idc.del_struc_member(sid, 0xAC)
            if not ok:
                print("    [!] del_struc_member(+0xAC) FAILED")
            ok = idc.add_struc_member(sid, "_moveCount", 0xAC, idc.FF_DATA | idc.FF_DWORD, -1, 4)
            if ok != 0:
                print(f"    [!] add_struc_member(+0xAC, dword) FAILED (code {ok})")
    else:
        print(f"Savegame+0xAC: {name_ac!r} size={size_ac} -- already fine, leaving alone")


# Function(s) referencing the renamed field, worth renaming for consistency.
FUNCTION_RENAMES = [
    ("addOverworldWidget", "addOverworldEntity",
     "consistency with Savegame._overworldEntities -- adds an entry to that array (monster, or a ship/marker left behind), not always a monster."),
]


def apply_function_renames():
    for cur_name, new_name, note in FUNCTION_RENAMES:
        ea = idc.get_name_ea_simple(cur_name)
        if ea == idc.BADADDR:
            print(f"[!] function {cur_name!r} not found in this IDB -- skipping")
            continue
        cur = idc.get_name(ea)
        if cur == new_name:
            print(f"{ea:X}: already {new_name!r} -- skipping")
            continue
        print(f"{ea:X}: {cur!r} -> {new_name!r}")
        print(f"    {note}")
        if not DRY_RUN:
            ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
            if not ok:
                print("    [!] rename FAILED")


def main():
    apply_savegame_renames()
    fix_movecount_type()
    apply_function_renames()
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names took.")


if __name__ == "__main__":
    main()
