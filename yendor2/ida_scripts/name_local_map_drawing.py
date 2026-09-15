"""
Traced ShowLocalAreaMap's per-row/per-cell drawing helpers.

sub_22140 -> DrawLocalMapRow: draws 40 columns of one map row. For
each cell, tests a bit from the explored/fog-of-war bitmap (same
format PersistExploredCell writes) -- if unexplored, draws a fixed
blank/fog tile (g_pictureDir entry 0x13) directly; if explored, calls
sub_221A0 to draw the actual cell contents.

sub_221A0 -> DrawLocalMapCell: gated on word_328CA bit 4, calls the
already-named TryInteractAtPosition on the current cell to check for
a special interactive object there (errorCode 6/7 distinguish two
specific outcomes -- e.g. a locked door or similar), selecting an
overlay tile value accordingly; otherwise falls back to the cell's
own stored tile-type fields ([si]/[si+2]). So the local-area map shows
special objects (doors, etc.) with distinct icons, not just plain
terrain.

Run via:
    .\run_ida_script.ps1 name_local_map_drawing.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x22140: "DrawLocalMapRow",
    0x221A0: "DrawLocalMapCell",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x22140,
    "Draws 40 columns of one ShowLocalAreaMap row. Per cell, tests "
    "the explored/fog-of-war bitmap bit (same format "
    "PersistExploredCell writes): unexplored -> fixed blank/fog tile "
    "(g_pictureDir entry 0x13); explored -> DrawLocalMapCell.",
    False,
)
ida_bytes.set_cmt(
    0x221A0,
    "Draws one local-area-map cell. If word_328CA bit 4 is set, "
    "calls TryInteractAtPosition on the cell to check for a special "
    "interactive object (errorCode 6/7 select an overlay tile); "
    "otherwise uses the cell's own stored tile-type fields.",
    False,
)
