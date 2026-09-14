"""
Fixes the mis-disassembled-as-code region flagged while wiring
RosterEntry op_stroff (docs/roadmap.md, linear 0x16700-0x16900+).
Turns out to be `printNameByIndex`'s own backing data: a 0x88-entry
near-pointer table at linear 0x16556 (already known from that
function's `[bx+6556h]` operand) indexing into a large null-terminated
ASCII string blob immediately after it -- confirmed by direct byte
dump, not guessed: terrain names (index 1-9: Water/Grass/Brush/Forest/
Mountains/Dungeon/Towne/Castle/Floor -- exactly the tile categories
`cmdLook` describes), objects/vehicles (0xA-0xC), the Whirlpool
(0xD), NPCs/monster-class names (0xE-0x20, ending at 'Exodus'),
dungeon features (0x21-0x26: Force Field/Lava/Moon Gate/Wall/Void),
single-letter rune/sign tiles (0x27-~0x42), weapon names (~0x41-0x50,
matching `_weaponOwned`'s confirmed 15-entry array), armour names
(0x51-0x58, an 8th "Skin"/unarmored entry ahead of the previously-
confirmed 7-entry `_armourOwned` list -- likely `_armourIndex`'s own
range, which includes the "wearing nothing" state `_armourOwned`
itself doesn't need to track), all 32 spell names again (0x59-0x78,
a second copy distinct from SPELL_NAME_TABLE at 0x1590B), and finally
a 16-entry monster bestiary (0x79-0x88: Brigand/Cutpurse/Goblin/Troll/
Ghoul/Zombie/Golem/Titan/Gargoyle/Mane/Snatch/Bradle/Griffon/Wyvern/
Orcus/Devil) -- Ultima III's actual named monster list, confirmed
directly from the game's own data for the first time this project.

Converts the pointer table to a named word array and the string blob
to individual create_strlit-defined strings (falling back to the
established del_items+create_data(FF_BYTE) trick from
fix_wind_string_array.py for any that mysteriously fail create_strlit,
same as every other string-array fix this project has needed).
"""

import idc
import ida_bytes

DRY_RUN = False

TABLE_EA = 0x16556
TABLE_COUNT = 0x88
STRING_START = 0x16666
# One string blob (terrain/monster/spell/rune names, addressed by the
# table above) runs from STRING_START through 'Devil'+NUL; a second,
# unrelated-but-adjacent blob of abbreviated inventory labels
# (Gems../Keys../Powd../Trch..) follows immediately after and is
# fixed too since it was swept up in the same original mis-decode.
STRING_END = 0x16A24  # just past "Trch..\0" -- confirmed via byte dump


def fix_pointer_table():
    size = TABLE_COUNT * 2
    ida_bytes.del_items(TABLE_EA, ida_bytes.DELIT_SIMPLE, size)
    ok = idc.create_data(TABLE_EA, idc.FF_WORD, size, idc.BADADDR)
    print(f"pointer table {TABLE_EA:#x} ({size} bytes): create_data -> {ok}")
    ok = idc.set_name(TABLE_EA, "GAME_NAME_TABLE", idc.SN_NOWARN)
    print(f"  set_name -> {ok}")


def fix_string_blob():
    ea = STRING_START
    count = 0
    fails = 0
    while ea < STRING_END:
        # find the NUL terminator
        end = ea
        while end < STRING_END and ida_bytes.get_byte(end) != 0:
            end += 1
        length = end - ea + 1  # include the NUL
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, length)
        ok = idc.create_strlit(ea, ea + length)
        if not ok:
            ok = idc.create_data(ea, idc.FF_BYTE, length, idc.BADADDR)
            fails += 1
        count += 1
        ea += length
    print(f"string blob {STRING_START:#x}-{STRING_END:#x}: "
          f"{count} strings defined, {fails} needed the create_data fallback")


def main():
    if DRY_RUN:
        print("[dry] would fix pointer table + string blob; "
              "set DRY_RUN=False to apply")
        return
    fix_pointer_table()
    fix_string_blob()


if __name__ == "__main__":
    main()
