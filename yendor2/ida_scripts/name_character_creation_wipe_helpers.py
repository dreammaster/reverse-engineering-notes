"""
Names 8 more small helpers in the character-creation wipe/fade
toolkit (alongside the already-named SetWipeEffectPixel/
RestoreWipeEffectPixel/ComputeVgaOffsetFromRowCol/
RunPaletteFadeSequence/StepPaletteFadeRange).

sub_160B0/sub_160C3: single-step wrappers around
StepPaletteFadeRange for a 16-color range (bx=1, cx=0x10) -- mode 3
(fade down) / mode 4 (fade up). -> StepPaletteRange16FadeDown /
StepPaletteRange16FadeUp

sub_16159/sub_16164: loop the above 0x3F (63) times each, running a
16-color fade down/up to completion in one call.
-> RunPaletteRange16FadeDown / RunPaletteRange16FadeUp

sub_1616F: busy-waits on word_328C4 bit 0x400, clears it once seen,
then loops cx times (a tick-synced busy-wait/clear primitive).
-> WaitForTickFlagAndClear

sub_16180: the same busy-wait, but draws one
DrawCharacterCreationAnimationFrame per tick instead of just
clearing the flag -- the frame-stepping primitive
PlayCharacterCreationIntroAnimation uses.
-> WaitForTickAndDrawCreationFrame

sub_161B6/sub_161C3: set word_328C8 bit 0x800 (and an unused dx=0x90
parameter), then run RunPaletteRange16FadeDown / RunPaletteRange16FadeUp
to completion. -> TriggerPaletteRange16FadeDown / TriggerPaletteRange16FadeUp

Run via:
    .\run_ida_script.ps1 name_character_creation_wipe_helpers.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x160B0, "StepPaletteRange16FadeDown",
     "StepPaletteFadeRange(ax=3, bx=1, cx=0x10) -- one fade-down "
     "step for a 16-color palette range."),
    (0x160C3, "StepPaletteRange16FadeUp",
     "StepPaletteFadeRange(ax=4, bx=1, cx=0x10) -- one fade-up step "
     "for a 16-color palette range."),
    (0x16159, "RunPaletteRange16FadeDown",
     "Loops StepPaletteRange16FadeDown 0x3F times -- runs a 16-color "
     "fade-down to completion."),
    (0x16164, "RunPaletteRange16FadeUp",
     "Loops StepPaletteRange16FadeUp 0x3F times -- runs a 16-color "
     "fade-up to completion."),
    (0x1616F, "WaitForTickFlagAndClear",
     "Busy-waits on word_328C4 bit 0x400, clears it once set, loops "
     "cx times. A tick-synced busy-wait/clear primitive."),
    (0x16180, "WaitForTickAndDrawCreationFrame",
     "Busy-waits on word_328C4 bit 0x400 then draws one "
     "DrawCharacterCreationAnimationFrame, loops cx times. Called "
     "from PlayCharacterCreationIntroAnimation."),
    (0x161B6, "TriggerPaletteRange16FadeDown",
     "Sets word_328C8 bit 0x800, then runs RunPaletteRange16FadeDown "
     "to completion. Called from RunCharacterCreationSelectionStep."),
    (0x161C3, "TriggerPaletteRange16FadeUp",
     "Sets word_328C8 bit 0x800, then runs RunPaletteRange16FadeUp "
     "to completion. Called from RunCharacterCreationSelectionStep."),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
