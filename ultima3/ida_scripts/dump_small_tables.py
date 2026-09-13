"""
Read-only dump of two small 4-entry near-pointer tables sitting right
after CLERIC_SPELL_TABLE's real (code-confirmed, 16-entry) end at
linear 0x1598B:

  0x1598B (4 entries, 8 bytes) -- unknown, addresses land around 0x15A5x
  0x15993 (4 entries, 8 bytes) -- confirmed used by sub_16366 (shrine
      entry) as `mov si,dx; shl si,1; mov si,[si+5993h]` indexed by
      `_partyPosition & 3`, printed as the shrine's virtue/attribute
      name on entry.

These are the exact addresses an earlier, WRONG version of
dump_spell_tables.py mistakenly attributed to CLERIC_SPELL_TABLE by
walking byte ranges until the next named symbol -- see docs/
overview.md's spell-table writeup. Dumping them properly here, by
address rather than by (wrong) table membership, to check whether the
first table is the RosterEntry status-letter name table
(aGood/aPoisoned/aDead/aAshes) suspected in create_roster_struct.py's
notes.

Read-only: does not rename or modify anything.
"""

import idc
import ida_bytes
import ida_name


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


def dump(base, count, label):
    print(f"{label} @ {base:#x}:")
    for i in range(count):
        ea = base + i * 2
        ptr = ida_bytes.get_word(ea)
        linear = 0x10000 + ptr
        nm = ida_name.get_name(linear)
        s = get_string_at(linear)
        print(f"  [{i}] {ea:#x}: ptr={ptr:#06x} (lin={linear:#06x})"
              f"{'  name=' + nm if nm else ''}  str={s!r}")


def main():
    dump(0x1598B, 4, "TABLE_A (unknown)")
    print()
    dump(0x15993, 4, "TABLE_B (shrine/stat names, per sub_16366)")


if __name__ == "__main__":
    main()
