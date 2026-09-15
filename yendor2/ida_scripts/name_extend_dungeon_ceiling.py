"""
Names sub_20C8E and sub_20CEC -- the ceiling counterpart to
DrawDungeonFloorAndCeiling/ExtendDungeonFloorTexture, called as its
own step between them and RenderDungeonViewport in both
RedrawDungeonScreen and RefreshDungeonScreen.

sub_20C8E (-> ExtendDungeonCeilingPass): same 7-call row-iteration
driver shape as RenderDungeonViewport/DrawDungeonFloorAndCeiling, but
calling sub_20CEC.

sub_20CEC (-> ExtendDungeonCeilingTexture): identical to
ExtendDungeonFloorTexture but checks the cell's *ceiling* field
([si+2] vs. the floor's [si]) against word_2E498 (the ceiling picture,
vs. ExtendDungeonFloorTexture's word_2E4A0) via IsPairedValueMatch,
drawing at z-layer 2 (vs. floor's layer 1) when it matches -- a
seamless-ceiling pass.

So the dungeon-screen render sequence is now clear: 1)
DrawDungeonFloorAndCeiling (backdrop images + floor-extension), 2)
ExtendDungeonCeilingPass (ceiling-extension), 3) RenderDungeonViewport
(full wall/door/monster/encounter rendering).

Run via:
    .\run_ida_script.ps1 name_extend_dungeon_ceiling.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20C8E: "ExtendDungeonCeilingPass",
    0x20CEC: "ExtendDungeonCeilingTexture",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20C8E,
    "Ceiling-extension driver: same 7-call row pattern as "
    "RenderDungeonViewport, calling ExtendDungeonCeilingTexture. "
    "Called from RedrawDungeonScreen/RefreshDungeonScreen between "
    "DrawDungeonFloorAndCeiling and RenderDungeonViewport.",
    False,
)
ida_bytes.set_cmt(
    0x20CEC,
    "Ceiling counterpart to ExtendDungeonFloorTexture: draws the "
    "current ceiling picture (word_2E498) at z-layer 2 for cells whose "
    "[+2] ceiling field matches (IsPairedValueMatch). Called 6x by "
    "ExtendDungeonCeilingPass.",
    False,
)
