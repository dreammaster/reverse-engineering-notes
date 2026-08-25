"""
IDA Pro script: rebuilds ULTIMA.EXE's `Savegame` struct from scratch to
match the canonical, now-synced layout in GEN.EXE/OUT.EXE/SPACE.EXE
(see apply_structs_savegame.py for how that layout was derived/cleaned
up).

Why a full rebuild instead of a rename pass like the other 3 IDBs:
ULTIMA.EXE's copy of this struct is **completely unused** -- `grep -i
savegame ultima1.asm` returns zero matches, not even the struct type
name applied to a variable anywhere. It's a vestigial definition (never
an actual `_savegame` global in this executable, which makes sense:
ULTIMA.EXE is just the title/attract screen, per
docs/overview.md#ultimaexe-status-complete -- it chains to GEN.EXE
before any savegame data would be touched). Because nothing references
it, its previous state was free to drift: every field from offset
0x20 onward was misaligned by 16 bytes relative to the real layout
(missing wisdom/intelligence/coins/experience/food/_ready{Weapon,Spell,
Armor} entirely), degrading into unnamed `field_XX` placeholders and a
giant unexplored gap from 0xB0 to 0x332. Since no disassembly output
anywhere depends on the old (wrong) layout, deleting and rebuilding it
is safe -- unlike GEN/OUT/SPACE, where a full rebuild would be riskier
than a plain member rename given how much code references these
fields by name.

    .\\run_ida_script.ps1 -Idb ultima1 -ScriptName apply_structs_savegame_ultima.py -NoExport
"""

import ida_struct
import idc

DRY_RUN = False

WORD = idc.FF_DATA | idc.FF_WORD
DWORD = idc.FF_DATA | idc.FF_DWORD

# (offset, name, size, kind) -- kind is "word", "dword", or a struct name
# for a nested/typed member. This is the full canonical Savegame layout
# as of the GEN/OUT/SPACE sync (apply_structs_savegame.py).
FIELDS = [
    (0x00, "_name", 16, "STR15"),
    (0x10, "_race", 2, "word"),
    (0x12, "_class", 2, "word"),
    (0x14, "_sex", 2, "word"),
    (0x16, "_hits", 2, "word"),
    (0x18, "_strength", 2, "word"),
    (0x1A, "_agility", 2, "word"),
    (0x1C, "_stamina", 2, "word"),
    (0x1E, "_charisma", 2, "word"),
    (0x20, "_wisdom", 2, "word"),
    (0x22, "_intelligence", 2, "word"),
    (0x24, "_coins", 2, "word"),
    (0x26, "_experience", 2, "word"),
    (0x28, "_food", 2, "word"),
    (0x2A, "_readyWeapon", 2, "word"),
    (0x2C, "_readySpell", 2, "word"),
    (0x2E, "_readyArmor", 2, "word"),
    (0x30, "_transportType", 2, "word"),
    (0x32, "_randomSeed", 2, "word"),
    (0x34, "_position", 4, "Point"),
    (0x38, "_soundOn", 2, "word"),
    (0x3A, "_questStatus", 18, "raw18"),  # 9-word array; kept as a plain
                                           # 18-byte member to match how
                                           # GEN/OUT/SPACE already have it
    (0x4C, "_redGems", 2, "word"),
    (0x4E, "_greenGems", 2, "word"),
    (0x50, "_blueGem", 2, "word"),
    (0x52, "_whiteGem", 2, "word"),
    (0x54, "_armorSlot0", 2, "word"),
    (0x56, "_leatherArmor", 2, "word"),
    (0x58, "_chainmail", 2, "word"),
    (0x5A, "_plateMail", 2, "word"),
    (0x5C, "_vacuumSuit", 2, "word"),
    (0x5E, "_reflectSuit", 2, "word"),
    (0x60, "_weaponSlot0", 2, "word"),
    (0x62, "_dagger", 2, "word"),
    (0x64, "_mace", 2, "word"),
    (0x66, "_axe", 2, "word"),
    (0x68, "_ropeSpikes", 2, "word"),
    (0x6A, "_sword", 2, "word"),
    (0x6C, "_greatSword", 2, "word"),
    (0x6E, "_bow", 2, "word"),
    (0x70, "_amulet", 2, "word"),
    (0x72, "_wand", 2, "word"),
    (0x74, "_staff", 2, "word"),
    (0x76, "_triangle", 2, "word"),
    (0x78, "_pistol", 2, "word"),
    (0x7A, "_lightSword", 2, "word"),
    (0x7C, "_phazor", 2, "word"),
    (0x7E, "_blaster", 2, "word"),
    (0x80, "_spellSlot0", 2, "word"),
    (0x82, "_open", 2, "word"),
    (0x84, "_unlock", 2, "word"),
    (0x86, "_magicMissile", 2, "word"),
    (0x88, "_steal", 2, "word"),
    (0x8A, "_ladderDown", 2, "word"),
    (0x8C, "_ladderUp", 2, "word"),
    (0x8E, "_blink", 2, "word"),
    (0x90, "_create", 2, "word"),
    (0x92, "_destroy", 2, "word"),
    (0x94, "_kill", 2, "word"),
    (0x96, "_transportSlot0", 2, "word"),
    (0x98, "_horse", 2, "word"),
    (0x9A, "_cart", 2, "word"),
    (0x9C, "_raft", 2, "word"),
    (0x9E, "_frigate", 2, "word"),
    (0xA0, "_aircar", 2, "word"),
    (0xA2, "_shuttle", 2, "word"),
    (0xA4, "_timeMachine", 2, "word"),
    (0xA6, "_enemyVessels", 2, "word"),
    (0xA8, "_signMarker", 2, "word"),
    (0xAA, "_overworldEntityCount", 2, "word"),
    (0xAC, "_moveCount", 4, "dword"),
    (0xB0, "_shipFuel", 2, "word"),
    (0xB2, "_shipShield", 2, "word"),
    (0xB4, "_overworldEntities", 640, "Creature*40"),
]

