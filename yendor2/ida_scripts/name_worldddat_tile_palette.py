"""
Names sub_27FE0 and sub_205C0 -- a generic WORLD.DAT FileEntry setup
helper and its tile-palette-loading caller.

sub_27FE0 (-> PrepareWorldDatRead): a generic sibling of the existing
WorldDat_setBlock1-6 family. Given a FileEntry pointer (bx) and a
value (ax), sets [+4]=ax (the requested id/offset, caller-supplied
rather than a fixed block number), [+0xA]/[+0xC] from a fixed table
at 0xCDEF (WORLD.DAT's base file-offset/sector info, matching the
other WorldDat_setBlock* functions' convention), and [+6] = 4 *
_blockSize3 (a default record-size offset). Called from
DrawClueBookMapGrid, sub_205C0, and sub_111C1 (not traced).

sub_205C0 (-> LoadWorldDatTilePalette): calls PrepareWorldDatRead(bx=
FileEntry 0x9043, ax=0xAFA8 buffer), overrides [+6] with
_blockSize3 * word_329FE (an index -- selects one palette record among
several), then FileEntry_Read + ErrorCheck. Called from the already-
named PaintCellAndPersist and from sub_205FB -- loading a per-level
tile palette record from WORLD.DAT, per file-formats.md's existing
note about RunMapEditorScreen's B/F palette-browsing keys.

Run via:
    .\run_ida_script.ps1 name_worldddat_tile_palette.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x27FE0: "PrepareWorldDatRead",
    0x205C0: "LoadWorldDatTilePalette",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x27FE0,
    "Generic WORLD.DAT FileEntry setup, sibling of WorldDat_setBlock1-6: "
    "sets [+4]=ax (caller-supplied id, not a fixed block number), "
    "[+0xA]/[+0xC] from table 0xCDEF, [+6]=4*_blockSize3 (default, "
    "often overridden by the caller). Called from DrawClueBookMapGrid, "
    "LoadWorldDatTilePalette, and sub_111C1.",
    False,
)
ida_bytes.set_cmt(
    0x205C0,
    "Loads a per-level tile-palette record from WORLD.DAT: "
    "PrepareWorldDatRead then overrides the block offset with "
    "_blockSize3*word_329FE (selects one record among several) before "
    "FileEntry_Read. Called from PaintCellAndPersist and sub_205FB -- "
    "feeds RunMapEditorScreen's B/F palette-browsing keys.",
    False,
)
