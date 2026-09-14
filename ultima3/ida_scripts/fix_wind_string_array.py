"""
One-off structural fix, run against BOTH ultima.idb and ultima_bootup.idb
(same bug, same shape, hits both since they share this runtime code -- see
docs/overview.md): the 5 wind-direction strings were originally one
undifferentiated data blob in each IDB -- IDA only let the first string
(aCalmWind, at the blob's start) be named; the other 4 sit at mid-array
offsets and idc.set_name silently failed for them (returns False, not an
exception) until the array is split into 5 separate 13-byte items.

Each string is exactly 13 bytes: a 0x10 lead-in byte, the text
(space-padded to a common width), a 0x11 byte, then a 0x00 terminator.
create_strlit returns False for these even after del_items -- RESOLVED
2026-09-14, not a bug: STRTYPE_C validates every byte before the
terminator against the encoding's legal character set, and control
bytes 0x10/0x11 aren't legal C-string characters, so create_strlit
correctly refuses. Falls back to a plain FF_BYTE array via
idc.create_data, which is actually the more accurate representation
of this mixed control-code-plus-text blob, not just a workaround.

IMPORTANT: each IDB's address list is only valid *in that IDB* -- the two
executables' address ranges overlap (both are tiny-model COM-style images
based at paragraph 1000h), so ultima.idb's wind-string addresses point at
unrelated real data inside ultima_bootup.idb and vice versa. Running the
wrong list against the wrong IDB would corrupt whatever legitimate data
happens to sit at those addresses there. STRING_STARTS below is set to
whichever list still needs fixing -- check docs/roadmap.md / the git log
for which IDB(s) this has already been run against before re-running, and
edit the active list by hand rather than combining them.

Already applied: ULTIMA_STARTS (2026-09-13, ultima.idb), BOOTUP_STARTS
(2026-09-13, ultima_bootup.idb).

Run once via run_ida_script.ps1 -Idb <target>, then re-run the matching
apply_renames_*.py to apply the now-unblocked aNorthWind/aSouthWind/
aEastWind/aWestWind renames.
"""

import ida_bytes
import idc

# ultima.idb's wind strings -- already applied, kept for reference only.
ULTIMA_STARTS = [0x1623D, 0x1624A, 0x16257, 0x16264, 0x16271]
# ultima_bootup.idb's wind strings -- already applied, kept for reference.
BOOTUP_STARTS = [0x11F5D, 0x11F6A, 0x11F77, 0x11F84, 0x11F91]
# ultima_exodus.idb's wind strings (same shape, different addresses) --
# this is the active list as of this script's last edit.
EXODUS_STARTS = [0x12AAD, 0x12ABA, 0x12AC7, 0x12AD4, 0x12AE1]

STRING_STARTS = EXODUS_STARTS
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
