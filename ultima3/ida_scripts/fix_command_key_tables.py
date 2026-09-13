"""
One-off structural fix for ultima_exodus.idb: the overworld command-key
table at 0x11887 (33 words, each an (scancode:char) AX value compared
via `repne scasw` against the keypress) was misdisassembled by IDA as
x86 code (it happens to decode into plausible-looking but nonsensical
instructions -- classic "data misidentified as code" failure mode).
Undefines that range and redefines it as a plain word array so it can
be named and read cleanly.

Run once via run_ida_script.ps1, then apply_renames_exodus.py can name
it OVERWORLD_COMMAND_KEYS.
"""

import ida_bytes
import idc

TABLE_EA = 0x11887
TABLE_WORDS = 33

DRY_RUN = False

if not DRY_RUN:
    ida_bytes.del_items(TABLE_EA, ida_bytes.DELIT_EXPAND, TABLE_WORDS * 2)
    ok = idc.create_data(TABLE_EA, idc.FF_WORD, TABLE_WORDS * 2, idc.BADADDR)
    print(f"create_data(word array, {TABLE_WORDS} entries) -> {ok}")
    idc.make_array(TABLE_EA, TABLE_WORDS)
else:
    print("[dry] nothing changed.")