TOTAL_SIZE = 0x334


def ensure_creature_struct():
    sid = ida_struct.get_struc_id("Creature")
    if sid != idc.BADADDR:
        print("Creature struct already exists -- leaving as-is")
        return sid
    print("Creature struct missing -- creating (matches GEN/OUT/SPACE layout)")
    if DRY_RUN:
        return idc.BADADDR
    sid = idc.add_struc(-1, "Creature", 0)
    for off, name in [(0x0, "_type"), (0x2, "_data"), (0x4, "_x"), (0x6, "_y"), (0x8, "_hits")]:
        idc.add_struc_member(sid, name, off, WORD, -1, 2)
    for off in (0xA, 0xC, 0xE):
        idc.add_struc_member(sid, f"field_{off:X}", off, WORD, -1, 2)
    return sid


def rebuild_savegame():
    old_sid = ida_struct.get_struc_id("Savegame")
    if old_sid != idc.BADADDR:
        cur_size = ida_struct.get_struc_size(old_sid)
        print(f"Deleting existing Savegame struct (was {cur_size:#x} bytes, unused in this IDB's code)")
        if not DRY_RUN:
            idc.del_struc(old_sid)

    if DRY_RUN:
        print("[dry] would recreate Savegame from scratch here -- skipping actual field-by-field dry listing for brevity")
        return

    sid = idc.add_struc(-1, "Savegame", 0)
    point_sid = ida_struct.get_struc_id("Point")
    creature_sid = ida_struct.get_struc_id("Creature")

    for off, name, size, kind in FIELDS:
        if kind == "word":
            ok = idc.add_struc_member(sid, name, off, WORD, -1, 2)
        elif kind == "dword":
            ok = idc.add_struc_member(sid, name, off, DWORD, -1, 4)
        elif kind == "raw18":
            # single 18-byte member, matching how GEN/OUT/SPACE already
            # have it (not split into individual words there either).
            ok = idc.add_struc_member(sid, name, off, idc.FF_DATA | idc.FF_BYTE, -1, 18)
        elif kind == "STR15":
            str15_sid = ida_struct.get_struc_id("STR15")
            ok = idc.add_struc_member(sid, name, off, idc.FF_DATA | idc.FF_STRUCT, str15_sid, size)
        elif kind == "Point":
            ok = idc.add_struc_member(sid, name, off, idc.FF_DATA | idc.FF_STRUCT, point_sid, size)
        elif kind == "Creature*40":
            ok = idc.add_struc_member(sid, name, off, idc.FF_DATA | idc.FF_STRUCT, creature_sid, size)
        else:
            raise ValueError(kind)
        if ok != 0:
            print(f"  [!] add_struc_member({name!r} @ {off:#04x}) FAILED (code {ok})")
        else:
            print(f"  {off:#04x}  {name}  ({size} bytes, {kind})")

    final_size = ida_struct.get_struc_size(sid)
    if final_size != TOTAL_SIZE:
        print(f"[!] final size {final_size:#x} != expected {TOTAL_SIZE:#x} -- check for gaps/overlaps")
    else:
        print(f"Savegame rebuilt: {final_size:#x} bytes, matches canonical size.")


def main():
    ensure_creature_struct()
    rebuild_savegame()
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new struct took.")


if __name__ == "__main__":
    main()
