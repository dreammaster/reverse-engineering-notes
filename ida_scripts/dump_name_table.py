"""
Read-only: dumps printNameByIndex's pointer table (linear 0x16556,
confirmed as `[bx+6556h]` in that function -- DS=0x1000 -> linear
0x10000+0x6556=0x16556) alongside the newly-found ASCII string blob
around 0x16690-0x16850+, to check whether the pointer table indexes
directly into these strings (which would fully explain
printNameByIndex's purpose and give a confirmed index->name mapping).
"""

import idc
import ida_bytes

TABLE = 0x16556
COUNT = 0x88  # printNameByIndex's own bounds check: al must be <= 0x88


def get_string_at(ea, maxlen=32):
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
    for i in range(0x30, 0x66):
        entry_ea = TABLE + (i - 1) * 2
        ptr = ida_bytes.get_word(entry_ea)
        linear = 0x10000 + ptr
        s = get_string_at(linear)
        print(f"  [{i:#04x}] {entry_ea:#06x}: ptr={ptr:#06x} (lin={linear:#06x}) -> {s!r}")


if __name__ == "__main__":
    main()
