"""
Names two small per-entry loop-body helpers.

sub_13380 (called from DrawClueBookMapGrid's per-entry loop): takes a
world-coordinate location-marker record at es:[di] (x at +2, y at
+4, an id/type at +6), computes its within-block pixel offset (mod
40/mod 24, scaled by 8) to get screen x/y, stores a bounding box
(x0/x1, y0/y1) plus the id into the output array at [si], then draws
a fixed marker icon (picture 0x73) at that position. Advances si by
0xA and di by 8 for the next entry. -> DrawClueBookMapLocationMarker

sub_147D8 (called 3 times from ListCompatibleClueBookItems): appends
one hit-test region entry to an output array -- x0=x, x1=x+0x10,
y0=y+5, y1=y+0xE, id=dx (auto-incrementing) -- then advances x by
0x1A (26px) and di by 0xA (10 bytes/entry), building a row of equally
-spaced clickable icon-slot regions (the same 10-byte hit-test-entry
layout HitTestRegionTable consumes elsewhere).
-> AppendClueBookItemHitTestSlot

Run via:
    .\run_ida_script.ps1 name_clue_book_map_marker_and_hittest_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13380
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawClueBookMapLocationMarker", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawClueBookMapLocationMarker': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Per-entry loop body for DrawClueBookMapGrid: converts a world-"
    "coordinate location-marker record (es:[di]) into an on-screen "
    "bounding box + id (stored at [si]) and draws a marker icon "
    "(picture 0x73) at the computed position. Advances si+=0xA, "
    "di+=8.",
    False,
)

ea = 0x147D8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "AppendClueBookItemHitTestSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'AppendClueBookItemHitTestSlot': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Per-entry loop body for ListCompatibleClueBookItems: appends "
    "one 10-byte hit-test region entry (x0/x1, y0/y1, auto-"
    "incrementing id) at the current x/y, then advances x by 0x1A "
    "for the next slot.",
    False,
)
