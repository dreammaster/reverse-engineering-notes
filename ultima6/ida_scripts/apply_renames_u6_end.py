"""
Master list of symbol renames for Ultima VI's END.EXE (u6_end.idb): the ending
sequence.  Idempotent; see docs/u6_game.md for the evidence.

    .\\run_ida_script.ps1 -Idb u6_end -ScriptName apply_renames_u6_end.py
"""

import idc
import ida_name

# (ea, new_name, note) -- same Origin file layer as GAME.EXE (matched by instruction signature).
FUNCTIONS = [
    (0x10671, "file_open", "= GAME.EXE file_open"),
    (0x106B0, "file_close", "= GAME.EXE file_close"),
    (0x10762, "file_read", "= GAME.EXE file_read(dest far, count dword, offset dword, handle)"),
    (0x10848, "file_write", "= GAME.EXE file_write"),
]

for ea, name, note in FUNCTIONS:
    if idc.get_name(ea) != name:
        ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
print("renamed %d functions" % len(FUNCTIONS))
