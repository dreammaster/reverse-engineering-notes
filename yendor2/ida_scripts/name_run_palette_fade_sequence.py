"""
Names sub_160D6, called twice from PlayCharacterCreationIntroAnimation
-- a multi-step palette fade sequence, a sibling of the already-named
FadePaletteStep, ties together the palette-buffer addresses already
identified this session (0x4D5C current / 0x442A target / 0x475A
output, per PlayCharacterCreationIntroAnimation's own -0x3F decode
step and ShowIntroPicture's matching fade buffer).

Takes bx=step count (word_31984), cx=RGB-triple count (word_31982,
also computed x3 into byte count word_31976), dx=palette start index
(word_2E52E, also x3 into byte offset word_322BA). For each of bx
outer steps: for each of the (cx*3) bytes, nudges the "current"
palette byte at 0x4D5C+offset one step toward the "target" byte at
0x442A+offset (if not already equal), and — unless nudging set a
0x80 sentinel bit (plausibly "reached max/done") — mirrors the new
value into the 0x475A output buffer. Then calls SetPaletteRange to
apply that step's output segment to the DAC. In short: runs a
multi-step palette fade-in sequence in one call, rather than
FadePaletteStep's single-step-per-call design.
-> RunPaletteFadeSequence

Run via:
    .\run_ida_script.ps1 name_run_palette_fade_sequence.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x160D6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunPaletteFadeSequence", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunPaletteFadeSequence': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Runs bx steps of a palette fade: nudges 0x4D5C (current) toward "
    "0x442A (target) byte-by-byte, mirrors into 0x475A (output) "
    "unless a 0x80 sentinel bit is set, then SetPaletteRange's the "
    "output segment (dx=start index, cx=RGB-triple count). A "
    "multi-step sibling of FadePaletteStep. Called from "
    "PlayCharacterCreationIntroAnimation.",
    False,
)
