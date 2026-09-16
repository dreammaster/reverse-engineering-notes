"""
Corrects the comment on PlayCharacterCreationIntroAnimation
(0x15429). When first named, its opening copy loop (al=[si]; al-=0x3F;
[di]=al, 0x442A -> 0x4D5C, 768 bytes) was described as decoding a
"simple-obfuscated graphics/data block". Re-examining file-formats.md
shows this is actually the EXACT SAME transform ShowIntroPicture uses
to build its palette fade-interpolation buffer (0x475A, also
documented as "not a second encoding layer... looks like it's
building a signed delta ... for FadePaletteStep's interpolation").
So sub_15429's copy loop is almost certainly building the same kind
of palette fade buffer, not decoding graphics -- corrected here.
0x442A is a heavily-referenced shared address elsewhere in the binary
(likely a palette/DAC staging buffer), not traced further this round.

Run via:
    .\run_ida_script.ps1 fix_char_creation_intro_anim_comment.py
"""
import ida_bytes

ea = 0x15429
ida_bytes.set_cmt(
    ea,
    "Character creation's opening animated sequence. Its first step "
    "(al=[si]; al-=0x3F; [di]=al, 0x442A->0x4D5C, 768 bytes) is the "
    "same transform ShowIntroPicture uses to build its palette "
    "fade-interpolation buffer -- corrected from an earlier 'decodes "
    "a graphics block' guess; this almost certainly builds a palette "
    "fade buffer too, not graphics data. Then plays music track "
    "0x12 and runs several staged sub-animations (63/5/20/10/10/20 "
    "frame loops via unnamed helpers), abortable via "
    "PollForEscapeKeyOnlyAlt after each stage. Called once from "
    "RunCharacterCreation.",
    False,
)
print("Comment updated for PlayCharacterCreationIntroAnimation (0x15429)")
