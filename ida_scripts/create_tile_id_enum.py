"""
IDA Pro script: create a `TileId` enum (0-63) for the overworld/town
tile IDs, so future disassembly work (and the eventual C++ port) can
reference tiles by name instead of a raw magic number.

Companion to label_tile_graphics.py, which named the *graphics* data
(tile_00..tile_63, the pixel bytes) but not the *tile ID values*
themselves (the numbers 0-63 that show up as literals all over
canMoveToTile, draw_map, etc. -- note the on-disk/in-map byte is that
ID x4, see docs/file-formats.md's "divide by 4" note; this enum is the
real 0-63 ID, not the x4-encoded stored byte). Paul explicitly does not
want the tile_NN graphics-array names changed to semantic names -- this
enum is the intended place for semantic tile names to live instead.

Source: https://moddingwiki.shikadi.net/wiki/Ultima_II_Map_Format's
tile ID table, fetched and reproduced verbatim (no guessing at gaps) on
2026-08-17. Cross-checked against our own pixel-art decode for several
entries and all matched: ID 0 (Water) and 1 (Swamp) show plausible
ripple/marsh dash patterns, ID 6 (Town) is an unambiguous twin-tower
castle icon, and ID 40 ("I / Door") renders as an exact serif capital-I
glyph (solid top/bottom bars, thin stem) -- strong independent
confirmation the wiki table lines up with this port's actual tile
order, not just the Apple II original.

Two caveats baked into the names below, confirmed with Paul rather than
guessed:
  - IDs 5 and 27 have no name on the wiki page ("?") -- kept as
    TILE_UNKNOWN_5 / TILE_UNKNOWN_27 rather than invented names.
  - The letter tiles (32-39, 41-47, 49-57) have gaps -- no Q, and 40/48
    are Door/Moongate instead of I/... -- Paul confirmed this is correct,
    intentional in the original (the game's in-town sign lettering never
    needed the full alphabet), not a fetch error.

Idempotent: if TileId already exists, only missing members are added;
existing ones are left alone (so it's safe to extend TILE_IDS below
and re-run without disturbing anything already applied).

Uses `ida_enum`/`ida_bytes` directly rather than `idc` for the
enum-specific calls -- IDA 8.3 has already dropped several `idc`
compatibility wrappers this project has hit (`idc.get_fileregion_ea`,
`idc.hexflag`; see project memory), and in both cases the real
function was fine in its owning module, just not re-exported through
`idc` anymore. Going straight to `ida_enum`/`ida_bytes` sidesteps that
whack-a-mole instead of fixing one dropped wrapper at a time.
"""

import ida_enum
import ida_bytes
import idaapi

DRY_RUN = False

ENUM_NAME = "TileId"
ENUM_COMMENT = (
    "Overworld/town tile IDs, 0-63. Source: ModdingWiki's Ultima II Map "
    "Format page, cross-checked against this port's actual tile_NN "
    "graphics (see docs/file-formats.md). NOTE: the byte stored on disk "
    "in mapx?? files is this ID x4, not the raw ID -- see the "
    "\"divide by 4\" note in docs/file-formats.md before comparing "
    "against a raw map byte."
)

