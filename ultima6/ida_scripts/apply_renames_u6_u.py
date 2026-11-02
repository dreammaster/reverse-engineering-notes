"""
Master list of symbol renames for Ultima VI's U.EXE (u6_u.idb): title/main menu,
intro and character creation.  Idempotent; see docs/u6_game.md for the evidence.

    .\\run_ida_script.ps1 -Idb u6_u -ScriptName apply_renames_u6_u.py
"""

import idc
import ida_name

# (ea, new_name, note) -- the DOS file layer is the same Origin run-time code as in GAME.EXE
# (matched by instruction signature against u6_game's file_* functions).
FUNCTIONS = [
    (0x109F3, "file_open", "= GAME.EXE file_open"),
    (0x10A32, "file_close", "= GAME.EXE file_close"),
    (0x10AE4, "file_read", "= GAME.EXE file_read(dest far, count dword, offset dword, handle)"),
    (0x10BCA, "file_write", "= GAME.EXE file_write"),
]

for ea, name, note in FUNCTIONS:
    if idc.get_name(ea) != name:
        ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
print("renamed %d functions" % len(FUNCTIONS))
