"""
Read-only exploratory dump: DUNGEON_COMMAND_LABELS (off_1778C), the
dungeon counterpart of the overworld prompt table dumped by
dump_overworld_labels.py. Referenced via:

    lea si, DUNGEON_COMMAND_LABELS
    mov si, [bx+si]
    jmp DUNGEON_COMMAND_TABLE[bx]

33 word entries (same bx=index*2 stride as DUNGEON_COMMAND_TABLE/
DUNGEON_COMMAND_KEYS). Unlike the overworld table (dump_overworld_
labels.py), IDA already resolved DUNGEON_COMMAND_LABELS as a proper
named symbol via `lea`, so no manual DS-segment math is needed for the
table base itself -- only the string pointers it holds are near
pointers requiring the `linear = 0x10000 + ptr` convention (confirmed
in the overworld dump). Read-only: does not rename or modify anything.
"""

import idc
import ida_bytes

COUNT = 33


def get_string_at(ea, maxlen=48):
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
    table_base = idc.get_name_ea_simple("DUNGEON_COMMAND_LABELS")
    keys_base = idc.get_name_ea_simple("DUNGEON_COMMAND_KEYS")
    print(f"DUNGEON_COMMAND_LABELS = {table_base:#x}")
    print(f"DUNGEON_COMMAND_KEYS   = {keys_base:#x}\n")

    for i in range(COUNT):
        entry_ea = table_base + i * 2
        ptr = ida_bytes.get_word(entry_ea)
        linear_ptr = 0x10000 + ptr
        s = get_string_at(linear_ptr)
        key_word = ida_bytes.get_word(keys_base + i * 2)
        char_code = key_word & 0xFF
        char_disp = chr(char_code) if 32 <= char_code < 127 else f"\\x{char_code:02x}"
        print(f"  [{i:2}] key={char_disp!r} label_ptr={ptr:#06x} (lin={linear_ptr:#06x}) -> {s!r}")


if __name__ == "__main__":
    main()
