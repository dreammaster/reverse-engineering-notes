"""
Names sub_21128, called from RenderDungeonViewRow (3 sites) for each
visible cell: draws the cell's base wall texture (picture id from the
0xE551 lookup table, indexed by the cell's [+0] id, via sub_29B0F at
z-layer word_32918=0 -- distinct from the 3/4 layers used for
items/monsters standing in the cell), then conditionally draws a
fixed overlay picture (id 5) if the cell's [+6] flags have bit 0x2000
set (a door/torch/decoration marker, not confirmed which).

-> DrawDungeonCellWallTexture

Run via:
    .\run_ida_script.ps1 name_draw_dungeon_wall_texture.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21128
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawDungeonCellWallTexture", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawDungeonCellWallTexture': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one dungeon cell's base wall texture (0xE551 lookup table "
    "by cell id, z-layer word_32918=0), then a fixed overlay (picture "
    "5) if the cell's [+6] flags have bit 0x2000 set (a door/torch/"
    "decoration marker, not confirmed). Called from "
    "RenderDungeonViewRow per visible cell.",
    False,
)
