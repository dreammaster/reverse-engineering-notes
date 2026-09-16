"""
Names sub_1D118: scans a table of 10-byte clickable-region entries at
ds:si (x_min, x_max, y_min, y_max, result -- 5 words each, 0xFFFF as
the x_min sentinel terminating the table) for one containing point
(ax, bx); returns (and stores in word_2E40A) the matching entry's
result word, or 0 if none match / point is outside all regions. Called
from `start`'s main loop with the mouse position and various
screen-specific region tables (including the 0x5CD0 table
GetListItemPosition also indexes) -- the generic mouse hit-test used
throughout the UI.

-> HitTestRegionTable

Run via:
    .\run_ida_script.ps1 name_hittest.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D118
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HitTestRegionTable", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HitTestRegionTable': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Generic mouse hit-test: scans a table at ds:si of 10-byte entries "
    "(x_min, x_max, y_min, y_max, result), 0xFFFF as x_min terminating "
    "the table, for one containing (ax, bx). Returns/stores in "
    "word_2E40A the matching result word, or 0 if none match.",
    False,
)