# (value, enum member name)
TILE_IDS = [
    (0, "TILE_WATER"),
    (1, "TILE_SWAMP"),
    (2, "TILE_GRASS"),
    (3, "TILE_FOREST"),
    (4, "TILE_MOUNTAIN"),
    (5, "TILE_UNKNOWN_5"),
    (6, "TILE_TOWN"),
    (7, "TILE_TOWER"),
    (8, "TILE_CASTLE"),
    (9, "TILE_DUNGEON_ENTRANCE"),
    (10, "TILE_SIGNPOST"),
    (11, "TILE_SEA_MONSTER"),
    (12, "TILE_ORC"),
    (13, "TILE_DAEMON"),
    (14, "TILE_DEVIL"),
    (15, "TILE_BALRON"),
    (16, "TILE_MINAX"),
    (17, "TILE_HORSE"),
    (18, "TILE_SHIP"),
    (19, "TILE_AIRPLANE"),
    (20, "TILE_ROCKET"),
    (21, "TILE_SHIELD"),
    (22, "TILE_SWORD"),
    (23, "TILE_FORCEFIELD"),
    (24, "TILE_GUARD"),
    (25, "TILE_JESTER"),
    (26, "TILE_SHOPKEEP"),
    (27, "TILE_UNKNOWN_27"),
    (28, "TILE_ROAD"),
    (29, "TILE_EMPTY"),
    (30, "TILE_WALL"),
    (31, "TILE_COUNTER_EMPTY"),
    (32, "TILE_LETTER_A"),
    (33, "TILE_LETTER_B"),
    (34, "TILE_LETTER_C"),
    (35, "TILE_LETTER_D"),
    (36, "TILE_LETTER_E"),
    (37, "TILE_LETTER_F"),
    (38, "TILE_LETTER_G"),
    (39, "TILE_LETTER_H"),
    (40, "TILE_DOOR"),          # wiki: "I / Door" -- shares glyph with letter I in sign-drawing font
    (41, "TILE_LETTER_J"),
    (42, "TILE_LETTER_K"),
    (43, "TILE_LETTER_L"),
    (44, "TILE_LETTER_M"),
    (45, "TILE_LETTER_N"),
    (46, "TILE_LETTER_O"),
    (47, "TILE_LETTER_P"),
    (48, "TILE_MOONGATE"),      # no Q tile -- intentional, see docstring
    (49, "TILE_LETTER_R"),
    (50, "TILE_LETTER_S"),
    (51, "TILE_LETTER_T"),
    (52, "TILE_LETTER_U"),
    (53, "TILE_LETTER_V"),
    (54, "TILE_LETTER_W"),
    (55, "TILE_LETTER_X"),
    (56, "TILE_LETTER_Y"),
    (57, "TILE_LETTER_Z"),
    (58, "TILE_COUNTER_END_RIGHT"),
    (59, "TILE_COUNTER_END_LEFT"),
    (60, "TILE_FIGHTER"),
    (61, "TILE_CLERIC"),
    (62, "TILE_MAGE"),
    (63, "TILE_THIEF"),
]


def main():
    assert len(TILE_IDS) == 64, f"expected 64 entries, got {len(TILE_IDS)}"
    assert sorted(v for v, _ in TILE_IDS) == list(range(64)), \
        "TILE_IDS must cover exactly 0..63 with no gaps or duplicates"

    enum_id = ida_enum.get_enum(ENUM_NAME)
    creating = enum_id == idaapi.BADADDR

    if creating:
        print(f"{ENUM_NAME} does not exist yet -- would create it with "
              f"{len(TILE_IDS)} members" + (" (dry run)" if DRY_RUN else ""))
        if DRY_RUN:
            for value, name in TILE_IDS:
                print(f"    {value:2} = {name}")
            print("\n[dry] nothing changed. Set DRY_RUN = False to create "
                  "the enum and all 64 members.")
            return
        # idaapi.BADADDR (size_t(-1)), not literal -1 -- the raw SWIG
        # binding wants an actual unsigned size_t, and Python -1 doesn't
        # auto-convert the way it would in C++ (OverflowError otherwise).
        enum_id = ida_enum.add_enum(idaapi.BADADDR, ENUM_NAME, ida_bytes.hex_flag())
        if enum_id == idaapi.BADADDR:
            print(f"[!] add_enum({ENUM_NAME!r}) failed")
            return
        ida_enum.set_enum_cmt(enum_id, ENUM_COMMENT, 0)
    else:
        print(f"{ENUM_NAME} already exists ({enum_id:X}) -- checking for "
              f"missing members only")

    added, skipped, failed = 0, 0, 0
    for value, name in TILE_IDS:
        existing_id = ida_enum.get_enum_member_by_name(name)
        if existing_id != idaapi.BADADDR:
            skipped += 1
            continue
        print(f"{'[would add] ' if DRY_RUN else ''}{value:2} = {name}")
        if DRY_RUN:
            continue
        err = ida_enum.add_enum_member(enum_id, name, value)
        if err != 0:
            print(f"    [!] add_enum_member failed, error code {err}")
            failed += 1
        else:
            added += 1

    if DRY_RUN:
        if not creating:
            print("\n[dry] nothing changed. Set DRY_RUN = False to add any "
                  "missing members above.")
        return

    print(f"\nDone. {added} member(s) added, {skipped} already present"
          + (f", {failed} failed" if failed else "") + ".")
    print("Re-export the .idc and check Local Types / Enums for TileId.")


if __name__ == "__main__":
    main()
