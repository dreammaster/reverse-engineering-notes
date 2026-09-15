"""
Names sub_28CB1, called from RevealMapRegion and RevealMapRegionRow
(both pre-existing names from an earlier session) -- previously
flagged as an open lead ("sub_28C94/sub_28CB1/sub_29259... don't force
a name").

Given a position (ax), reads WORLD.DAT block 3 (WorldDat_setBlock3)
into a buffer, preserving the caller's own block-read context
(word_368A7/A9/AB/AD/AF pushed/popped around the call -- a nested-read
guard), and returns one byte (word_3880C+1) from that buffer.

Mechanism confirmed (a single-byte WORLD.DAT block-3 read for a map
cell, used while revealing map regions); the exact meaning of the byte
still isn't identified.

-> ReadMapCellAttributeByte

Run via:
    .\run_ida_script.ps1 name_read_map_cell_attribute.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28CB1
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ReadMapCellAttributeByte", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ReadMapCellAttributeByte': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Reads WORLD.DAT block 3 for a given position (ax), preserving "
    "the caller's own block-read context (nested-read guard), and "
    "returns one byte from the result. Exact meaning of the byte not "
    "identified. Called from RevealMapRegion/RevealMapRegionRow.",
    False,
)
