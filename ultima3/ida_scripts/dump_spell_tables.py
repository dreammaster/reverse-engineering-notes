"""
Read-only exploratory dump: WIZARD_SPELL_TABLE / CLERIC_SPELL_TABLE
(spell-effect routine pointers) and the combined 32-entry spell-NAME
table castSpell reads via `mov si, [si+590Bh]` (linear 0x1590B).

IMPORTANT lesson applied here: an earlier attempt at this script walked
each spell table's byte range until IDA's *next named symbol*, which
silently over-read CLERIC_SPELL_TABLE into an unrelated, adjacently-
placed table (character-creation stat-name string pointers -- aStrength
etc.) that happens to have no boundary marker of its own. This is the
exact same class of mistake that produced the dungeon-command-table
mixup earlier this session (see docs/overview.md's "hard-won lesson").
Fixed by reading castSpell's own code instead of guessing at data
layout: it explicitly restricts the typed spell letter to 'A'..'P'
(cmp ah,41h/cmp ah,50h, castSpell+89..+92) before subtracting 'A' to
get a 0-based index, proving BOTH tables are exactly 16 entries (32
bytes) -- not derived from where a following label happens to sit.

Read-only: does not rename or modify anything.
"""

import idc
import ida_bytes
import ida_name

SPELL_NAME_TABLE = 0x1590B  # `[si+590Bh]`, DS=0x1000 -> linear 0x1000*16+0x590B
SPELLS_PER_CLASS = 16


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


def dump_effect_table(table_name, name_base_index):
    start = idc.get_name_ea_simple(table_name)
    print(f"{table_name}: start={start:#x}, {SPELLS_PER_CLASS} entries "
          f"(letters A-P, confirmed via castSpell's own bounds check)")
    for i in range(SPELLS_PER_CLASS):
        entry_ea = start + i * 2
        ptr = ida_bytes.get_word(entry_ea)
        linear = 0x10000 + ptr
        fn_name = ida_name.get_name(linear)
        name_ptr_ea = SPELL_NAME_TABLE + (name_base_index + i) * 2
        name_ptr = ida_bytes.get_word(name_ptr_ea)
        name_linear = 0x10000 + name_ptr
        spell_name = get_string_at(name_linear)
        letter = chr(ord('A') + i)
        print(f"  [{i:2}] {letter}  effect={linear:#06x}"
              f"{'  (' + fn_name + ')' if fn_name else ''}"
              f"  name={spell_name!r}")


def main():
    dump_effect_table("WIZARD_SPELL_TABLE", 0)
    print()
    dump_effect_table("CLERIC_SPELL_TABLE", SPELLS_PER_CLASS)


if __name__ == "__main__":
    main()
