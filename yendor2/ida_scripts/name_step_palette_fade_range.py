"""
Names sub_2589A, called from dozens of sites throughout the game
(66 refs) including 3 times from PlayStudioCreditsIntro -- a general,
mode-selectable palette-fade stepper, distinct from (and more
parameterized than) the existing FadePaletteStep/SetPaletteToWhite.

Dispatches on ax (0-5), each mode sharing 2 common parameter sets:
bx=word_31984 (iteration count), cx=word_31982/word_31976 (a palette
range width, tripled for the RGB-triple stride), dx=word_2E52E/
word_322BA (a starting register offset, also tripled).

Modes 0/3: fade a palette range DOWN toward black -- decrements each
byte in the working palette buffer (0x412A + offset) by 1 per call
(only as long as it's still above 0), applying the updated range via
SetPaletteRange; mode 0 first snapshots the current hardware palette
into the buffer via GetPalette. Once every byte in range has reached
0, returns without decrementing (a natural "fade complete" signal via
each individual call doing less and less work).

Modes 2/4: the inverse -- fade a palette range UP toward the master
palette (0x442A) one step per call, same completion signal. Modes 1
and 5 (not fully traced) appear to be related variants/completion
checks in the same state machine. Reads as: call once per animation
frame with a chosen direction/mode to smoothly fade a palette range
in or out over many frames. -> StepPaletteFadeRange

Run via:
    .\run_ida_script.ps1 name_step_palette_fade_range.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2589A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "StepPaletteFadeRange", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'StepPaletteFadeRange': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "General mode-selectable (ax=0-5) palette-fade stepper, called "
    "once per frame: modes 0/3 fade a palette range down toward "
    "black, modes 2/4 fade up toward the master palette (0x442A), "
    "one increment per call via SetPaletteRange. Modes 1/5 not fully "
    "traced. Distinct from the simpler FadePaletteStep/"
    "SetPaletteToWhite. Called from dozens of sites including "
    "PlayStudioCreditsIntro.",
    False,
)
