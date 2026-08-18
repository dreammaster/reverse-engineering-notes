"""
IDA Pro script: splits the _mapMonsters struct-of-arrays blob into 8
individually-named 32-byte sub-arrays, one per confirmed monx?? field.

Background: monx?? is loaded as one 256-byte buffer into _mapMonsters,
but it's not one struct per monster -- each FIELD is its own 32-byte
array (one byte per monster slot 0-31), with the slot index used as a
common `di`-relative index across every field. IDA only knows about
`_mapMonsters` itself (currently declared as a flat 0xBC=188-byte
array, undersized vs. the true 256 bytes) plus computed offsets like
`(_mapMonsters+60h)[di]` for the rest -- there's no way to give these
sub-arrays their own names via apply_structs.py's normal struct-member
ops, since this is a collection of independent parallel globals, not
one struct type (see that script's docstring).

CORRECTION (2026-08-18) to an earlier session's docs: file-formats.md
used to describe 8 additional "runtime-only, not saved to disk" fields
at _mapMonsters+0x137 through +0x217. That framing was a
misunderstanding -- _mapMonsters itself sits at SEGMENT-RELATIVE offset
0x137 (confirmed: segment sg08e3 starts at 0x17410, _mapMonsters is at
0x17547, difference = 0x137), and every one of those "runtime" offsets
is exactly _mapMonsters + {0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0,
0xE0} -- the SAME 256-byte buffer, just accessed via raw segment-
literal displacements (e.g. `[di+1D7h]`) in some code paths instead of
the symbolic `(_mapMonsters+A0h)[di]` form used elsewhere. There is no
separate unsaved region. Verified directly: `[di+1D7h]` (=
_mapMonsters+0xA0) gates the same shopkeeper/offer-flag branch in
`transact` as the already-documented Offer flag; `[di+1F7h]`/
`[di+217h]` (= _mapMonsters+0xC0/+0xE0) hold a temp copy of
_playerX/_playerY during a position swap, later copied into
_mapMonsters[di]. Two other claimed offsets (+0x177 cooldown, +0x1B7
cached-tile) don't appear anywhere in the current .asm and were
dropped as unverifiable. Net result: the entire 256-byte buffer is
exactly 8 fields x 32 bytes, no gaps. See docs/file-formats.md for the
updated writeup.

This script creates a properly-sized (32-byte) named byte array at
each field's address, so instructions naturally render as
`_monsterType[di]` instead of `(_mapMonsters+60h)[di]`. Offset +0x00
keeps the existing `_mapMonsters` name (already used pervasively
throughout the codebase and docs as "Map X") -- only resized to 32
bytes for consistency with its 7 siblings, not renamed.

USAGE: dry-run first (structural/graph-surgery script convention, same
as resolve_command_jump_table.py and the fix_*.py scripts) -- back up
the idb first, run once with DRY_RUN=True, review, then flip to False.
"""

import idaapi
import idc
import ida_bytes

DRY_RUN = False

FIELD_SIZE = 32  # one byte per monster slot, 32 slots (0-31)

