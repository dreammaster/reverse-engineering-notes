"""
Read-only exploratory dump: the overworld per-command prompt-string
table at file offset 0x18C9 (referenced via `mov si, [bx+18C9h]` in
sub_17B54's dispatcher, immediately before `jmp
OVERWORLD_COMMAND_TABLE[bx]` -- the overworld counterpart of
DUNGEON_COMMAND_LABELS/off_1778C). 33 word entries, same bx stride (2)
and same index convention as OVERWORLD_COMMAND_TABLE/
OVERWORLD_COMMAND_KEYS.

Learned the hard way this session (see docs/overview.md's "hard-won
lesson" on the dungeon-command-table mixup): read the table bounds
directly and print raw values -- do not assume a length or guess at
adjacent-table boundaries. This script only reads bytes/strings; it
does not rename or modify anything.
"""

import idc
import ida_bytes

TABLE_BASE = 0x118C9  # `[bx+18C9h]` is DS-relative; DS=0x1000 (this IDB's
# load segment per identify.py's "entry point: 0x100 (cs=0x1000)"), so
# linear = 0x1000*16 + 0x18C9 = 0x118C9, matching the loaded segment
# range (seg000: 0x10100-0x1ADCA) -- the raw 0x18C9 from the operand is
# NOT itself a valid linear address in this segment.
COUNT = 33

OVERWORLD_COMMAND_KEYS = 0x11887  # word scancode:char array, 33 entries


def get_string_at(ea, maxlen=40):
    if ea == idc.BADADDR:
        return None
    out = []
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
    return "".join(out)


def main():
    print(f"table base: {TABLE_BASE:#x}, {COUNT} entries\n")
    for i in range(COUNT):
        entry_ea = TABLE_BASE + i * 2
        ptr = ida_bytes.get_word(entry_ea)
        linear_ptr = 0x10000 + ptr  # near pointer, DS=0x1000 segment
        s = get_string_at(linear_ptr)
        key_word = ida_bytes.get_word(OVERWORLD_COMMAND_KEYS + i * 2)
        char_code = key_word & 0xFF
        char_disp = chr(char_code) if 32 <= char_code < 127 else f"\\x{char_code:02x}"
        print(f"  [{i:2}] key={char_disp!r} label_ptr={ptr:#06x} (lin={linear_ptr:#06x}) -> {s!r}")


if __name__ == "__main__":
    main()
