"""
Names sub_29B0F, a large (632-line) function used as the universal
picture/sprite drawing primitive throughout the dungeon viewport
rendering system named this session: DrawDungeonCellWallTexture,
ExtendDungeonFloorTexture, ExtendDungeonCeilingTexture,
DrawDungeonCellSideFeature, RenderDungeonVanishingPoint,
DrawMonsterAndUpdateAttackState, and RenderActiveMonsterSprites (via
the latter) all call it with a consistent parameter convention:
word_2E530 = picture id, word_2E532 = a size/scale class, word_32918 =
a z-layer/depth value (distinct values seen: 0 for wall textures, 1
for floor, 2 for ceiling, 6 for the far vanishing-point wall, 7/8 for
door/side-features, plus item/monster layers 3/4 elsewhere),
_font_bgTransparent = a transparency flag.

Given the consistent (picture id, scale class, depth layer,
transparency) signature across every viewport-rendering caller this
session found, this is confidently the dungeon viewport's core
sprite/picture blitter -- the perspective/depth-aware counterpart to
the simpler, general-purpose DrawPicture. Its own 632-line internals
(almost certainly scaling/clipping/z-ordering logic) are not traced.

-> DrawViewportSprite

Run via:
    .\run_ida_script.ps1 name_draw_viewport_sprite.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x29B0F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawViewportSprite", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawViewportSprite': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Core dungeon-viewport sprite/picture blitter (632 lines, internals "
    "not traced): draws word_2E530 (picture id) at a scale class "
    "(word_2E532) and z-layer/depth (word_32918), honoring "
    "_font_bgTransparent. Called by every dungeon-viewport rendering "
    "function named this session (walls, floor/ceiling extension, "
    "doors, vanishing point, monsters) -- the depth-aware counterpart "
    "to the simpler general-purpose DrawPicture.",
    False,
)
