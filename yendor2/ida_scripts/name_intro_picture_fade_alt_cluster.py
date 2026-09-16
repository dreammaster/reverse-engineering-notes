"""
Names 7 more functions -- byte-for-byte overlay-segment duplicates of
the character-creation wipe/fade toolkit just named, this copy used
by ShowIntroPicture (and, for the wait primitive, the sound-cue
system) instead.

sub_1192D/sub_11940: duplicates of StepPaletteRange16FadeDown/Up.
-> StepPaletteRange16FadeDownAlt / StepPaletteRange16FadeUpAlt

sub_119CA/sub_119D5: duplicates of RunPaletteRange16FadeDown/Up (loop
the above 0x3F times).
-> RunPaletteRange16FadeDownAlt / RunPaletteRange16FadeUpAlt

sub_119F6/sub_11A03 (called from ShowIntroPicture): duplicates of
TriggerPaletteRange16FadeDown/Up.
-> TriggerPaletteRange16FadeDownAlt / TriggerPaletteRange16FadeUpAlt

sub_11E39 (called from TryPlaySoundCueAlt, which already referenced
it as "still-unnamed sub_11E39"): duplicate of WaitForTickFlagAndClear.
-> WaitForTickFlagAndClearAlt

Run via:
    .\run_ida_script.ps1 name_intro_picture_fade_alt_cluster.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x1192D, "StepPaletteRange16FadeDownAlt", "Overlay-segment duplicate of StepPaletteRange16FadeDown."),
    (0x11940, "StepPaletteRange16FadeUpAlt", "Overlay-segment duplicate of StepPaletteRange16FadeUp."),
    (0x119CA, "RunPaletteRange16FadeDownAlt", "Overlay-segment duplicate of RunPaletteRange16FadeDown."),
    (0x119D5, "RunPaletteRange16FadeUpAlt", "Overlay-segment duplicate of RunPaletteRange16FadeUp."),
    (0x119F6, "TriggerPaletteRange16FadeDownAlt", "Overlay-segment duplicate of TriggerPaletteRange16FadeDown. Called from ShowIntroPicture."),
    (0x11A03, "TriggerPaletteRange16FadeUpAlt", "Overlay-segment duplicate of TriggerPaletteRange16FadeUp. Called from ShowIntroPicture."),
    (0x11E39, "WaitForTickFlagAndClearAlt", "Overlay-segment duplicate of WaitForTickFlagAndClear. Called from TryPlaySoundCueAlt."),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
