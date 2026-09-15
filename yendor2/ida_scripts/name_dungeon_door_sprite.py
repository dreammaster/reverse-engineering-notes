"""
Names sub_21187 and its null-check wrapper sub_2117F, called from
RenderDungeonViewRow per cell: draws a door/side-feature sprite.

sub_21187 (-> DrawDungeonCellSideFeature): indexes a 10-byte-stride,
4-direction table at 0xE175 by the cell's [+2] field (a
door/side-feature id) and the current facing (word_36CF5 tier bits --
same pattern as ShowCompassDirection/SpawnMonsterInFacingDirection),
draws the resulting picture at z-layer 7 or 8 depending on whether
[+2] falls within near/far distance bands (_val21..24), then draws an
extra overlay (picture 6) if the cell's [+6] flags have bit 0x1000
set and the just-drawn picture matched _val27 -- plausibly an
open-door or lit-torch variant.

sub_2117F (-> TryDrawDungeonCellSideFeature): trivial null-check
wrapper -- skips the above if the cell's [+2] field is 0 (no
door/feature here).

Run via:
    .\run_ida_script.ps1 name_dungeon_door_sprite.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x21187: "DrawDungeonCellSideFeature",
    0x2117F: "TryDrawDungeonCellSideFeature",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x21187,
    "Draws a door/side-feature sprite for the current cell: table at "
    "0xE175 (10-byte stride, 4 facing directions) indexed by the "
    "cell's [+2] id, picture drawn at z-layer 7 or 8 depending on "
    "near/far distance banding, plus a conditional overlay (picture "
    "6) for an open-door/lit-torch-like variant. Also called from "
    "sub_21217.",
    False,
)
ida_bytes.set_cmt(
    0x2117F,
    "Null-check wrapper: calls DrawDungeonCellSideFeature only if the "
    "cell's [+2] field is nonzero. Called from RenderDungeonViewRow "
    "per cell.",
    False,
)
