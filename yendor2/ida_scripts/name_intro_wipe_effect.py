"""
Names a 3-function cluster used only by PlayCharacterCreationIntroAnimation's
staged sub-animations (called nowhere else) -- a simple pixel
wipe/reveal effect.

sub_1614D: computes a VGA mode-13h linear framebuffer offset from
row*320+col (ax=row, bx=col in; bx=offset out) -- called only by the
two functions below.

sub_1619F ("set/punch" step): computes the offset for
(word_328FA,word_32900), reads the pixel currently there into
_font_bgColor (repurposed here as a one-pixel scratch stash, not an
actual font color), then overwrites that pixel with _font_fgColor.

sub_1618E ("restore" step): computes the same offset and writes the
stashed _font_bgColor value back -- undoing sub_1619F's punch.

PlayCharacterCreationIntroAnimation's loops call restore-then-set (or
just set) while stepping (word_328FA,word_32900) by -2/+2 each
iteration and sometimes decrementing _font_fgColor -- a moving
highlight/scan-line wipe effect, not a persistent draw.

-> ComputeVgaOffsetFromRowCol / SetWipeEffectPixel / RestoreWipeEffectPixel

Run via:
    .\run_ida_script.ps1 name_intro_wipe_effect.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x1614D: "ComputeVgaOffsetFromRowCol",
    0x1619F: "SetWipeEffectPixel",
    0x1618E: "RestoreWipeEffectPixel",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1614D,
    "VGA mode-13h linear offset: bx = ax(row)*320 + bx(col). Called "
    "only by SetWipeEffectPixel/RestoreWipeEffectPixel, part of "
    "PlayCharacterCreationIntroAnimation's wipe effect.",
    False,
)
ida_bytes.set_cmt(
    0x1619F,
    "Stashes the pixel at (word_328FA,word_32900) into _font_bgColor "
    "(repurposed as scratch, not a real font color) then overwrites "
    "it with _font_fgColor -- the 'punch' half of a moving wipe "
    "effect. Called from PlayCharacterCreationIntroAnimation.",
    False,
)
ida_bytes.set_cmt(
    0x1618E,
    "Writes the stashed _font_bgColor value back to "
    "(word_328FA,word_32900), undoing SetWipeEffectPixel's punch. "
    "Called from PlayCharacterCreationIntroAnimation.",
    False,
)