# (offset_from_mapMonsters_base, new_name, note)
FIELDS = [
    (0x00, "_mapMonsters",
     "Map X position. Already the established name for offset 0 --"
     " resized to exactly 32 bytes here (was a flat 0xBC=188-byte"
     " array covering all 6 original on-disk fields at once), not"
     " renamed. Evidence: sub_1330C/find_cursor_target_monster"
     " matches it against _mapX+_mapLeft when locating the monster"
     " under the cursor for cast."),

    (0x20, "_monsterMapY",
     "Map Y position. Evidence: same routine, _mapY+_mapTop against"
     " this field."),

    (0x40, "_monsterSpellHP",
     "Magic/spell HP. cast subtracts a computed spell-damage value"
     " from it and checks for borrow (death) -- separate pool from"
     " the runtime melee HP. Same borrowless-subtract idiom as"
     " player._hp."),

    (0x60, "_monsterType",
     "Monster type = TileId x4. attack's post-kill dispatch and"
     " transact's flavor-line dispatch (\"A GUARD SAYS: PAY YOUR"
     " TAXES!\"=0x60, \"A JESTER SINGS...\"=0x64, \"A MERCHANT"
     " SAYS...\"=0x68, plus 0xF0/0xF4/0xF8/0xFC) both confirm exact"
     " TileId x4 matches (Minax/Guard/Thief/Fighter)."),

    (0x80, "_monsterGlyphTile",
     "Display/glyph tile ID. Written directly onto the map as a tile"
     " byte after a non-lethal hit; also compared against a caller-"
     " supplied value in find_cursor_target_monster (a 'match this"
     " specific glyph' parameter for cast's detection). Likely a"
     " companion/duplicate of _monsterType rather than something"
     " fully distinct -- both get zeroed together on death."),

    (0xA0, "_monsterOfferFlag",
     "Dual-use field, confirmed via 2 independent call sites: offer"
     " reads it as an offer-type (values 0x81/0x82/0x83, high bit +"
     " 1-3 index) gating a location-specific gold-offer script;"
     " transact checks the same high bit (cmp al,80h) to decide"
     " 'is this NPC a shopkeeper', and if not, falls through to the"
     " _monsterType flavor-line dispatch. Was mistakenly documented"
     " as two separate fields (on-disk 'Offer flag' + a supposed"
     " runtime-only 'shopkeeper flag' at +0x1D7) before the segment-"
     " offset correction above -- it's one field, accessed via two"
     " different addressing idioms in different code paths."),

    (0xC0, "_monsterTempX",
     "Temporary scratch, not persistent monster data: holds a saved"
     " copy of _playerX during a position swap (asm ~2259, 2272-2274"
     " -- 'mov al,_playerX / mov [di+1F7h],al' then later 'mov"
     " al,[di+1F7h] / mov _playerX,al / mov _mapMonsters[di],al')."
     " Was documented as an unmapped/unreferenced byte range before"
     " the segment-offset correction above."),

    (0xE0, "_monsterTempY",
     "Temporary scratch for _playerY, same swap pattern as"
     " _monsterTempX (asm ~2260-2261, 2275+)."),
]


def get_base_ea():
    ea = idc.get_name_ea_simple("_mapMonsters")
    if ea == idaapi.BADADDR:
        print("[!] _mapMonsters not found -- has it been renamed?")
        return None
    return ea


def split_field(base_ea, offset, name, note):
    ea = base_ea + offset
    cur_name = idc.get_name(ea)
    cur_size = ida_bytes.get_item_size(ea) if cur_name else 0
    if cur_name == name and cur_size == FIELD_SIZE:
        print(f"{ea:X} (+{offset:X}): already {name!r}, {FIELD_SIZE} bytes -- skipping")
        return

    print(f"{ea:X} (+{offset:X}): {cur_name!r} ({cur_size} bytes) -> "
          f"{name!r} ({FIELD_SIZE} bytes)")
    print(f"    {note}")
    if DRY_RUN:
        return

    if not ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, FIELD_SIZE):
        print(f"    [!] del_items reported failure at {ea:X} (may be harmless)")
    if not ida_bytes.create_data(ea, ida_bytes.FF_BYTE, FIELD_SIZE, idaapi.BADADDR):
        print(f"    [!] create_data FAILED at {ea:X}")
        return
    if idc.get_name(ea) != name:
        if not idc.set_name(ea, name, idc.SN_NOWARN):
            print(f"    [!] set_name FAILED at {ea:X}")


def main():
    base_ea = get_base_ea()
    if base_ea is None:
        return
    print(f"_mapMonsters base = {base_ea:X}")

    for offset, name, note in FIELDS:
        split_field(base_ea, offset, name, note)

    mode = "DRY RUN -- nothing changed" if DRY_RUN else "applied"
    print(f"\nDone ({mode}).")
    if DRY_RUN:
        print("Set DRY_RUN = False to apply.")
    else:
        print("Re-export the .asm/.idc and check the new names took, "
              "then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
