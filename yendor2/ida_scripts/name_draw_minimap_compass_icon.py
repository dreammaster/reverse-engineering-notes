"""
Names sub_2169C, called once from DrawMinimap: draws a small facing-
direction icon onto the minimap.

Sets a fixed position (x=0x110, y=0x20), redirects drawing to the
offscreen buffer (_videoSegment = _videoBufferSeg), forces the same
fixed glyph DrawMinimap itself uses for its 7x9 grid (word_2E532=0x90,
"entry 9" of g_pictureDir, the small 8x8 icon), then picks a 0-3
remap/variant value (word_2E530) from the party's current facing --
the same word_36CF5 tier bits (0x8000/0x4000/0x1000, else default)
used throughout the dungeon-rendering code (ShowCompassDirection,
SpawnMonsterInFacingDirection, DrawDungeonCellSideFeature) -- before
calling DrawPicture. In short: the minimap's small facing/compass
arrow icon, a graphical counterpart to ShowCompassDirection's text
HUD readout. -> DrawMinimapCompassIcon

Run via:
    .\run_ida_script.ps1 name_draw_minimap_compass_icon.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2169C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMinimapCompassIcon", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMinimapCompassIcon': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws the minimap's facing/compass icon: fixed position "
    "(0x110,0x20) into the offscreen buffer, fixed glyph 0x90 (same "
    "as DrawMinimap's 7x9 grid icon), remap value 0-3 selected by the "
    "word_36CF5 facing-tier bits (same convention as "
    "ShowCompassDirection/SpawnMonsterInFacingDirection). Called from "
    "DrawMinimap.",
    False,
)
