"""
One-off structural fix: the 5 wind-direction strings at 0x1623D-0x1627D
(see apply_renames.py's WIND_DIRECTION_TABLE/aCalmWind/etc entries) were
originally one undifferentiated data blob -- IDA only let the first
string (aCalmWind, at the blob's start) be named; the other 4 sit at
mid-array offsets and idc.set_name silently failed for them (returns
False, not an exception) until the array is split into 5 separate
13-byte string-literal items.

Each string is exactly 13 bytes: a 0x10 lead-in byte, the text
(space-padded to a common width), a 0x11 byte, then a 0x00 terminator --
STRTYPE_C's normal null-termination handles this fine even though the
leading/trailing control bytes aren't printable ASCII.

Run once via run_ida_script.ps1, then re-run apply_renames.py to apply
the now-unblocked aNorthWind/aSouthWind/aEastWind/aWestWind renames.
"""

import ida_bytes
import idc

STRING_STARTS = [0x1623D, 0x1624A, 0x16257, 0x16264, 0x16271]
STRING_LEN = 13

DRY_RUN = False

for ea in STRING_STARTS:
    raw = ida_bytes.get_bytes(ea, STRING_LEN)
    print(f"{ea:#x}: {raw!r}")
    if DRY_RUN:
        continue
    ok1 = ida_bytes.del_items(ea, ida_bytes.DELIT_EXPAND, STRING_LEN)
    print(f"    del_items -> {ok1}, flags now {ida_bytes.get_full_flags(ea):#x}")
    ok2 = ida_bytes.create_strlit(ea, ea + STRING_LEN, idc.STRTYPE_C)
    print(f"    create_strlit -> {ok2}")
    if not ok2:
        ok3 = idc.create_data(ea, idc.FF_BYTE, STRING_LEN, idc.BADADDR)
        print(f"    fallback create_data(byte array) -> {ok3}")

if DRY_RUN:
    print("\n[dry] nothing changed.")
else:
    print("\nDone -- re-run apply_renames.py to name the 4 previously-blocked strings.")
