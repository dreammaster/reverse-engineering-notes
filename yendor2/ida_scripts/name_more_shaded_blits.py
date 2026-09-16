"""
Names two more members of the ShiftPaletteShadeClamped blit family.

sub_2A51B, called twice from DrawViewportSprite directly (not through
DrawRleMaskedShadedRun/DrawRleScaledSpriteColumn's RLE record table):
a straight run copy of cx=[bp-0x14] pixels, shading each via
ShiftPaletteShadeClamped, with masking (0xFF=skip) applied only when
[bp-0x20] is nonzero -- otherwise every pixel is drawn unconditionally.
The simplest member of this family: no RLE record table, no chained
hue-remap/callback effects. -> DrawShadedPixelRun

sub_2A0FC, called repeatedly from unnamed sub_29FF6 (262 bytes,
references _videoSegment, not traced this round): copies a 224-pixel-
wide row (matching DimDungeonViewport's viewport width) from si to di,
shading each pixel via ShiftPaletteShadeClamped, then advances both si
and di by 0x140 (320, one full mode-13h screen row) and repeats for an
outer count supplied by the caller -- a straightforward "copy this
viewport-width region N rows at a time, shading every pixel" primitive.
-> CopyShadedViewportRows

Run via:
    .\run_ida_script.ps1 name_more_shaded_blits.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2A51B: "DrawShadedPixelRun",
    0x2A0FC: "CopyShadedViewportRows",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2A51B,
    "Straight run copy of cx=[bp-0x14] pixels, shaded via "
    "ShiftPaletteShadeClamped; masks 0xFF pixels only if [bp-0x20] is "
    "nonzero, else draws unconditionally. Simplest member of the "
    "DrawViewportSprite shaded-blit family. Called from "
    "DrawViewportSprite.",
    False,
)
ida_bytes.set_cmt(
    0x2A0FC,
    "Copies a 224-pixel-wide row from si to di, shading each pixel "
    "via ShiftPaletteShadeClamped, then advances both by 0x140 (320) "
    "and repeats for the caller's outer count. Called from unnamed "
    "sub_29FF6.",
    False,
)
