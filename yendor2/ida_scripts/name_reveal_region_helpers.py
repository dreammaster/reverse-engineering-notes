"""
Names sub_28C94 and sub_29259, both called from RevealMapRegion and
RevealMapRegionRow -- previously flagged open leads
("sub_28C94/sub_28CB1/sub_29259... don't force a name").

sub_28C94 (-> ComputeMapCellIndex): a coordinate-to-index conversion,
`(bx / word_32A0E) * word_32A04 + (bx / word_32A06)` -- the classic
"row/column from tile size, times row width, plus column" formula for
turning a raw coordinate into a flat WORLD.DAT/array index.

sub_29259 (-> DrawRevealedCellIcon): draws one revealed cell's
minimap-style icon -- the `0xE551` table's `[+0xA]` field (a wall/
floor top-down icon variant, a field not otherwise used yet) plus,
if present, an overlay from the `0xE175` door/side-feature table's
`[+8]` field (the same table DrawDungeonCellSideFeature reads).

Run via:
    .\run_ida_script.ps1 name_reveal_region_helpers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x28C94: "ComputeMapCellIndex",
    0x29259: "DrawRevealedCellIcon",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x28C94,
    "Coordinate-to-index conversion: (bx/word_32A0E)*word_32A04 + "
    "(bx/word_32A06). Called from RevealMapRegion/RevealMapRegionRow.",
    False,
)
ida_bytes.set_cmt(
    0x29259,
    "Draws one revealed cell's minimap-style icon (0xE551 table's "
    "[+0xA] field) plus an optional door/feature overlay (0xE175 "
    "table's [+8] field). Called from RevealMapRegionRow.",
    False,
)
