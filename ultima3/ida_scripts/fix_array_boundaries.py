"""
One-off fix, growing list: addresses that failed idc.set_name in
apply_renames_exodus.py because each falls mid-array inside a larger,
undifferentiated data blob IDA had already defined -- the same class
of issue as the wind-direction string arrays (fix_wind_string_array.py)
and the command-key word tables (fix_command_key_tables.py). Fix:
del_items() to clear the existing item boundary, redefine as a plain
byte array of the right size, then retry the name. Idempotent: skips
any entry that already has its target name.
"""

import idc
import ida_bytes

# (address, name, size in bytes)
TABLES = [
    (0x18605, "MONSTER_HP_TABLE", 16),
    (0x18615, "MONSTER_EXP_TABLE", 16),
    (0x1598B, "FACING_DIRECTION_NAME_TABLE", 8),
    (0x15993, "SHRINE_ATTRIBUTE_NAME_TABLE", 8),
]


def main():
    for ea, name, size in TABLES:
        if idc.get_name(ea) == name:
            print(f"{ea:#x}: already {name!r} -- skipping")
            continue
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
        ok = idc.create_data(ea, idc.FF_BYTE, size, idc.BADADDR)
        print(f"{ea:#x}: create_data -> {ok}")
        ok = idc.set_name(ea, name, idc.SN_NOWARN)
        print(f"{ea:#x}: set_name({name!r}) -> {ok}")


if __name__ == "__main__":
    main()
