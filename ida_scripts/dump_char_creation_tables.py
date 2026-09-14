"""
Read-only: decodes ultima_bootup.idb's sex/race/class letter-code and
display-name tables (byte_11045/byte_1106B/byte_110D0 -- the
key-letter lists gatherCharacterCreationInput/showCharacterDetails
pass to getMenuChoice) by walking the actual pointer tables
(byte_11048/byte_11070/byte_110DB) programmatically rather than
hand-parsing a raw hex dump, which is exactly the kind of manual
byte-counting this project has been burned by before (the dungeon
command table mixup, the CLERIC_SPELL_TABLE over-read).
"""

import idc
import ida_bytes

# (key-letter-table addr, key count, pointer-table addr, roster field)
TABLES = [
    (0x11045, 3, 0x11048, "_sex"),
    (0x1106B, 5, 0x11070, "_race"),
    (0x110D0, 11, 0x110DB, "_class"),
]


def get_string_at(ea, maxlen=32):
    out = []
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
    return "".join(out)


def main():
    for keys_ea, count, ptrs_ea, field in TABLES:
        print(f"{field} (keys @ {keys_ea:#x}, ptrs @ {ptrs_ea:#x}):")
        for i in range(count):
            key = ida_bytes.get_byte(keys_ea + i)
            ptr = ida_bytes.get_word(ptrs_ea + i * 2)
            linear = 0x10000 + ptr
            name = get_string_at(linear)
            print(f"  '{chr(key)}' ({key:#04x}) -> ptr={ptr:#06x} "
                  f"(lin={linear:#06x}) -> {name!r}")
        print()


if __name__ == "__main__":
    main()
