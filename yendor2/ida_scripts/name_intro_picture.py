"""
Names two more pieces of the picture-display system, found while
tracing SetPaletteRange's callers:

- sub_11953: per-step palette interpolation -- for each of cx palette
  registers, nudges the current DAC value ([di]) one step toward a
  target value ([si]), then calls SetPaletteRange with the updated
  buffer. Called in an outer loop (classic smooth fade-to/from-palette
  over N steps). -> FadePaletteStep
- sub_1177C: calls DrawPicture (picture id from word_2E530/word_2E532,
  the same globals DrawPicture/g_pictureDir use), FadePaletteStep, and
  polls byte_2E400 (the keypress byte PollKeyboardInput fills in) --
  the "show a picture, fade the palette, wait for a keypress" sequence.
  Called directly from both `start` and InitGame. -> ShowIntroPicture

Run via:
    .\run_ida_script.ps1 name_intro_picture.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x11953: "FadePaletteStep",
    0x1177C: "ShowIntroPicture",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x11953,
    "One step of a palette fade: nudges each of cx DAC registers one "
    "step from its current value toward a target buffer, then calls "
    "SetPaletteRange. Called repeatedly (once per animation frame) by "
    "ShowIntroPicture to fade a picture's palette in/out smoothly.",
    False,
)
ida_bytes.set_cmt(
    0x1177C,
    "Shows a picture (DrawPicture, id from word_2E530/word_2E532) with "
    "a palette fade (FadePaletteStep) and waits for a keypress "
    "(byte_2E400, filled by PollKeyboardInput). Called directly from "
    "`start` and InitGame -- likely the boot-time splash/logo display.",
    False,
)
