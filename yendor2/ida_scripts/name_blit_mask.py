"""
Follow-up to the ShowTileLegend/tile-table investigation: while ruling
out word_32926 as a color parameter, read sub_2A53C (DrawPicture+0x13's
only callee) and found it's the *factored-out* copy of a mask-expand
loop that also appears inlined twice more: directly inside DrawPicture
(at loc_29A02, gated the same way) and inside sub_29B0F (the other
icon-blit routine used by the tile-legend "highlight current cell"
cluster). All three copies are byte-for-byte identical:

    zero a 15-word scratch buffer, then, if word_328C6 bit 0 is set,
    walk word_2E490 bytes from word_2E48E, and for each byte take its
    low nibble, shift it into bits 4-7, and store as a word into the
    scratch buffer (`(byte & 0xF) << 4`, zero-extended to 16 bits).

This is a classic VGA masked-blit prep step: turning a packed
one-nibble-per-pixel mask into a one-word-per-pixel test value the
actual pixel-copy loop can compare against cheaply. word_2E48E/
word_2E490 (mask data pointer / length) are set from over a dozen
call sites throughout the game (often length 6, i.e. a 6-pixel-wide
mask) before drawing something -- so this is a general "does this
sprite have an explicit transparency mask, not just a color key"
mechanism shared by both blit paths, not specific to any one sprite.
word_328C6 itself is a much broader flags word reused by many
unrelated subsystems (confirmed by its wide variety of tested bit
patterns elsewhere -- 0x2,0x4,0x8,0x20,0x40,0x80,0x200,0x800,0x1000,
0x2000,0x4000,0x7800 all tested independently) so it is NOT renamed
here, only bit 0's role in this specific context is documented via
comments at the three use sites.

-> sub_2A53C = ExpandBlitMaskNibbles
-> word_2E48E = g_blitMaskPtr
-> word_2E490 = g_blitMaskLen

Run via:
    .\run_ida_script.ps1 name_blit_mask.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2A53C: "ExpandBlitMaskNibbles",
    0x2E48E: "g_blitMaskPtr",
    0x2E490: "g_blitMaskLen",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2A53C,
    "If word_328C6 bit 0 is set, expands g_blitMaskLen bytes from "
    "g_blitMaskPtr into a 15-word scratch buffer: each byte's low "
    "nibble becomes (nibble << 4) zero-extended to a word -- classic "
    "masked-blit prep. Factored-out copy of the same loop inlined in "
    "DrawPicture (loc_29A02) and sub_29B0F.",
    False,
)
ida_bytes.set_cmt(
    0x2E48E,
    "Pointer to the current sprite's explicit transparency/AND mask "
    "data (paired with g_blitMaskLen), consumed by "
    "ExpandBlitMaskNibbles when word_328C6 bit 0 is set. Set from ~12 "
    "call sites before drawing a masked sprite; often length 6.",
    False,
)
ida_bytes.set_cmt(
    0x2E490,
    "Length in bytes of the mask at g_blitMaskPtr (see "
    "ExpandBlitMaskNibbles). Commonly 6.",
    False,
)
