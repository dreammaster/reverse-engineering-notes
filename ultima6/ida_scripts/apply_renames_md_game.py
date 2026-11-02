"""
Symbol renames for the Martian Dreams GAME.EXE (md_game.idb).  Idempotent.

    .\run_ida_script.ps1 -Idb md_game -ScriptName apply_renames_md_game.py

The Origin DOS file layer is byte-for-byte the code of Ultima VI's GAME.EXE (matched by instruction signature,
see tools/match_functions.py), at addresses 3Bh higher.  The LZW decoder is the same algorithm as Ultima VI's
lzw_decode (3ACB4) but is not signature-identical.
"""

import idc
import ida_name

# (ea, new_name, note)
FUNCTIONS = [
    (0x10245, "file_open", "= Ultima VI file_open"),
    (0x10284, "file_close", "= Ultima VI file_close"),
    (0x102B8, "file_create", "= Ultima VI file_create"),
    (0x102F8, "file_delete", "= Ultima VI file_delete"),
    (0x10336, "file_read", "= Ultima VI file_read(dest far, count dword, offset dword, handle)"),
    (0x1041C, "file_write", "= Ultima VI file_write"),
    (0x104E1, "file_rename", "= Ultima VI file_rename"),
    (0x368FA, "lzw_decode", "LZW: 9-12 bit codes, 100h reset, 101h end (same algorithm as Ultima VI 3ACB4)"),
]

for ea, name, note in FUNCTIONS:
    if idc.get_name(ea) != name:
        if not ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED %05X %s" % (ea, name))
print("renamed %d functions" % len(FUNCTIONS))
