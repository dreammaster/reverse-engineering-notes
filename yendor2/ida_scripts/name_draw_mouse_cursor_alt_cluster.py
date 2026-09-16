"""
Resolves sub_238CD -- flagged earlier this session as "too broad/
general-purpose to name confidently" (264 call sites, called from
ShowAbilityDescriptionColumn, ShowIntroPicture, EnforceDemoBoundary,
and dozens more). Tracing its two small helpers reveals it's actually
the overlay-segment byte-for-byte duplicate of DrawMouseCursor's own
inline logic -- DrawMouseCursor's own pre-existing comment already
describes exactly this: "if word_3195C bit1 is set, first saves the
video buffer content at the cursor's new position into 0xE0E ... then
blits the cursor sprite from 0x3FE6 onto the video buffer with 0xFF
as a transparent color key." sub_238CD implements the identical
save+blit sequence via two helper calls instead of DrawMouseCursor's
inline code -- the same "recompiled into a different overlay segment"
duplication pattern seen throughout this session (DrawShadowedText/
Alt, ConfirmContainerInteraction/ConfirmAlchemyInteraction,
SetPaletteToWhite/Alt, etc). This also explains sub_238CD's huge
264-reference count: it's the *other* overlay segment's copy of the
single most pervasive per-event call in the game.

sub_238CD: checks word_3195C bit 0x1 (cursor moved / needs a new
background save), then bit 0x2 (already saved this position, skip).
If a fresh save is needed: caches the cursor's new position
(word_2E782/word_2E784 -> word_2E776/word_31956), calls
SaveCursorBackgroundPixels, sets bit 0x2, calls BlitCursorSprite.
-> DrawMouseCursorAlt

sub_2391C: copies a 16x8-word block of VGA memory (0xA000 segment) at
the cached cursor position into a fixed save buffer at 0xE0E, stride
0x140 (one VGA scanline) per row, clipped to the 320x200 screen --
the pixels DrawMouseCursorAlt is about to draw over.
-> SaveCursorBackgroundPixels

sub_23965: blits the 16x16 cursor sprite bitmap (source 0x3FE6) onto
VGA memory at the cached cursor position, treating byte value 0xFF as
a transparent color key, clipped to the screen edge.
-> BlitCursorSprite

Run via:
    .\run_ida_script.ps1 name_draw_mouse_cursor_alt_cluster.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x238CD, "DrawMouseCursorAlt",
     "Overlay-segment duplicate of DrawMouseCursor's inline cursor-"
     "position-sync logic: if word_3195C bit 0x1 is set and bit 0x2 "
     "isn't, caches the new cursor position and calls "
     "SaveCursorBackgroundPixels + BlitCursorSprite. Called from "
     "dozens of sites (264 refs) throughout the game -- the other "
     "overlay segment's copy of the same per-event cursor-draw call "
     "DrawMouseCursor itself handles inline."),
    (0x2391C, "SaveCursorBackgroundPixels",
     "Copies the 16x16-pixel VGA region at the cached cursor "
     "position into the fixed save buffer at 0xE0E (stride 0x140 "
     "per row, clipped to 320x200). Called from DrawMouseCursorAlt."),
    (0x23965, "BlitCursorSprite",
     "Blits the 16x16 cursor sprite bitmap (source 0x3FE6) onto VGA "
     "memory at the cached cursor position, 0xFF as the transparent "
     "color key, clipped to the screen edge. Called from "
     "DrawMouseCursorAlt."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
