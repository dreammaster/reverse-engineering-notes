"""
IDA Pro script: fixes word_1F95E in OUT.EXE -- flagged in roadmap.md as
"currently a single `dw 1` plus raw `db` bytes, should be a proper
`dw 4 dup(?)`". Confirmed via its only reference (readAmount, the
4-digit numeric-input reader used by dropPence/transactGrocer/etc.):
`mul word_1F95E[bx]` with bx = digitPosition*2, accumulating a
decimal number one digit at a time from the last-typed digit backward
-- i.e. this is a powers-of-ten lookup table for ASCII-to-integer
conversion. The raw bytes following the first `dw 1` decode as
0x000A(10), 0x0064(100), 0x03E8(1000) -- exactly 1/10/100/1000.

Un-defines the 8-byte range and redefines it as a proper 4-element
word array, renamed POWERS_OF_TEN (matching this IDB's existing
ALL_CAPS convention for lookup tables: ARMOR, WEAPONS_LOWERCASE,
SPELL_NAMES, TRANSPORTS, ATTRIBUTES, RACES, CLASSES).

    .\\run_ida_script.ps1 -Idb ultima1_out -ScriptName fix_powers_of_ten.py
"""

import idc
import ida_bytes

DRY_RUN = False

EA = 0x1F95E
EXPECTED_BYTES = [1, 0, 0xA, 0, 0x64, 0, 0xE8, 3]  # little-endian 1,10,100,1000


def main():
    actual = [ida_bytes.get_byte(EA + i) for i in range(8)]
    if actual != EXPECTED_BYTES:
        print(f"[!] byte content at {EA:#x} doesn't match expected 1/10/100/1000 -- aborting")
        print(f"    expected: {EXPECTED_BYTES}")
        print(f"    actual:   {actual}")
        return

    cur_name = idc.get_name(EA)
    print(f"{EA:#x}: {cur_name!r}, 8 raw bytes (1 dw + 6 undefined db) -- fixing to a 4-word array")
    if DRY_RUN:
        print("[dry] would undefine, redefine as 4 words, rename to POWERS_OF_TEN")
        return

    ok = ida_bytes.del_items(EA, ida_bytes.DELIT_SIMPLE, 8)
    print(f"  del_items ok={ok}")
    ok = ida_bytes.create_word(EA, 8)
    print(f"  create_word(8 bytes = 4 words) ok={ok}")
    ok = idc.set_name(EA, "POWERS_OF_TEN", idc.SN_NOWARN)
    print(f"  rename to POWERS_OF_TEN ok={ok}")

    for i in range(4):
        w = ida_bytes.get_wide_word(EA + i * 2)
        print(f"  [{i}] = {w}")

    print("\nDone. Re-export the .asm/.idc and check the new array display took.")


if __name__ == "__main__":
    main()
