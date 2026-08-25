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
  -> **`_skin`/`_hands`/`_prayer`/`_foot`** (2026-08-25, resolved --
  previously landed as the placeholder `_armorSlot0`/`_weaponSlot0`/
  `_spellSlot0`/`_transportSlot0` while the identity was still
  unconfirmed): each category's item-name table in OUT.EXE
  (`ARMOR`/`WEAPONS_LOWERCASE`/`SPELL_NAMES`/`TRANSPORTS`, all
  pre-existing names) has a real, named entry at index 0 --
  `"Skin"`/`"hands"`/`"Prayer"`/`"Foot"` respectively -- confirming
  these aren't padding at all, just the "you have nothing equipped in
  this category" baseline item. Confirmed further for armor:
  `dropArmor`'s menu loop explicitly starts at index 1 (`var_A = 1`),
  skipping index 0 -- you can't drop your own skin. Same table also
  explains `ATTRIBUTES[0] = "Hit Points"`, matching `_hits` being
  index 0 of the attribute array fixed in `apply_structs_gen.py`.

**`Creature.field_A`/`field_C`/`field_E`** (2026-08-25, resolved --
previously left unnamed): confirmed genuinely unused via an exhaustive
scan (not just a symbolic-name grep) of every instruction's raw
operand values in both OUT.EXE and SPACE.EXE for the fixed
array-base-relative immediate that would reach each field through the
`[si]`-indexed access pattern every other Creature field uses (see
`ida_scripts/find_creature_padding_refs.py`) -- zero hits in either
executable. The array is indexed via `shl ax, 4` (multiply by 16)
rather than `imul ax, 10`, which is presumably *why* the padding
exists: the struct's real data is 5 words (10 bytes: type/data/x/y/
hits), padded out to a power-of-2 16 bytes purely so array indexing
can use a fast shift instead of a multiply. Renamed to `_unused1`/
`_unused2`/`_unused3` to reflect that confirmed (not just assumed)
status.

    .\\run_ida_script.ps1 -Idb ultima1 -ScriptName apply_structs_savegame.py -NoExport
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
    (0x54, "_skin", "ARMOR[0] = \"Skin\" -- confirmed via dropArmor's menu loop starting at index 1, skipping this slot."),
    (0x60, "_hands", "WEAPONS_LOWERCASE[0] = \"hands\" -- bare-handed/unarmed."),
    (0x80, "_prayer", "SPELL_NAMES[0] = \"Prayer\"."),
    (0x96, "_foot", "TRANSPORTS[0] = \"Foot\" -- on foot/walking, matches the existing TRANSPORT_WALKING constant."),
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


# (offset, new_name, note) -- Creature struct fields.
CREATURE_RENAMES = [
    (0xA, "_unused1", "confirmed unreferenced by any instruction in OUT.EXE or SPACE.EXE -- see module docstring."),
    (0xC, "_unused2", "see _unused1."),
    (0xE, "_unused3", "see _unused1."),
]


def apply_creature_renames():
    sid = idc.get_struc_id("Creature")
    if sid == idc.BADADDR:
        print("[!] Creature struct not found in this IDB -- skipping")
        return
    for off, new_name, note in CREATURE_RENAMES:
        cur = idc.get_member_name(sid, off)
        if cur == new_name:
            print(f"Creature+{off:#04x}: already {new_name!r} -- skipping")
            continue
        print(f"Creature+{off:#04x}: {cur!r} -> {new_name!r}")
        print(f"    {note}")
        if DRY_RUN:
            continue
        ok = idc.set_member_name(sid, off, new_name)
        if not ok:
            print("    [!] rename FAILED")


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
    apply_creature_renames()
    apply_function_renames()
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names took.")


if __name__ == "__main__":
    main()
