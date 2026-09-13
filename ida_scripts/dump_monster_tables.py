"""
Read-only exploratory dump: two 16-entry monster-class stat tables
found while tracing applyCombatDamage's experience-award lookup
(`[bx-79EBh]`, bx = _conflictMonsterClass & 0xFh) and
beginCombatEncounter's per-monster HP-roll setup (`[bx-79FBh]`, same
index convention, 0x10 bytes earlier).

`[bx-NNNNh]` with a small bx (0-15) wraps around 16-bit arithmetic:
linear = (0x10000 - 0xNNNN + bx) with DS=0x1000 added again for the
absolute address, i.e. linear = 0x10000 + (0x10000 - 0xNNNN) + bx.
For -79FBh: 0x10000 + 0x8605 = 0x18605. For -79EBh: 0x10000 + 0x8615 =
0x18615 -- exactly 0x10 apart, i.e. two adjacent 16-byte tables, with
byte_18625 (already referenced elsewhere for lookupWeaponGlyph) sitting
right after them at +0x20, consistent with a small monster-class stat
block laid out table-by-table rather than struct-by-struct.

Read-only: does not rename or modify anything.
"""

import ida_bytes

HP_TABLE = 0x18605
EXP_TABLE = 0x18615
COUNT = 16


def main():
    print("class : hp-roll-param : exp-award")
    for i in range(COUNT):
        hp = ida_bytes.get_byte(HP_TABLE + i)
        exp = ida_bytes.get_byte(EXP_TABLE + i)
        print(f"  {i:2} (0x{i:X}) : {hp:#04x} ({hp:3}) : {exp:#04x} ({exp:3})")


if __name__ == "__main__":
    main()
