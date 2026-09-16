"""
Names two internal RLE sprite-blit primitives called from
DrawViewportSprite (632 lines, internals not otherwise traced) --
both process a run-record table (6 bytes/record: outer-repeat-count,
inner-draw-count, skip-amount at [bx]/[bx+2]/[bx+4]), applying
ShiftPaletteShadeClamped per pixel and tail-looping (self-jump) to the
next record until the outer count hits 0.

sub_2A589: the richer variant. Inner loop does a straight `lodsb`/
`stosb` (both si and di auto-increment by 1 -- a contiguous run copy,
not necessarily a screen scanline), optionally treating pixel value
0xFF as transparent (skip, `inc di` without drawing, gated on
[bp-0x20]), shades via ShiftPaletteShadeClamped, then optionally
chains two more unnamed per-pixel effects (sub_2A4B0/sub_2A217, each
also 0xFF-transparent-aware) before writing. After each inner run,
advances si by the record's skip amount. -> DrawRleMaskedShadedRun

sub_2A5F7: a simpler, structurally different variant -- source steps
by a caller-supplied stride ([bp-0x0A]) each pixel (not auto-
increment), while the destination always advances by 0x140 (320, one
full mode-13h screen row) -- i.e. draws one on-screen *column*,
top-to-bottom, sampling the source at a configurable rate. Also
0xFF-transparent-aware and shaded via ShiftPaletteShadeClamped. This
per-column-with-configurable-source-stride shape is the classic
technique for perspective-scaled sprite columns.
-> DrawRleScaledSpriteColumn

Run via:
    .\run_ida_script.ps1 name_viewport_sprite_blit.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2A589: "DrawRleMaskedShadedRun",
    0x2A5F7: "DrawRleScaledSpriteColumn",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2A589,
    "RLE run-record blitter (6-byte records: outer count/inner "
    "count/skip at [bx]/[bx+2]/[bx+4]): contiguous lodsb/stosb copy, "
    "optional 0xFF-transparent skip, ShiftPaletteShadeClamped shading, "
    "then optional chained effects (sub_2A4B0/sub_2A217). Tail-loops "
    "to the next record. Called from DrawViewportSprite.",
    False,
)
ida_bytes.set_cmt(
    0x2A5F7,
    "RLE run-record blitter drawing one screen column (dest advances "
    "by 0x140/320 per pixel, source by a caller-supplied stride) -- "
    "the perspective-scaled-sprite-column shape. Optional "
    "0xFF-transparent skip, shaded via ShiftPaletteShadeClamped. "
    "Called from DrawViewportSprite.",
    False,
)
