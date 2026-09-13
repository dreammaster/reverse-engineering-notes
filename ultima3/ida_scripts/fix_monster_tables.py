"""
One-off fix: MONSTER_HP_TABLE (0x18605) and MONSTER_EXP_TABLE
(0x18615) failed idc.set_name in apply_renames_exodus.py because each
address falls mid-array inside a larger, undifferentiated data blob
IDA had already defined -- the same class of issue as the wind-
direction string arrays (fix_wind_string_array.py) and the command-key
word tables (fix_command_key_tables.py). Fix: del_items() to clear the
existing item boundary, redefine as a 16-byte array, then retry the
name.
"""

import idc
import ida_bytes

TABLES = [
    (0x18605, "MONSTER_HP_TABLE"),
    (0x18615, "MONSTER_EXP_TABLE"),
]

SIZE = 16


def main():
    for ea, name in TABLES:
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, SIZE)
        ok = idc.create_data(ea, idc.FF_BYTE, SIZE, idc.BADADDR)
        print(f"{ea:#x}: create_data -> {ok}")
        ok = idc.set_name(ea, name, idc.SN_NOWARN)
        print(f"{ea:#x}: set_name({name!r}) -> {ok}")


if __name__ == "__main__":
    main()
