"""
Reverts the false-positive RosterEntry op_stroff annotations that
apply_roster_stroff.py's first (uncurated) run persisted to disk --
IDA's .idb is a live paged database, so changes commit as they
happen regardless of the driver's own `-NoExport`/save_database()
skip; there is no "discard without saving" for a headless run once an
edit has been made. These 21 sites are garbage-decoded data-table
bytes (confirmed by hand -- see apply_roster_stroff.py's
EXCLUDED_RANGES comment) or ordinary stack-frame locals, not real
RosterEntry accesses, and were wrongly annotated before that
exclusion existed. idc.op_hex resets the operand's number format back
to plain hex, clearing the struct-offset display.
"""

import idc

# (ea, opnum)
BAD_SITES = [
    (0x16773, 1), (0x16779, 0), (0x1677D, 0), (0x16781, 0), (0x16785, 0),
    (0x16789, 0), (0x1678D, 0), (0x16791, 0), (0x16795, 0), (0x16799, 0),
    (0x167A1, 1), (0x167A6, 1), (0x167CB, 0), (0x168B7, 1), (0x16A84, 1),
    (0x178B1, 1), (0x17B87, 1), (0x17C3A, 1), (0x17CC5, 1),
    (0x168FE, 1), (0x16B62, 0),
]

for ea, n in BAD_SITES:
    before = idc.GetDisasm(ea)
    ok = idc.op_hex(ea, n)
    after = idc.GetDisasm(ea)
    print(f"{ea:#x} op{n}: {'ok' if ok else 'FAILED'}  {before}  ->  {after}")
